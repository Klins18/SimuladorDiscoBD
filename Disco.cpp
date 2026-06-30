#include "Disco.h"
#include "Disco_funciones.h"
#include <stdexcept>

Disco inicializar_disco(int cantidad_platos, int cantidad_pistas_por_superficie, int cantidad_sectores_por_pista, int capacidad_bytes_por_sector) {
	Disco disco;
	disco.cantidad_platos = cantidad_platos;
	disco.platos = new Plato[cantidad_platos];

	for (int i = 0; i < cantidad_platos; ++i) {
		disco.platos[i].numero_plato = i;
		for (int j = 0; j < 2; ++j) {
			disco.platos[i].superficies[j].numero_superficie = j;
			disco.platos[i].superficies[j].cantidad_pistas = cantidad_pistas_por_superficie;
			disco.platos[i].superficies[j].pistas = new Pista[cantidad_pistas_por_superficie];
			for (int k = 0; k < cantidad_pistas_por_superficie; ++k) {
				disco.platos[i].superficies[j].pistas[k].numero_pista = k;
				disco.platos[i].superficies[j].pistas[k].cantidad_sectores = cantidad_sectores_por_pista;
				disco.platos[i].superficies[j].pistas[k].sectores = new Sector[cantidad_sectores_por_pista];
				for (int s = 0; s < cantidad_sectores_por_pista; ++s) {
					disco.platos[i].superficies[j].pistas[k].sectores[s].capacidad_bytes = capacidad_bytes_por_sector;
					disco.platos[i].superficies[j].pistas[k].sectores[s].datos = new char[capacidad_bytes_por_sector];
					disco.platos[i].superficies[j].pistas[k].sectores[s].libre = true;
					disco.platos[i].superficies[j].pistas[k].sectores[s].es_continuacion = false;
					disco.platos[i].superficies[j].pistas[k].sectores[s].direccion.posicion_plato = i;
					disco.platos[i].superficies[j].pistas[k].sectores[s].direccion.posicion_superficie = j;
					disco.platos[i].superficies[j].pistas[k].sectores[s].direccion.posicion_pista = k;
					disco.platos[i].superficies[j].pistas[k].sectores[s].direccion.posicion_sector = s;
					disco.platos[i].superficies[j].pistas[k].sectores[s].siguiente_sector = nullptr;
				}
			}
		}
	}
	return disco;
}

void liberar_disco(Disco& disco) {
	for (int i = 0; i < disco.cantidad_platos; ++i) {
		for (int j = 0; j < 2; ++j) {
			for (int k = 0; k < disco.platos[i].superficies[j].cantidad_pistas; ++k) {
				for (int s = 0; s < disco.platos[i].superficies[j].pistas[k].cantidad_sectores; ++s) {
					delete[] disco.platos[i].superficies[j].pistas[k].sectores[s].datos;
				}
				delete[] disco.platos[i].superficies[j].pistas[k].sectores;
			}
			delete[] disco.platos[i].superficies[j].pistas;
		}
	}
	delete[] disco.platos;
}

Sector* buscar_sector_libre(Disco& disco) {
	for (int i = 0; i < disco.cantidad_platos; ++i) {
		for (int j = 0; j < 2; ++j) {
			for (int k = 0; k < disco.platos[i].superficies[j].cantidad_pistas; ++k) {
				for (int s = 0; s < disco.platos[i].superficies[j].pistas[k].cantidad_sectores; ++s) {
					if (disco.platos[i].superficies[j].pistas[k].sectores[s].libre) {
						return &disco.platos[i].superficies[j].pistas[k].sectores[s];
					}
				}
			}
		}
	}
	return nullptr;
}

void escribir_registro(Disco& disco, string datos) {
	int capacidad_sector =
		disco.platos[0]
		.superficies[0]
		.pistas[0]
		.sectores[0]
		.capacidad_bytes;

	int sectores_necesarios = ((int)datos.size() + capacidad_sector - 1) / capacidad_sector;

	int offset = 0;
	Sector* sector_anterior = nullptr;

	while (offset < datos.size()) {

		Sector* sector_actual = buscar_sector_libre(disco);

		if (sector_actual == nullptr) {
			throw runtime_error("No hay sectores libres");
		}

		int bytes_a_copiar =
			min(capacidad_sector,
				static_cast<int>(datos.size() - offset));

		memcpy(
			sector_actual->datos,
			datos.c_str() + offset,
			bytes_a_copiar
		);

		sector_actual->libre = false;
		sector_actual->es_continuacion = (offset > 0);
		sector_actual->siguiente_sector = nullptr;

		if (sector_anterior != nullptr) {
			sector_anterior->siguiente_sector = sector_actual;
		}

		sector_anterior = sector_actual;
		offset += bytes_a_copiar;
	}
}

DireccionDisco calcular_direccion(
	int n,
	int num_platos,
	int pistas_por_sup,
	int sectores_por_pista)
{
	DireccionDisco dir = { -1, -1, -1, -1, false };
	if (num_platos <= 0 || pistas_por_sup <= 0 || sectores_por_pista <= 0)
		return dir;
	int sectores_por_sup = pistas_por_sup * sectores_por_pista;
	int sectores_por_plato = 2 * sectores_por_sup;
	int total_sectores = num_platos * sectores_por_plato;
	if (n < 0 || n >= total_sectores)
		return dir;
	int resto = n;
	dir.posicion_plato = resto / sectores_por_plato; resto %= sectores_por_plato;
	dir.posicion_superficie = resto / sectores_por_sup;   resto %= sectores_por_sup;
	dir.posicion_pista = resto / sectores_por_pista; resto %= sectores_por_pista;
	dir.posicion_sector = resto;
	dir.valida = true;
	return dir;
}