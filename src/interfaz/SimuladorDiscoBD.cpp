// =============================================================================
//  SimuladorDiscoBD.cpp
//  Interfaz grafica (SFML 3 + ImGui) del simulador de almacenamiento fisico.
//
//  Disposicion:
//    - Panel izquierdo: configuracion del disco y estructura de la tabla.
//    - Panel derecho con pestanas: Datos / Consulta.
//    - Barra de estado inferior (mensaje + conteo de registros).
//
//  Los datos se guardan en BINARIO tipado (ver Esquema.cpp). Las busquedas usan
//  la interfaz EstructuraDatos; aqui la implementacion concreta es ArbolAVL, asi
//  que para cambiar de estructura solo se cambia el tipo de la variable 'indice'.
// =============================================================================
#include <SFML/Graphics.hpp>   // ventana y dibujo (SFML)
#include <imgui.h>             // widgets (Dear ImGui)
#include <imgui-SFML.h>        // union entre ImGui y SFML
#include <string>
#include <vector>
#include <cstdio>            
#include <windows.h>          
#include <commdlg.h>           // std::snprintf, popen/pclose
#include <optional>           // std::optional (lo devuelve window.pollEvent en SFML 3)
#include "Disco_funciones.h"
#include "Esquema.h"
#include "Consultas.h"
#include "Datos.h"
#include "ArbolAVL.h"

#ifdef _WIN32
#define popen _popen
#define pclose _pclose
#endif
// Nombres de los tipos para el menu desplegable (mismo orden que TipoDato,
// con los mismos nombres que usa PostgreSQL).
static const char* NOMBRES_TIPOS[] = { "INTEGER", "DOUBLE PRECISION", "VARCHAR", "BOOLEAN" };

// Bytes por defecto de un tipo (VARCHAR usa el valor que elige el usuario).
static int bytes_por_defecto(int tipo_idx, int bytes_varchar) {
    if (tipo_idx == 0) return BYTES_INTEGER;   // INTEGER
    if (tipo_idx == 1) return BYTES_DOUBLE;    // DOUBLE PRECISION
    if (tipo_idx == 3) return BYTES_BOOLEAN;   // BOOLEAN
    return bytes_varchar;                      // VARCHAR (indice 2)
}

// Abre el selector de archivos NATIVO de macOS (el Finder) y devuelve la ruta
// elegida, o cadena vacia si se cancelo a traves de 'osascript' (AppleScript)
static std::string elegir_archivo(const char* titulo) {
    char buffer[512] = "";

    OPENFILENAMEA ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFile = buffer;
    ofn.nMaxFile = sizeof(buffer);
    ofn.lpstrFilter = "Archivos (*.csv;*.txt)\0*.csv;*.txt\0Todos\0*.*\0";
    ofn.lpstrTitle = titulo;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if (GetOpenFileNameA(&ofn)) {
        return std::string(buffer);
    }
    return "";  // el usuario cancelo
}

// Boton "Abrir" que abre el Finder y, si se elige un archivo, copia la ruta a 'destino'.
static void boton_explorar(const char* id, const char* titulo, char* destino, size_t tam) {
    if (ImGui::Button(id)) {                       // si se pulsa el boton...
        std::string r = elegir_archivo(titulo);    // abrimos el Finder
        if (!r.empty()) std::snprintf(destino, tam, "%s", r.c_str()); // copiamos la ruta
    }
}

