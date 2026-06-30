#pragma once
// =============================================================================
//  Disco.h
//  Modelo de la GEOMETRIA FISICA del disco duro que se simula.
//
//  Jerarquia (de mayor a menor):
//      Disco -> Platos -> Superficies (2 por plato) -> Pistas -> Sectores
//
//  El sector es la unidad minima de almacenamiento. Cada sector guarda un
//  buffer de bytes CRUDOS (no texto) y puede encadenarse al siguiente sector
//  cuando un registro no cabe en uno solo (registros de longitud variable).
// =============================================================================

// Direccion fisica unica dentro del disco, basada en su geometria.
struct DireccionDisco {
    int  posicion_plato;       // indice del plato
    int  posicion_superficie;  // indice de la superficie (0 o 1)
    int  posicion_pista;       // indice de la pista
    int  posicion_sector;      // indice del sector dentro de la pista
    bool valida;               // true si la direccion es valida
};

// Sector: unidad minima de almacenamiento.
struct Sector {
    int            capacidad_bytes;   // capacidad del sector en bytes
    char*          datos;             // buffer de bytes CRUDOS (no es texto)
    bool           libre;             // true si el sector no esta ocupado
    bool           es_continuacion;   // true si es continuacion de un registro previo
    DireccionDisco direccion;         // direccion fisica de este sector
    Sector*        siguiente_sector;  // siguiente sector del registro (encadenamiento)
};

// Pista: conjunto de sectores.
struct Pista {
    int     numero_pista;       // indice de la pista
    int     cantidad_sectores;  // sectores por pista
    Sector* sectores;           // arreglo de sectores
};

// Superficie: una de las dos caras de un plato; contiene pistas.
struct Superficie {
    int    numero_superficie;  // indice de la superficie (0 o 1)
    int    cantidad_pistas;    // pistas por superficie
    Pista* pistas;             // arreglo de pistas
};

// Plato: tiene exactamente 2 superficies (cara superior e inferior).
struct Plato {
    int        numero_plato;     // indice del plato
    Superficie superficies[2];   // 2 superficies por plato
};

// Disco: conjunto de platos.
struct Disco {
    int    cantidad_platos;  // numero de platos
    Plato* platos;           // arreglo de platos
};
