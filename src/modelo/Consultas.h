#pragma once
// =============================================================================
//  Consultas.h
//  Catalogo de registros y BUSQUEDAS sobre la tabla (por elemento y por rango).
//
//  Las busquedas se apoyan en una ESTRUCTURA DE DATOS (interfaz EstructuraDatos,
//  p. ej. un Arbol AVL): se indexa la columna consultada y se busca sobre ella.
//  Como dependen de la interfaz y no de una estructura concreta, cambiar el AVL
//  por otra estructura no obliga a modificar estas funciones.
//
//  Las comparaciones son sobre VALORES TIPADOS, nunca sobre texto crudo.
// =============================================================================
#include "Disco.h"
#include "Esquema.h"
#include "EstructuraDatos.h"
#include <vector>

// Una entrada del catalogo = un registro almacenado en el disco.
struct EntradaCatalogo {
    int            id;             // identificador correlativo del registro
    DireccionDisco direccion;      // direccion fisica unica (primer sector)
    Sector*        primer_sector;  // puntero al primer sector (para leer la cadena)
};

// Resultado de una busqueda: el registro encontrado, ya deserializado.
struct ResultadoBusqueda {
    EntradaCatalogo    entrada;  // registro (id + direccion fisica)
    std::vector<Valor> valores;  // valores tipados de cada columna
};

// Busqueda por ELEMENTO: registros cuyo valor en 'indice_columna' es igual a
// 'objetivo'. Usa 'estructura' como indice (se llena con la columna y se busca).
std::vector<ResultadoBusqueda> buscar_por_elemento(
    EstructuraDatos& estructura,
    const Esquema& esquema,
    const std::vector<EntradaCatalogo>& catalogo,
    int indice_columna,
    const Valor& objetivo);

// Busqueda por RANGO: registros cuyo valor en 'indice_columna' esta dentro de
// [minimo, maximo] (inclusive). Tambien usa 'estructura' como indice.
std::vector<ResultadoBusqueda> buscar_por_rango(
    EstructuraDatos& estructura,
    const Esquema& esquema,
    const std::vector<EntradaCatalogo>& catalogo,
    int indice_columna,
    const Valor& minimo,
    const Valor& maximo);

void exportar_resultados_a_csv(const Esquema& esquema,
    const std::vector<ResultadoBusqueda>& resultados,
    const std::string& ruta);