// Dibuja una tabla (contenido o resultados) de registros ya leidos del disco.
static void tabla_registros(const char* id, const Esquema& esquema,
    const std::vector<ResultadoBusqueda>& filas,
    float alto = 0) {
    int n_cols = (int)esquema.columnas.size();
    ImVec2 avail = ImGui::GetContentRegionAvail();
    float alto_tabla = (alto > 0) ? alto : avail.y;     // espacio disponible (para llenar lo que queda)
    // Tabla con: columna "#" + una columna por campo + columna de direccion.
    if (ImGui::BeginTable(id, n_cols + 2,
            ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY,
            ImVec2(0, avail.y))) {
        ImGui::TableSetupScrollFreeze(0, 1);            // deja la fila de cabeceras fija al hacer scroll
        ImGui::TableSetupColumn("#", ImGuiTableColumnFlags_WidthFixed, 36);
        for (const Columna& c : esquema.columnas) ImGui::TableSetupColumn(c.nombre.c_str());
        ImGui::TableSetupColumn("Dir (P,S,Pi,Se)", ImGuiTableColumnFlags_WidthFixed, 130);
        ImGui::TableHeadersRow();                       // dibuja la fila de cabeceras

        for (const ResultadoBusqueda& r : filas) {      // por cada registro a mostrar
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::Text("%d", r.entrada.id);  // columna #
            for (int c = 0; c < n_cols; ++c) {          // una celda por cada campo
                ImGui::TableSetColumnIndex(c + 1);
                ImGui::TextUnformatted(valor_a_texto(r.valores[c]).c_str()); // valor como texto
            }
            const DireccionDisco& d = r.entrada.direccion;
            ImGui::TableSetColumnIndex(n_cols + 1);     // ultima columna: direccion fisica
            ImGui::Text("%d,%d,%d,%d", d.posicion_plato, d.posicion_superficie,
                        d.posicion_pista, d.posicion_sector);
        }
        ImGui::EndTable();
    }
}

