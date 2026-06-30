// =============================================================================
//  ArbolAVL.cpp
//  Implementacion del Arbol AVL (ver ArbolAVL.h).
//
//  Recordatorio AVL:
//  - Factor de balance de un nodo = altura(izq) - altura(der).
//  - Un nodo esta balanceado si su factor es -1, 0 o +1.
//  - Tras insertar, si algun nodo queda con factor +2 o -2, se corrige con una
//    rotacion (simple o doble). Hay 4 casos: Izq-Izq, Der-Der, Izq-Der, Der-Izq.
// =============================================================================
#include "ArbolAVL.h"
#include <algorithm>  // std::max

ArbolAVL::ArbolAVL() : raiz(nullptr), claves_distintas(0) {}

ArbolAVL::~ArbolAVL() {
    liberar_rec(raiz);
}

// ----------------------------- Helpers de altura -----------------------------

// Altura de un nodo (0 si es nulo).
int ArbolAVL::altura_nodo(Nodo* n) {
    return n ? n->altura : 0;
}

// Factor de balance = altura(izquierda) - altura(derecha).
int ArbolAVL::factor_balance(Nodo* n) {
    return n ? altura_nodo(n->izq) - altura_nodo(n->der) : 0;
}

// Recalcula la altura de un nodo a partir de la de sus hijos.
void ArbolAVL::recalcular_altura(Nodo* n) {
    if (n) n->altura = 1 + std::max(altura_nodo(n->izq), altura_nodo(n->der));
}

// ------------------------------- Rotaciones ---------------------------------
//
//      Rotacion a la DERECHA sobre y:           Rotacion a la IZQUIERDA sobre x:
//
//            y                 x                       x                 y
//           / \               / \                     / \               / \
//          x   T3    -->     T1  y                   T1  y     -->      x   T3
//         / \                   / \                     / \           / \
//        T1  T2                T2  T3                   T2  T3        T1  T2

ArbolAVL::Nodo* ArbolAVL::rotar_derecha(Nodo* y) {
    Nodo* x  = y->izq;
    Nodo* T2 = x->der;
    x->der = y;
    y->izq = T2;
    recalcular_altura(y);
    recalcular_altura(x);
    return x; // nueva raiz del subarbol
}

ArbolAVL::Nodo* ArbolAVL::rotar_izquierda(Nodo* x) {
    Nodo* y  = x->der;
    Nodo* T2 = y->izq;
    y->izq = x;
    x->der = T2;
    recalcular_altura(x);
    recalcular_altura(y);
    return y; // nueva raiz del subarbol
}

// Reequilibra un nodo segun su factor de balance (los 4 casos clasicos AVL).
ArbolAVL::Nodo* ArbolAVL::balancear(Nodo* n) {
    recalcular_altura(n);
    int balance = factor_balance(n);

    // Caso Izquierda-Izquierda.
    if (balance > 1 && factor_balance(n->izq) >= 0)
        return rotar_derecha(n);
    // Caso Izquierda-Derecha.
    if (balance > 1 && factor_balance(n->izq) < 0) {
        n->izq = rotar_izquierda(n->izq);
        return rotar_derecha(n);
    }
    // Caso Derecha-Derecha.
    if (balance < -1 && factor_balance(n->der) <= 0)
        return rotar_izquierda(n);
    // Caso Derecha-Izquierda.
    if (balance < -1 && factor_balance(n->der) > 0) {
        n->der = rotar_derecha(n->der);
        return rotar_izquierda(n);
    }
    return n; // ya estaba balanceado
}

// ------------------------------- Insercion ----------------------------------

ArbolAVL::Nodo* ArbolAVL::insertar_rec(Nodo* nodo, const Valor& clave, int id) {
    // Lugar vacio: se crea un nodo nuevo con la clave y su primer id.
    if (nodo == nullptr) {
        Nodo* nuevo = new Nodo;
        nuevo->clave  = clave;
        nuevo->ids.push_back(id);
        nuevo->izq = nuevo->der = nullptr;
        nuevo->altura = 1;
        claves_distintas++;
        return nuevo;
    }

    int cmp = comparar_valores(clave, nodo->clave);
    if (cmp < 0) {
        nodo->izq = insertar_rec(nodo->izq, clave, id);
    } else if (cmp > 0) {
        nodo->der = insertar_rec(nodo->der, clave, id);
    } else {
        // Clave ya existente: se agrega el id a la lista (no se crea nodo).
        nodo->ids.push_back(id);
        return nodo;
    }
    // Reequilibrar el camino de vuelta.
    return balancear(nodo);
}

void ArbolAVL::insertar(const Valor& clave, int id_registro) {
    raiz = insertar_rec(raiz, clave, id_registro);
}

// ----------------------------- Busquedas ------------------------------------

void ArbolAVL::buscar_elemento_rec(Nodo* nodo, const Valor& clave,
                                   std::vector<int>& salida) const {
    if (nodo == nullptr) return;
    int cmp = comparar_valores(clave, nodo->clave);
    if (cmp < 0)      buscar_elemento_rec(nodo->izq, clave, salida);
    else if (cmp > 0) buscar_elemento_rec(nodo->der, clave, salida);
    else              salida.insert(salida.end(), nodo->ids.begin(), nodo->ids.end());
}

std::vector<int> ArbolAVL::buscar_elemento(const Valor& clave) const {
    std::vector<int> salida;
    buscar_elemento_rec(raiz, clave, salida);
    return salida;
}

// Recorrido in-order con PODA: solo baja por las ramas que pueden contener
// claves dentro del rango, y visita las claves en orden ascendente.
void ArbolAVL::buscar_rango_rec(Nodo* nodo, const Valor& min, const Valor& max,
                                std::vector<int>& salida) const {
    if (nodo == nullptr) return;
    int cmp_min = comparar_valores(nodo->clave, min);
    int cmp_max = comparar_valores(nodo->clave, max);

    // Si la clave es mayor que min, puede haber resultados a la izquierda.
    if (cmp_min > 0) buscar_rango_rec(nodo->izq, min, max, salida);
    // La clave actual entra si esta dentro de [min, max].
    if (cmp_min >= 0 && cmp_max <= 0)
        salida.insert(salida.end(), nodo->ids.begin(), nodo->ids.end());
    // Si la clave es menor que max, puede haber resultados a la derecha.
    if (cmp_max < 0) buscar_rango_rec(nodo->der, min, max, salida);
}

std::vector<int> ArbolAVL::buscar_rango(const Valor& minimo, const Valor& maximo) const {
    std::vector<int> salida;
    buscar_rango_rec(raiz, minimo, maximo, salida);
    return salida;
}

// ----------------------------- Mantenimiento --------------------------------

void ArbolAVL::liberar_rec(Nodo* nodo) {
    if (nodo == nullptr) return;
    liberar_rec(nodo->izq);
    liberar_rec(nodo->der);
    delete nodo;
}

void ArbolAVL::limpiar() {
    liberar_rec(raiz);
    raiz = nullptr;
    claves_distintas = 0;
}

int ArbolAVL::altura() const { return altura_nodo(raiz); }

int ArbolAVL::cantidad_claves() const { return claves_distintas; }
