// =============================================================================
//  Datos.cpp
//  Implementacion de la importacion CSV/TXT, la lectura del CREATE TABLE y la
//  insercion de registros en el disco.
// =============================================================================
#include "Datos.h"
#include "Disco_funciones.h"
#include <fstream>   // std::ifstream (leer archivos)
#include <sstream>   // std::stringstream (separar texto)
#include <cctype>    // std::tolower / std::toupper

// Intenta abrir 'ruta' probando varias ubicaciones. Asi el archivo se encuentra
// sin importar si el programa se ejecuta desde la raiz del proyecto, desde la
// carpeta build/ o desde otra ubicacion. Devuelve true si logro abrirlo.
static bool abrir_archivo(const std::string& ruta, std::ifstream& archivo) {
    const std::string candidatos[] = {     // lista de rutas a probar, en orden
        ruta,                  // la ruta tal cual (o absoluta)
        "datos/" + ruta,       // ejecutando desde la raiz del proyecto
        "../datos/" + ruta,    // ejecutando desde build/
        "../../datos/" + ruta  // ejecutando desde build/algo/
    };
    for (const std::string& c : candidatos) {
        archivo.open(c);                   // intentamos abrir esta ruta
        if (archivo.is_open()) return true; // si abrio, listo
        archivo.clear();                   // si no, limpiamos el estado de error y seguimos
    }
    return false;                          // ninguna ruta funciono
}

// Convierte una linea "a,b,c" en sus campos individuales separados por ','.
static std::vector<std::string> separar_por_comas(const std::string& linea) {
    std::vector<std::string> campos;       // aqui guardamos cada campo
    std::stringstream ss(linea);           // tratamos la linea como un "flujo" de texto
    std::string campo;
    while (std::getline(ss, campo, ',')) { // leemos hasta cada coma
        // Quita espacios al inicio y al final de cada campo.
        size_t ini = campo.find_first_not_of(" \t\r\n"); // primer caracter no-espacio
        size_t fin = campo.find_last_not_of(" \t\r\n");   // ultimo caracter no-espacio
        if (ini == std::string::npos) campos.push_back("");          // campo vacio
        else campos.push_back(campo.substr(ini, fin - ini + 1));     // campo recortado
    }
    return campos;
}

// Convierte una linea de texto a un vector de valores TIPADOS segun el esquema.
std::vector<Valor> linea_a_valores(const Esquema& esquema, const std::string& linea) {
    std::vector<std::string> campos = separar_por_comas(linea); // partimos por comas
    std::vector<Valor> valores;

    // Recorremos cada columna y tomamos su campo correspondiente.
    for (size_t i = 0; i < esquema.columnas.size(); ++i) {
        const Columna& col = esquema.columnas[i];
        std::string texto = (i < campos.size()) ? campos[i] : ""; // si falta, queda vacio

        Valor val;
        val.tipo = col.tipo;               // el valor toma el tipo de la columna
        switch (col.tipo) {
            case TipoDato::INTEGER:
                // stoi convierte el texto a numero SOLO para interpretarlo; se guarda en binario.
                try { val.entero = std::stoi(texto); }
                catch (...) { val.entero = 0; }   // si no es un numero valido, 0
                break;
            case TipoDato::DOUBLE_PRECISION:
                try { val.flotante = std::stod(texto); }
                catch (...) { val.flotante = 0.0; }
                break;
            case TipoDato::VARCHAR:
                val.cadena = texto;               // el texto se guarda tal cual
                break;
            case TipoDato::BOOLEAN: {
                // Acepta varias formas de verdadero; cualquier otra cosa es falso.
                std::string t;
                for (char c : texto) t += (char)std::tolower((unsigned char)c); // a minusculas
                val.booleano = (t == "true" || t == "1" || t == "si" || t == "sí" ||
                                t == "verdadero" || t == "v" || t == "yes" || t == "y");
                break;
            }
        }
        valores.push_back(val);
    }
    return valores;
}

