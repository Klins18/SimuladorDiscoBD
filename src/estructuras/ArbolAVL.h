#pragma once
// =============================================================================
//  ArbolAVL.h
//  Implementacion de un ARBOL AVL como estructura de indice de busqueda.
//
//  Un Arbol AVL es un arbol binario de busqueda AUTO-BALANCEADO: tras cada
//  insercion se reequilibra con rotaciones para que su altura se mantenga en
//  O(log n). Asi la busqueda por elemento es O(log n) y la busqueda por rango
//  es eficiente (recorrido in-order con poda).
//
//  Implementa la interfaz EstructuraDatos: cada nodo guarda una CLAVE (Valor
//  tipado) y la lista de IDs de registros que comparten esa clave (duplicados).
// =============================================================================
#include "EstructuraDatos.h"
#include "Esquema.h"
#include <vector>

class ArbolAVL : public EstructuraDatos {
public:
    ArbolAVL();
    ~ArbolAVL() override;

    // --- Metodos de la interfaz EstructuraDatos ---
    void insertar(const Valor& clave, int id_registro) override;
    std::vector<int> buscar_elemento(const Valor& clave) const override;
    std::vector<int> buscar_rango(const Valor& minimo, const Valor& maximo) const override;
    void limpiar() override;
    const char* nombre() const override { return "Arbol AVL"; }

    // --- Utilidades (diagnostico / pruebas) ---
    int altura() const;            // altura del arbol (0 si esta vacio)
    int cantidad_claves() const;   // numero de claves distintas almacenadas

private:
    // Nodo del arbol: una clave y todos los registros que la tienen.
    struct Nodo {
        Valor            clave;
        std::vector<int> ids;    // ids de registros con esta misma clave
        Nodo*            izq;
        Nodo*            der;
        int              altura; // altura del subarbol (para el balanceo AVL)
    };

    Nodo* raiz;
    int   claves_distintas;

    // Helpers de balanceo AVL.
    static int   altura_nodo(Nodo* n);
    static int   factor_balance(Nodo* n);
    static void  recalcular_altura(Nodo* n);
    static Nodo* rotar_derecha(Nodo* y);
    static Nodo* rotar_izquierda(Nodo* x);
    static Nodo* balancear(Nodo* n);

    // Recursiones internas.
    Nodo* insertar_rec(Nodo* nodo, const Valor& clave, int id);
    void  buscar_elemento_rec(Nodo* nodo, const Valor& clave, std::vector<int>& salida) const;
    void  buscar_rango_rec(Nodo* nodo, const Valor& min, const Valor& max, std::vector<int>& salida) const;
    void  liberar_rec(Nodo* nodo);
};
