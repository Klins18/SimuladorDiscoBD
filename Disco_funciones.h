#pragma once
#include "Disco.h"
#include <string>
using namespace std;

Disco inicializar_disco(int cantidad_platos,
    int cantidad_pistas_por_superficie,
    int cantidad_sectores_por_pista,
    int capacidad_bytes_por_sector
);

void liberar_disco(Disco& disco);

Sector* buscar_sector_libre(Disco& disco);

void escribir_registro(Disco& disco, string datos);

DireccionDisco calcular_direccion(int n, int num_platos, int pistas_por_sup, int sectores_por_pista);