// Inserta un registro tipado: serializa -> escribe en disco -> registra en catalogo.
DireccionDisco insertar_registro(const Esquema& esquema,
                                 const std::vector<Valor>& valores,
                                 Disco& disco,
                                 std::vector<EntradaCatalogo>& catalogo) {
    std::vector<unsigned char> bytes = serializar_registro(esquema, valores); // valores -> bytes
    DireccionDisco direccion = escribir_registro(disco, bytes);               // bytes -> disco

    EntradaCatalogo entrada;
    entrada.id = (int)catalogo.size();                  // id = posicion en el catalogo
    entrada.direccion = direccion;                      // su direccion fisica
    entrada.primer_sector = sector_en_direccion(disco, direccion); // puntero a su primer sector
    catalogo.push_back(entrada);                        // lo registramos en el catalogo

    return direccion;
}

// Importa todos los registros de un archivo CSV/TXT.
int importar_csv(const std::string& ruta,
                 const Esquema& esquema,
                 Disco& disco,
                 std::vector<EntradaCatalogo>& catalogo,
                 std::string& mensaje) {
    std::ifstream archivo;
    if (!abrir_archivo(ruta, archivo)) {                // intentamos abrir el archivo
        mensaje = "No se pudo abrir el archivo: " + ruta +
                  " (debe estar en la carpeta 'datos/' o usar la ruta completa).";
        return 0;
    }

    int importados = 0;                                 // contador de registros leidos
    std::string linea;
    while (std::getline(archivo, linea)) {              // leemos linea por linea
        // Ignora lineas vacias.
        if (linea.find_first_not_of(" \t\r\n") == std::string::npos) continue;

        std::vector<Valor> valores = linea_a_valores(esquema, linea); // linea -> valores
        try {
            insertar_registro(esquema, valores, disco, catalogo);     // valores -> disco
            importados++;
        } catch (const std::exception& e) {             // si el disco se lleno
            mensaje = "Error al importar (sin espacio en disco): " + std::string(e.what());
            return importados;                          // devolvemos lo que alcanzo a entrar
        }
    }

    mensaje = "Importados " + std::to_string(importados) + " registros.";
    return importados;
}

// -----------------------------------------------------------------------------
//  Lectura del esquema desde un archivo CREATE TABLE (.txt)
// -----------------------------------------------------------------------------

// Pasa un texto a MAYUSCULAS (para comparar nombres de tipo sin importar caja).
static std::string a_mayusculas(const std::string& s) {
    std::string r;
    for (char c : s) r += (char)std::toupper((unsigned char)c);
    return r;
}

// Quita espacios, tabuladores y saltos de linea al inicio y al final.
static std::string recortar(const std::string& s) {
    size_t ini = s.find_first_not_of(" \t\r\n");   // primer caracter util
    if (ini == std::string::npos) return "";       // todo eran espacios
    size_t fin = s.find_last_not_of(" \t\r\n");    // ultimo caracter util
    return s.substr(ini, fin - ini + 1);
}

// Interpreta el nombre de un tipo (y su tamano opcional) hacia un TipoDato.
// Usa los nombres de PostgreSQL como canonicos y acepta algunos sinonimos.
// Devuelve false si el tipo no se reconoce. 'tamanio' solo importa para VARCHAR.
static bool interpretar_tipo(const std::string& texto_tipo, TipoDato& tipo, int& tamanio) {
    // Separa el nombre base "VARCHAR" de un tamano entre parentesis "(20)".
    std::string base = texto_tipo;
    int n = 0;
    size_t par = texto_tipo.find('(');             // buscamos el '('
    if (par != std::string::npos) {                // si hay tamano entre parentesis...
        base = texto_tipo.substr(0, par);          // ...la parte antes del '(' es el nombre
        size_t cierre = texto_tipo.find(')', par); // ...y entre '(' y ')' va el numero
        std::string num = texto_tipo.substr(par + 1, cierre - par - 1);
        try { n = std::stoi(recortar(num)); } catch (...) { n = 0; }
    }
    // A mayusculas y SIN espacios internos, para reconocer nombres de dos
    // palabras como "DOUBLE PRECISION" o "CHARACTER VARYING".
    base = a_mayusculas(recortar(base));
    std::string b;
    for (char c : base) if (c != ' ' && c != '\t') b += c;  // quitamos espacios internos

    if (b == "INTEGER" || b == "INT" || b == "INT4") {
        tipo = TipoDato::INTEGER; tamanio = BYTES_INTEGER; return true;
    }
    if (b == "DOUBLEPRECISION" || b == "DOUBLE" || b == "FLOAT8" ||
        b == "FLOAT" || b == "REAL") {
        tipo = TipoDato::DOUBLE_PRECISION; tamanio = BYTES_DOUBLE; return true;
    }
    if (b == "BOOLEAN" || b == "BOOL") {
        tipo = TipoDato::BOOLEAN; tamanio = BYTES_BOOLEAN; return true;
    }
    if (b == "VARCHAR" || b == "CHARACTERVARYING" || b == "CHARACTER" ||
        b == "CHAR" || b == "TEXT") {
        tipo = TipoDato::VARCHAR;
        tamanio = (n > 0) ? n : 20;   // si no indican tamano, se usa 20 por defecto
        return true;
    }
    return false;                     // tipo no reconocido
}

