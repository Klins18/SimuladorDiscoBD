#pragma once
// =============================================================================
//  Disco_funciones.h
//  Declaracion de las operaciones sobre la geometria del disco:
//  inicializacion, liberacion, busqueda de sector libre, escritura y lectura
//  de registros (en BYTES crudos, sin castear a string) y calculo de direccion.
// =============================================================================
#include "Disco.h"
#include <vector>

// Crea e inicializa el disco con la geometria indicada. Reserva memoria para
// todos los platos, superficies, pistas y sectores, y marca los sectores libres.
Disco inicializar_disco(int cantidad_platos,
                        int cantidad_pistas_por_superficie,
                        int cantidad_sectores_por_pista,
                        int capacidad_bytes_por_sector);

// Libera toda la memoria reservada por el disco.
void liberar_disco(Disco& disco);

// Devuelve un puntero al primer sector libre del disco, o nullptr si no hay.
Sector* buscar_sector_libre(Disco& disco);

// Escribe un registro (buffer de BYTES crudos, ya serializado en binario) en el
// disco, ocupando uno o varios sectores encadenados via 'siguiente_sector'.
// Devuelve la DireccionDisco del PRIMER sector del registro = direccion unica
// del registro basada en la geometria del disco.
DireccionDisco escribir_registro(Disco& disco, const std::vector<unsigned char>& bytes);

// Lee un registro completo siguiendo la cadena de sectores a partir de
// 'primer_sector', concatenando hasta 'tamanio_registro' bytes.
std::vector<unsigned char> leer_registro(const Sector* primer_sector, int tamanio_registro);

// Devuelve el sector ubicado en una direccion fisica concreta (o nullptr).
Sector* sector_en_direccion(Disco& disco, const DireccionDisco& dir);

// Calcula la direccion fisica (plato, superficie, pista, sector) que
// corresponde al sector lineal numero 'n', dada la geometria del disco.
DireccionDisco calcular_direccion(int n, int num_platos, int pistas_por_sup, int sectores_por_pista);

// Operacion inversa de calcular_direccion: devuelve el numero de sector lineal
// que corresponde a una direccion fisica, dada la geometria.
int direccion_a_numero(const DireccionDisco& dir, int pistas_por_sup, int sectores_por_pista);
