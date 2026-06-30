// =============================================================================
//  Consultas.cpp
//  Busquedas por elemento y por rango usando una EstructuraDatos como indice.
//
//  Pasos de una busqueda:
//    1. Se indexa la columna consultada: se recorre el catalogo, se lee y
//       deserializa cada registro, y se inserta (valor_de_la_columna, id) en la
//       estructura (a traves de la interfaz EstructuraDatos).
//    2. Se pregunta a la estructura por los ids que coinciden (elemento o rango).
//    3. Se arma el resultado leyendo cada registro por su id.
// =============================================================================
#include <fstream>
#include <filesystem>
#include <stdexcept>
#include "Consultas.h"
#include "Disco_funciones.h"

// Lee y deserializa el registro cuyo id = posicion en el catalogo.
// Devuelve un ResultadoBusqueda (la entrada del catalogo + sus valores tipados).
static ResultadoBusqueda armar_resultado(
    const Esquema& esquema, const std::vector<EntradaCatalogo>& catalogo, int id) {
    ResultadoBusqueda r;
    r.entrada = catalogo[id];                          // datos del registro (id, direccion, sector)
    // Leemos sus bytes desde el disco (siguiendo la cadena de sectores)...
    std::vector<unsigned char> bytes = leer_registro(catalogo[id].primer_sector,
                                                     esquema.tamanio_registro());
    r.valores = deserializar_registro(esquema, bytes.data()); // ...y los volvemos valores tipados
    return r;
}

// Indexa la columna 'col' de todos los registros dentro de la estructura.
// (Llena el AVL con pares: valor de la columna -> id del registro.)
static void indexar_columna(EstructuraDatos& estructura, const Esquema& esquema,
                            const std::vector<EntradaCatalogo>& catalogo, int col) {
    estructura.limpiar();                              // vaciamos el indice anterior
    int tam = esquema.tamanio_registro();
    for (const EntradaCatalogo& e : catalogo) {        // por cada registro del catalogo
        std::vector<unsigned char> bytes = leer_registro(e.primer_sector, tam); // leer del disco
        std::vector<Valor> valores = deserializar_registro(esquema, bytes.data()); // a valores
        estructura.insertar(valores[col], e.id);       // insertamos (valor de la columna, id)
    }
}

void exportar_resultados_a_csv(const Esquema& esquema,
    const std::vector<ResultadoBusqueda>& resultados,
    const std::string& ruta) {
    std::filesystem::path p(ruta);
    if (p.has_parent_path()) {
        std::filesystem::create_directories(p.parent_path());
    }
    std::ofstream archivo(ruta);
    if (!archivo.is_open()) {
        throw std::runtime_error("No se pudo crear el archivo: " + ruta);
    }

    // Encabezado: nombres de columna separados por coma.
    for (size_t i = 0; i < esquema.columnas.size(); ++i) {
        if (i > 0) archivo << ",";
        archivo << esquema.columnas[i].nombre;
    }
    archivo << "\n";

    // Una fila por cada resultado.
    for (const ResultadoBusqueda& r : resultados) {
        for (size_t i = 0; i < r.valores.size(); ++i) {
            if (i > 0) archivo << ",";
            archivo << valor_a_texto(r.valores[i]);
        }
        archivo << "\n";
    }
}

// BUSQUEDA POR ELEMENTO: registros cuyo valor en 'indice_columna' es == objetivo.
std::vector<ResultadoBusqueda> buscar_por_elemento(
    EstructuraDatos& estructura, const Esquema& esquema,
    const std::vector<EntradaCatalogo>& catalogo, int indice_columna,
    const Valor& objetivo) {
    indexar_columna(estructura, esquema, catalogo, indice_columna); // 1) armamos el indice
    std::vector<ResultadoBusqueda> resultados;
    // 2) le pedimos al indice los ids iguales y 3) armamos cada resultado.
    for (int id : estructura.buscar_elemento(objetivo))
        resultados.push_back(armar_resultado(esquema, catalogo, id));
    return resultados;
}

// BUSQUEDA POR RANGO: registros cuyo valor esta dentro de [minimo, maximo].
std::vector<ResultadoBusqueda> buscar_por_rango(
    EstructuraDatos& estructura, const Esquema& esquema,
    const std::vector<EntradaCatalogo>& catalogo, int indice_columna,
    const Valor& minimo, const Valor& maximo) {
    indexar_columna(estructura, esquema, catalogo, indice_columna); // 1) armamos el indice
    std::vector<ResultadoBusqueda> resultados;
    // 2) ids dentro del rango y 3) armamos cada resultado.
    for (int id : estructura.buscar_rango(minimo, maximo))
        resultados.push_back(armar_resultado(esquema, catalogo, id));
    return resultados;
}
