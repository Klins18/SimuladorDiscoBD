#pragma once
struct DireccionDisco {
	int posicion_plato;
	int posicion_superficie;
	int posicion_pista;
	int posicion_sector;
	bool valida;
};

struct Sector{
	int capacidad_bytes;
	char* datos;
	bool libre;
	bool es_continuacion;
	DireccionDisco direccion;
	Sector* siguiente_sector;
};

struct Pista {
	int numero_pista;
	int cantidad_sectores;
	Sector* sectores;
};

struct Superficie {
	int numero_superficie;
	int cantidad_pistas;
	Pista* pistas;
};

struct Plato {
	int numero_plato;
	Superficie superficies[2];
};

struct Disco {
	int cantidad_platos;
	Plato* platos;
};

