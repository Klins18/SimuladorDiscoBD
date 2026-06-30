#pragma once
// =============================================================================
//  EstructuraDatos.h
//  INTERFAZ (contrato) de una estructura de datos usada como indice de la tabla.
//
//  Idea clave del diseno:
//  El resto del programa (consultas, interfaz) NO depende de una estructura
//  concreta, sino de esta interfaz. La estructura asocia una CLAVE tipada (el
//  valor de una columna) con los IDs de los registros que tienen esa clave, y
//  ofrece las operaciones de insercion, busqueda y limpieza.
//
//  Para cambiar de estructura de datos (por ejemplo, de Arbol AVL a una tabla
//  hash o a un Arbol B), basta con crear otra clase que herede de EstructuraDatos
//  e implemente estos metodos. No hay que tocar las consultas ni la interfaz.
//
//      EstructuraDatos           <- interfaz (esta clase abstracta)
//          ^         ^
//          |         |
//      ArbolAVL   (otra estructura futura: TablaHash, ArbolB, ...)
// =============================================================================
#include "Esquema.h"   // Valor, comparar_valores
#include <vector>

class EstructuraDatos {
public:
    virtual ~EstructuraDatos() {}

    // Inserta una clave (valor de la columna indexada) junto con el id del
    // registro al que pertenece. Una misma clave puede repetirse (varios
    // registros con el mismo valor).
    virtual void insertar(const Valor& clave, int id_registro) = 0;

    // Busqueda por ELEMENTO: devuelve los ids de los registros cuya clave es
    // IGUAL a 'clave'.
    virtual std::vector<int> buscar_elemento(const Valor& clave) const = 0;

    // Busqueda por RANGO: devuelve los ids de los registros cuya clave esta
    // dentro de [minimo, maximo] (inclusive).
    virtual std::vector<int> buscar_rango(const Valor& minimo,
                                          const Valor& maximo) const = 0;

    // Vacia la estructura (la deja sin ninguna clave).
    virtual void limpiar() = 0;

    // Nombre de la estructura concreta (p. ej. "Arbol AVL"). Util para mostrarlo
    // en la interfaz y para diagnostico.
    virtual const char* nombre() const = 0;
};
