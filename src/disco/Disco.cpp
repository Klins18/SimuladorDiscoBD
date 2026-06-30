// =============================================================================
//  Disco.cpp
//  Implementacion de las operaciones sobre la geometria del disco.
//
//  Cambio importante respecto del avance anterior:
//  'escribir_registro' YA NO recibe un std::string. Ahora recibe un buffer de
//  BYTES crudos ya serializados en binario (ver Esquema.cpp). De esta forma el
//  numero 20 se guarda como 4 bytes binarios y no como el texto "20"; es decir,
//  se elimina el casteo de todo a string que marcaba la observacion.
// =============================================================================
#include "Disco.h"            // estructuras Disco, Plato, Superficie, Pista, Sector
#include "Disco_funciones.h"  // declaraciones de estas funciones
#include <stdexcept>          // std::runtime_error (cuando no hay sectores libres)
#include <cstring>            // std::memcpy y std::memset (copiar/poner bytes)
#include <algorithm>          // std::min

// -----------------------------------------------------------------------------
// Inicializa el disco reservando memoria para toda la geometria y marcando
// cada sector como libre, con su direccion fisica asignada.
// -----------------------------------------------------------------------------
Disco inicializar_disco(int cantidad_platos,
                        int cantidad_pistas_por_superficie,
                        int cantidad_sectores_por_pista,
                        int capacidad_bytes_por_sector) {
    Disco disco;                                       // el disco que vamos a construir
    disco.cantidad_platos = cantidad_platos;           // guardamos cuantos platos tendra
    disco.platos = new Plato[cantidad_platos];         // reservamos el arreglo de platos

    // Recorremos cada PLATO (i = indice del plato).
    for (int i = 0; i < cantidad_platos; ++i) {
        disco.platos[i].numero_plato = i;              // numeramos el plato
        // Cada plato tiene 2 superficies (cara superior e inferior): j = 0 y j = 1.
        for (int j = 0; j < 2; ++j) {
            Superficie& sup = disco.platos[i].superficies[j];  // referencia a la superficie j
            sup.numero_superficie = j;                          // numeramos la superficie
            sup.cantidad_pistas = cantidad_pistas_por_superficie;
            sup.pistas = new Pista[cantidad_pistas_por_superficie]; // arreglo de pistas

            // Recorremos cada PISTA (k = indice de la pista).
            for (int k = 0; k < cantidad_pistas_por_superficie; ++k) {
                Pista& pista = sup.pistas[k];                 // referencia a la pista k
                pista.numero_pista = k;                       // numeramos la pista
                pista.cantidad_sectores = cantidad_sectores_por_pista;
                pista.sectores = new Sector[cantidad_sectores_por_pista]; // arreglo de sectores

                // Recorremos cada SECTOR (s = indice del sector).
                for (int s = 0; s < cantidad_sectores_por_pista; ++s) {
                    Sector& sec = pista.sectores[s];          // referencia al sector s
                    sec.capacidad_bytes = capacidad_bytes_por_sector;
                    sec.datos = new char[capacidad_bytes_por_sector]; // buffer de bytes del sector
                    std::memset(sec.datos, 0, capacidad_bytes_por_sector); // lo llenamos de ceros
                    sec.libre = true;                         // empieza vacio (libre)
                    sec.es_continuacion = false;              // aun no es continuacion de nada
                    // Direccion fisica del sector segun su posicion en la geometria.
                    sec.direccion.posicion_plato = i;
                    sec.direccion.posicion_superficie = j;
                    sec.direccion.posicion_pista = k;
                    sec.direccion.posicion_sector = s;
                    sec.direccion.valida = true;              // la direccion es valida
                    sec.siguiente_sector = nullptr;           // todavia no apunta a otro sector
                }
            }
        }
    }
    return disco;                                      // devolvemos el disco ya construido
}

// -----------------------------------------------------------------------------
// Libera toda la memoria reservada por el disco.
// Se libera "de adentro hacia afuera": primero los buffers de cada sector,
// luego los arreglos de sectores, de pistas y de platos.
// -----------------------------------------------------------------------------
void liberar_disco(Disco& disco) {
    if (disco.platos == nullptr) return;               // si ya esta vacio, no hay nada que liberar
    for (int i = 0; i < disco.cantidad_platos; ++i) {  // por cada plato
        for (int j = 0; j < 2; ++j) {                  // por cada superficie
            Superficie& sup = disco.platos[i].superficies[j];
            for (int k = 0; k < sup.cantidad_pistas; ++k) {        // por cada pista
                for (int s = 0; s < sup.pistas[k].cantidad_sectores; ++s) {
                    delete[] sup.pistas[k].sectores[s].datos;       // libera el buffer del sector
                }
                delete[] sup.pistas[k].sectores;        // libera el arreglo de sectores de la pista
            }
            delete[] sup.pistas;                        // libera el arreglo de pistas de la superficie
        }
    }
    delete[] disco.platos;                              // libera el arreglo de platos
    disco.platos = nullptr;                             // lo dejamos en null para no liberarlo 2 veces
}