int main() {
    // Creamos la ventana de la aplicacion (ancho x alto) y arrancamos ImGui.
    sf::RenderWindow window(sf::VideoMode({ 1050, 720 }), "Simulador de Disco - Base de Datos");
    window.setFramerateLimit(60);
    ImGui::SFML::Init(window);          // construye el atlas de fuentes
    ImGui::StyleColorsDark();           // tema oscuro
    sf::Clock clock;                    // reloj para medir el tiempo entre frames

    // --- Geometria del disco (valores iniciales) ---
    int platos = 2, pistas = 3, sectores = 4, capacidad = 16;
    Disco disco = inicializar_disco(platos, pistas, sectores, capacidad);

    // --- Tabla: esquema + catalogo de registros ---
    Esquema esquema;                                // estructura de la tabla (columnas)
    std::vector<EntradaCatalogo> catalogo;          // lista de registros guardados

    // --- Estructura de datos para las busquedas (interfaz EstructuraDatos).
    //     Para usar otra estructura, basta cambiar este tipo por el de otra
    //     clase que herede de EstructuraDatos.
    ArbolAVL indice;

    // --- Estado de los formularios (lo que el usuario escribe en la pantalla) ---
    char nombre_columna[64] = "";
    int  tipo_columna = 0;          // 0=INTEGER, 1=DOUBLE PRECISION, 2=VARCHAR, 3=BOOLEAN
    int  bytes_columna = 20;
    char ruta_archivo[256] = "datos_ejemplo.csv";   // ruta del CSV de datos
    char ruta_esquema[256] = "esquema_ejemplo.txt"; // ruta del CREATE TABLE
    std::vector<std::string> entradas;   // un buffer de texto por columna (insercion manual)
    std::string mensaje = "Listo.";      // mensaje de la barra de estado

    int  col_consulta = 0;          // columna elegida para consultar
    int  modo = 0;                  // 0 = elemento, 1 = rango
    char val_min[64] = "", val_max[64] = ""; // valores de la consulta
    std::vector<ResultadoBusqueda> resultados;   // resultados de la ultima busqueda

    // Vacia la tabla: reinicia el disco y borra el catalogo. Se usa al cambiar
    // la estructura o antes de importar, para que los datos siempre encajen con
    // el esquema vigente.
    auto vaciar_tabla = [&]() {
        liberar_disco(disco);
        disco = inicializar_disco(platos, pistas, sectores, capacidad);
        catalogo.clear();
        resultados.clear();
    };

    // Convierte el texto de un formulario al Valor tipado de una columna.
    auto a_valor = [](const Columna& col, const char* txt) {
        Valor v; v.tipo = col.tipo;
        if (col.tipo == TipoDato::INTEGER)        { try { v.entero = std::stoi(txt); } catch (...) { v.entero = 0; } }
        else if (col.tipo == TipoDato::DOUBLE_PRECISION) { try { v.flotante = std::stod(txt); } catch (...) { v.flotante = 0; } }
        else                                     { v.cadena = txt; }  // VARCHAR/BOOLEAN: texto tal cual
        return v;
    };

    // Lee del disco todos los registros del catalogo (para mostrarlos en la tabla).
    auto leer_catalogo = [&]() {
        std::vector<ResultadoBusqueda> filas;
        int tam = esquema.tamanio_registro();
        for (const EntradaCatalogo& e : catalogo) {
            ResultadoBusqueda r; r.entrada = e;
            std::vector<unsigned char> bytes = leer_registro(e.primer_sector, tam);
            r.valores = deserializar_registro(esquema, bytes.data());
            filas.push_back(r);
        }
        return filas;
    };

    // ====================== BUCLE PRINCIPAL (un giro por frame) ======================
    while (window.isOpen()) {
        // 1) Procesar eventos (teclado, mouse, cerrar ventana).
        while (const std::optional event = window.pollEvent()) {
            ImGui::SFML::ProcessEvent(window, *event);
            if (event->is<sf::Event::Closed>()) window.close();
        }
        ImGui::SFML::Update(window, clock.restart());   // actualiza ImGui (tiempo del frame)

        // 2) Definir una ventana ImGui que ocupa TODA la pantalla.
        const ImGuiViewport* vp = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(vp->WorkPos);
        ImGui::SetNextWindowSize(vp->WorkSize);
        ImGui::Begin("Simulador", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoBringToFrontOnFocus);

        int total_sectores = platos * 2 * pistas * sectores;   // sectores totales del disco
        int n_cols = (int)esquema.columnas.size();             // numero de columnas del esquema
        float pie = ImGui::GetFrameHeightWithSpacing() + 4;    // alto reservado para la barra de estado

        // ================= PANEL IZQUIERDO =================
        ImGui::BeginChild("panel_izq", ImVec2(360, -pie), true);  // sub-area de 360px de ancho

        // ----- Seccion: configuracion del disco -----
        if (ImGui::CollapsingHeader("Configuracion del disco", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::PushItemWidth(-130);                 // ancho de los campos
            ImGui::InputInt("Platos", &platos);         // cada InputInt edita una variable
            ImGui::InputInt("Pistas", &pistas);
            ImGui::InputInt("Sectores", &sectores);
            ImGui::InputInt("Capacidad (bytes)", &capacidad);
            ImGui::PopItemWidth();
            if (platos < 1) platos = 1;                 // evitamos valores invalidos (minimo 1)
            if (pistas < 1) pistas = 1;
            if (sectores < 1) sectores = 1;
            if (capacidad < 1) capacidad = 1;
            if (ImGui::Button("Inicializar disco", ImVec2(-1, 28))) {
                vaciar_tabla();                         // crea el disco con la nueva geometria
                mensaje = "Disco inicializado.";
            }
            ImGui::TextDisabled("Total: %d sectores (%d bytes)", total_sectores, total_sectores * capacidad);
        }

        // ----- Seccion: estructura de la tabla -----
        if (ImGui::CollapsingHeader("Estructura de la tabla", ImGuiTreeNodeFlags_DefaultOpen)) {
            // Cargar el esquema completo desde un archivo CREATE TABLE (.txt).
            ImGui::PushItemWidth(-170);
            ImGui::InputText("CREATE TABLE", ruta_esquema, sizeof(ruta_esquema)); // ruta del .txt
            ImGui::PopItemWidth();
            ImGui::SameLine();
            boton_explorar("Abrir##esq", "Elegir archivo CREATE TABLE", ruta_esquema, sizeof(ruta_esquema));
            if (ImGui::Button("Cargar esquema (.txt)", ImVec2(-1, 26))) {
                Esquema nuevo;
                if (leer_create_table(ruta_esquema, nuevo, mensaje)) {  // si se leyo bien...
                    vaciar_tabla();                          // ...vaciamos la tabla
                    esquema = nuevo;                         // ...y adoptamos el nuevo esquema
                    entradas.assign(esquema.columnas.size(), ""); // un buffer por columna
                }
            }
            ImGui::Separator();

            // Agregar una columna a mano.
            ImGui::PushItemWidth(-130);
            ImGui::InputText("Nombre", nombre_columna, sizeof(nombre_columna));
            ImGui::Combo("Tipo", &tipo_columna, NOMBRES_TIPOS, IM_ARRAYSIZE(NOMBRES_TIPOS));
            // Solo VARCHAR (indice 2) pide cuantos bytes.
            if (tipo_columna == 2) { ImGui::InputInt("Bytes (VARCHAR)", &bytes_columna); if (bytes_columna < 1) bytes_columna = 1; }
            ImGui::PopItemWidth();
            if (ImGui::Button("Agregar columna", ImVec2(-1, 26))) {
                if (std::string(nombre_columna).size() > 0) {       // si hay un nombre...
                    if (!catalogo.empty()) vaciar_tabla();  // cambia el tamano de registro -> vaciar
                    // Construimos la columna con su nombre, tipo y bytes.
                    Columna col{ nombre_columna, (TipoDato)tipo_columna, bytes_por_defecto(tipo_columna, bytes_columna) };
                    esquema.columnas.push_back(col);
                    entradas.push_back("");                 // su buffer de insercion manual
                    nombre_columna[0] = '\0';               // limpiamos el campo del nombre
                    mensaje = "Columna agregada: " + col.nombre;
                }
            }
            if (ImGui::Button("Limpiar tabla", ImVec2(-1, 22))) {
                vaciar_tabla(); esquema.columnas.clear(); entradas.clear();
                mensaje = "Esquema y datos limpiados.";
            }

            // Lista de columnas definidas (tabla con nombre/tipo/bytes).
            if (ImGui::BeginTable("esquema", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
                ImGui::TableSetupColumn("Columna");
                ImGui::TableSetupColumn("Tipo");
                ImGui::TableSetupColumn("Bytes");
                ImGui::TableHeadersRow();
                for (const Columna& c : esquema.columnas) {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0); ImGui::TextUnformatted(c.nombre.c_str());
                    ImGui::TableSetColumnIndex(1); ImGui::TextUnformatted(NOMBRES_TIPOS[(int)c.tipo]);
                    ImGui::TableSetColumnIndex(2); ImGui::Text("%d", c.tamanio_bytes);
                }
                ImGui::EndTable();
            }
            ImGui::TextDisabled("Tamano de cada registro: %d bytes", esquema.tamanio_registro());
        }

        ImGui::EndChild();   // fin del panel izquierdo

        // ================= PANEL DERECHO (pestanas) =================
        ImGui::SameLine();                                       // a la derecha del panel izquierdo
        ImGui::BeginChild("panel_der", ImVec2(0, -pie), false);  // ocupa el resto del ancho
        if (ImGui::BeginTabBar("pestanas")) {

            // ----- Pestana: Datos -----
            if (ImGui::BeginTabItem("  Datos  ")) {
                // Campo de ruta del CSV + boton para elegirlo con el Finder.
                ImGui::PushItemWidth(280);
                ImGui::InputText("Archivo CSV", ruta_archivo, sizeof(ruta_archivo));
                ImGui::PopItemWidth();
                ImGui::SameLine();
                boton_explorar("Abrir##csv", "Elegir archivo de datos (CSV)", ruta_archivo, sizeof(ruta_archivo));
                ImGui::SameLine();
                if (ImGui::Button("Cargar")) {
                    if (n_cols == 0) mensaje = "Primero define el esquema.";
                    else { vaciar_tabla(); importar_csv(ruta_archivo, esquema, disco, catalogo, mensaje); }
                }
                ImGui::SameLine();
                if (ImGui::Button("Limpiar datos")) { vaciar_tabla(); mensaje = "Datos vaciados."; }

                if (n_cols == 0) {
                    ImGui::TextDisabled("Define el esquema (panel izquierdo) para ingresar datos.");
                } else {
                    // Insercion manual: un campo por columna (de a 3 por fila).
                    ImGui::PushItemWidth(150);
                    for (int i = 0; i < n_cols; ++i) {
                        char buf[128]; std::snprintf(buf, sizeof(buf), "%s", entradas[i].c_str());
                        if (i > 0 && i % 3 != 0) ImGui::SameLine();  // 3 campos por linea
                        if (ImGui::InputText(esquema.columnas[i].nombre.c_str(), buf, sizeof(buf)))
                            entradas[i] = buf;                       // guardamos lo que escribe
                    }
                    ImGui::PopItemWidth();
                    if (ImGui::Button("Insertar registro")) {
                        // Unimos los campos con comas y reutilizamos linea_a_valores.
                        std::string linea;
                        for (int i = 0; i < n_cols; ++i) { if (i) linea += ","; linea += entradas[i]; }
                        try { insertar_registro(esquema, linea_a_valores(esquema, linea), disco, catalogo);
                              mensaje = "Registro insertado."; }
                        catch (const std::exception& e) { mensaje = std::string("Error: ") + e.what(); }
                    }

                    ImGui::Separator();
                    ImGui::Text("Contenido (%d registros)  |  Dir = P:Plato S:Superficie Pi:Pista Se:Sector",
                                (int)catalogo.size());
                    tabla_registros("contenido", esquema, leer_catalogo());  // tabla con todos los registros
                }
                ImGui::EndTabItem();
            }

            // ----- Pestana: Consulta -----
            if (ImGui::BeginTabItem("  Consulta  ")) {
                if (n_cols == 0) {
                    ImGui::TextDisabled("Define el esquema y carga datos para consultar.");
                } else {
                    // Lista de nombres de columna para el desplegable.
                    std::vector<const char*> nombres;
                    for (const Columna& c : esquema.columnas) nombres.push_back(c.nombre.c_str());
                    if (col_consulta >= n_cols) col_consulta = 0;

                    ImGui::PushItemWidth(160);
                    ImGui::Combo("Columna", &col_consulta, nombres.data(), n_cols);  // elegir columna
                    ImGui::PopItemWidth();
                    ImGui::SameLine(); ImGui::RadioButton("Elemento", &modo, 0);     // modo elemento
                    ImGui::SameLine(); ImGui::RadioButton("Rango", &modo, 1);        // modo rango

                    ImGui::PushItemWidth(160);
                    if (modo == 0) ImGui::InputText("Valor", val_min, sizeof(val_min));      // un valor
                    else { ImGui::InputText("Minimo", val_min, sizeof(val_min)); ImGui::SameLine();
                           ImGui::InputText("Maximo", val_max, sizeof(val_max)); }            // min y max
                    ImGui::PopItemWidth();

                    if (ImGui::Button("Buscar")) {
                        const Columna& col = esquema.columnas[col_consulta];
                        if (modo == 0)   // por elemento
                            resultados = buscar_por_elemento(indice, esquema, catalogo, col_consulta, a_valor(col, val_min));
                        else             // por rango
                            resultados = buscar_por_rango(indice, esquema, catalogo, col_consulta, a_valor(col, val_min), a_valor(col, val_max));
                        mensaje = "Consulta: " + std::to_string(resultados.size()) + " resultado(s).";
                    }
                    ImGui::SameLine();
                    // Mostramos que la busqueda se hizo con el AVL.
                    ImGui::TextDisabled("Estructura: %s  |  %d resultado(s)", indice.nombre(), (int)resultados.size());

                    ImGui::Separator();
                    tabla_registros("resultados", esquema, resultados, 250); // tabla con los resultados

                    ImGui::Separator();
                    static char ruta_export[256] = "exportados\\resultados.csv";
                    ImGui::PushItemWidth(200);
                    ImGui::InputText("Archivo de salida", ruta_export, sizeof(ruta_export));
                    ImGui::PopItemWidth();
                    ImGui::SameLine();
                    if (ImGui::Button("Exportar resultados a CSV")) {
                        if (resultados.empty()) {
                            mensaje = "No hay resultados para exportar. Realiza una busqueda primero.";
                        }
                        else {
                            try {
                                exportar_resultados_a_csv(esquema, resultados, ruta_export);
                                mensaje = "Exportado a: " + std::string(ruta_export);
                            }
                            catch (const std::exception& e) {
                                mensaje = std::string("Error al exportar: ") + e.what();
                            }
                        }
                    }

                }
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }
        ImGui::EndChild();   // fin del panel derecho

        // ================= BARRA DE ESTADO =================
        ImGui::Separator();
        ImGui::Text("Estado: %s", mensaje.c_str());     // mensaje a la izquierda
        char info[96];
        std::snprintf(info, sizeof(info), "Registros: %d", (int)catalogo.size());
        float ancho = ImGui::CalcTextSize(info).x;      // ancho del texto para alinearlo a la derecha
        ImGui::SameLine(ImGui::GetWindowWidth() - ancho - 20);
        ImGui::TextDisabled("%s", info);                // conteo de registros a la derecha

        ImGui::End();                       // fin de la ventana principal
        window.clear(sf::Color(25, 25, 30)); // limpiamos la pantalla (color de fondo)
        ImGui::SFML::Render(window);        // dibujamos ImGui
        window.display();                   // mostramos el frame
    }

    liberar_disco(disco);     // liberamos la memoria del disco al cerrar
    ImGui::SFML::Shutdown();  // cerramos ImGui
    return 0;
}