// Lee un archivo con una sentencia tipo:
//     CREATE TABLE nombre (
//         columna1 TIPO,
//         columna2 VARCHAR(20),
//         ...
//     );
// y construye el Esquema. Devuelve true si se leyo correctamente.
bool leer_create_table(const std::string& ruta, Esquema& esquema, std::string& mensaje) {
    std::ifstream archivo;
    if (!abrir_archivo(ruta, archivo)) {
        mensaje = "No se pudo abrir el archivo: " + ruta +
                  " (debe estar en la carpeta 'datos/' o usar la ruta completa).";
        return false;
    }

    // Lee todo el archivo en un solo texto, ignorando las lineas de comentario
    // que empiezan con "--".
    std::string contenido, linea;
    while (std::getline(archivo, linea)) {
        std::string limpia = recortar(linea);
        if (limpia.rfind("--", 0) == 0) continue;   // comentario SQL: lo saltamos
        contenido += linea + "\n";
    }

    // El cuerpo (las columnas) esta entre el primer '(' y el ultimo ')'.
    size_t abre = contenido.find('(');
    size_t cierra = contenido.rfind(')');
    if (abre == std::string::npos || cierra == std::string::npos || cierra < abre) {
        mensaje = "Formato invalido: falta '( ... )' con las columnas.";
        return false;
    }
    std::string cuerpo = contenido.substr(abre + 1, cierra - abre - 1); // texto entre parentesis

    // Separa las definiciones de columna por comas.
    Esquema nuevo;                          // esquema que vamos armando
    std::stringstream ss(cuerpo);
    std::string definicion;
    while (std::getline(ss, definicion, ',')) {     // cada "nombre tipo" separado por coma
        definicion = recortar(definicion);
        if (definicion.empty()) continue;

        // El nombre de la columna es el primer token; el resto es el tipo.
        size_t esp = definicion.find_first_of(" \t");   // primer espacio
        if (esp == std::string::npos) {                 // si no hay espacio, falta el tipo
            mensaje = "Columna sin tipo: '" + definicion + "'";
            return false;
        }
        std::string nombre = recortar(definicion.substr(0, esp));       // antes del espacio
        std::string texto_tipo = recortar(definicion.substr(esp + 1));  // despues del espacio

        Columna col;
        col.nombre = nombre;
        if (!interpretar_tipo(texto_tipo, col.tipo, col.tamanio_bytes)) { // interpretamos el tipo
            mensaje = "Tipo no reconocido en la columna '" + nombre + "': " + texto_tipo;
            return false;
        }
        nuevo.columnas.push_back(col);      // agregamos la columna al esquema nuevo
    }

    if (nuevo.columnas.empty()) {           // no encontro ninguna columna valida
        mensaje = "No se encontraron columnas en el CREATE TABLE.";
        return false;
    }

    esquema = nuevo;                        // reemplazamos el esquema con el leido
    mensaje = "Esquema cargado: " + std::to_string((int)esquema.columnas.size()) + " columnas.";
    return true;
}