// -----------------------------------------------------------------------------
// Busca y devuelve el primer sector libre recorriendo la geometria en orden
// (plato -> superficie -> pista -> sector). Devuelve nullptr si no hay libres.
// Este orden es el que hace que los datos se guarden "en orden" y no al azar.
// -----------------------------------------------------------------------------
Sector* buscar_sector_libre(Disco& disco) {
    for (int i = 0; i < disco.cantidad_platos; ++i) {          // recorre platos
        for (int j = 0; j < 2; ++j) {                          // recorre superficies
            Superficie& sup = disco.platos[i].superficies[j];
            for (int k = 0; k < sup.cantidad_pistas; ++k) {    // recorre pistas
                for (int s = 0; s < sup.pistas[k].cantidad_sectores; ++s) { // recorre sectores
                    if (sup.pistas[k].sectores[s].libre) {     // el primero que este libre...
                        return &sup.pistas[k].sectores[s];     // ...se devuelve (puntero a el)
                    }
                }
            }
        }
    }
    return nullptr;                                            // no quedo ningun sector libre
}

// -----------------------------------------------------------------------------
// Escribe un registro (bytes binarios ya serializados) en el disco.
// El registro puede ocupar varios sectores: se toman sectores libres y se
// encadenan mediante 'siguiente_sector'. Devuelve la direccion del PRIMER
// sector, que actua como direccion unica del registro.
// -----------------------------------------------------------------------------
DireccionDisco escribir_registro(Disco& disco, const std::vector<unsigned char>& bytes) {
    DireccionDisco direccion_registro = { -1, -1, -1, -1, false }; // direccion a devolver (aun invalida)

    // Capacidad de un sector (todos los sectores tienen la misma; usamos el primero).
    int capacidad_sector = disco.platos[0].superficies[0].pistas[0].sectores[0].capacidad_bytes;

    int offset = 0;                       // cuantos bytes del registro ya escribimos
    Sector* sector_anterior = nullptr;    // el ultimo sector usado (para encadenarlo con el siguiente)
    int total = (int)bytes.size();        // tamano total del registro en bytes

    if (total == 0) return direccion_registro;  // registro vacio: nada que escribir

    // Mientras queden bytes por escribir...
    while (offset < total) {
        Sector* sector_actual = buscar_sector_libre(disco);  // tomamos un sector libre
        if (sector_actual == nullptr) {                      // si no hay, el disco esta lleno
            throw std::runtime_error("No hay sectores libres en el disco");
        }

        // Cuantos bytes entran en ESTE sector: lo que falte, pero sin pasarnos de la capacidad.
        int bytes_a_copiar = std::min(capacidad_sector, total - offset);

        // Copiamos esos bytes CRUDOS (no texto) desde el registro al buffer del sector.
        std::memcpy(sector_actual->datos, bytes.data() + offset, bytes_a_copiar);

        sector_actual->libre = false;                       // el sector queda ocupado
        sector_actual->es_continuacion = (offset > 0);      // true si NO es el primer pedazo
        sector_actual->siguiente_sector = nullptr;          // de momento no apunta a otro

        if (offset == 0) {
            // Es el PRIMER sector del registro: su direccion es la "direccion del registro".
            direccion_registro = sector_actual->direccion;
        }
        if (sector_anterior != nullptr) {
            // Encadenamos: el sector anterior apunta a este (lista enlazada de sectores).
            sector_anterior->siguiente_sector = sector_actual;
        }

        sector_anterior = sector_actual;   // este sector pasa a ser el "anterior" de la proxima vuelta
        offset += bytes_a_copiar;          // avanzamos el contador de bytes escritos
    }
    return direccion_registro;             // devolvemos la direccion del primer sector
}

// -----------------------------------------------------------------------------
// Lee un registro completo siguiendo la cadena de sectores desde 'primer_sector'
// hasta juntar 'tamanio_registro' bytes.
// -----------------------------------------------------------------------------
std::vector<unsigned char> leer_registro(const Sector* primer_sector, int tamanio_registro) {
    std::vector<unsigned char> bytes;       // aqui vamos juntando los bytes del registro
    bytes.reserve(tamanio_registro);        // reservamos espacio (optimizacion, no obligatorio)

    const Sector* sector_actual = primer_sector;  // empezamos por el primer sector
    // Mientras haya sector y aun no juntemos todos los bytes del registro...
    while (sector_actual != nullptr && (int)bytes.size() < tamanio_registro) {
        int faltan = tamanio_registro - (int)bytes.size();          // cuantos bytes faltan
        int a_leer = std::min(sector_actual->capacidad_bytes, faltan); // leemos lo que falte (sin pasarnos)
        for (int b = 0; b < a_leer; ++b) {
            bytes.push_back((unsigned char)sector_actual->datos[b]); // copiamos byte a byte
        }
        sector_actual = sector_actual->siguiente_sector;            // saltamos al siguiente sector
    }
    // Garantiza que siempre se devuelvan 'tamanio_registro' bytes: si la cadena
    // de sectores termino antes (registro incompleto), rellena con ceros. Asi
    // deserializar_registro nunca lee fuera del buffer.
    bytes.resize(tamanio_registro, 0);
    return bytes;
}

// -----------------------------------------------------------------------------
// Devuelve el puntero al sector ubicado en una direccion fisica concreta.
// Valida cada coordenada antes de acceder, para no salirse de los arreglos.
// -----------------------------------------------------------------------------
Sector* sector_en_direccion(Disco& disco, const DireccionDisco& dir) {
    if (!dir.valida) return nullptr;                                   // direccion no valida
    if (dir.posicion_plato < 0 || dir.posicion_plato >= disco.cantidad_platos) return nullptr;
    Superficie& sup = disco.platos[dir.posicion_plato].superficies[dir.posicion_superficie];
    if (dir.posicion_pista < 0 || dir.posicion_pista >= sup.cantidad_pistas) return nullptr;
    Pista& pista = sup.pistas[dir.posicion_pista];
    if (dir.posicion_sector < 0 || dir.posicion_sector >= pista.cantidad_sectores) return nullptr;
    return &pista.sectores[dir.posicion_sector];                       // sector encontrado
}

// -----------------------------------------------------------------------------
// Calcula la direccion fisica que corresponde al sector lineal numero 'n'.
// La numeracion lineal recorre: plato -> superficie -> pista -> sector.
// Es como convertir un numero a un "sistema mixto" de coordenadas del disco.
// -----------------------------------------------------------------------------
DireccionDisco calcular_direccion(int n, int num_platos, int pistas_por_sup, int sectores_por_pista) {
    DireccionDisco dir = { -1, -1, -1, -1, false };       // resultado por defecto: invalido
    if (num_platos <= 0 || pistas_por_sup <= 0 || sectores_por_pista <= 0)
        return dir;                                       // geometria invalida

    int sectores_por_sup = pistas_por_sup * sectores_por_pista; // sectores que hay en una superficie
    int sectores_por_plato = 2 * sectores_por_sup;              // x2 porque el plato tiene 2 superficies
    int total_sectores = num_platos * sectores_por_plato;       // total de sectores del disco
    if (n < 0 || n >= total_sectores)
        return dir;                                       // 'n' fuera de rango

    int resto = n;                                        // iremos "descomponiendo" n
    // Cada division entera da la coordenada; el modulo (resto) sigue con lo que sobra.
    dir.posicion_plato = resto / sectores_por_plato; resto %= sectores_por_plato;
    dir.posicion_superficie = resto / sectores_por_sup;  resto %= sectores_por_sup;
    dir.posicion_pista = resto / sectores_por_pista; resto %= sectores_por_pista;
    dir.posicion_sector = resto;                         // lo que queda es el sector
    dir.valida = true;
    return dir;
}

// -----------------------------------------------------------------------------
// Operacion inversa: numero de sector lineal a partir de una direccion fisica.
// Multiplica cada coordenada por su "peso" y los suma (como armar un numero).
// -----------------------------------------------------------------------------
int direccion_a_numero(const DireccionDisco& dir, int pistas_por_sup, int sectores_por_pista) {
    if (!dir.valida) return -1;                          // direccion invalida
    int sectores_por_sup = pistas_por_sup * sectores_por_pista;
    int sectores_por_plato = 2 * sectores_por_sup;
    return dir.posicion_plato * sectores_por_plato        // peso del plato
         + dir.posicion_superficie * sectores_por_sup     // peso de la superficie
         + dir.posicion_pista * sectores_por_pista        // peso de la pista
         + dir.posicion_sector;                           // + el sector
}
