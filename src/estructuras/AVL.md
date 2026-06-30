# Explicación de la implementación del Árbol AVL

## Antes de empezar: cómo funciona `claves_distintas`

La variable:

```cpp
claves_distintas
```

cuenta cuántas **claves diferentes** hay en el árbol. No cuenta la cantidad total de `ids`.

En esta implementación, cada nodo guarda una clave y una lista de ids:

```cpp
clave -> ids
```

Por eso, si una misma clave aparece varias veces, se mantiene un solo nodo y se agregan los ids a su vector.

Ejemplo:

```cpp
insertar("Ana", 10);
insertar("Ana", 12);
insertar("Luis", 20);
```

El árbol tendría dos claves distintas:

```text
Ana  -> [10, 12]
Luis -> [20]
```

Entonces:

```text
claves_distintas = 2
cantidad total de ids = 3
```

La variable se inicializa en el constructor:

```cpp
ArbolAVL::ArbolAVL() : raiz(nullptr), claves_distintas(0) {}
```

Aumenta solamente cuando se crea un nodo nuevo:

```cpp
if (nodo == nullptr) {
    Nodo* nuevo = new Nodo;
    ...
    claves_distintas++;
    return nuevo;
}
```

Cuando la clave ya existe, no aumenta:

```cpp
else {
    nodo->ids.push_back(id);
    return nodo;
}
```

Y cuando se limpia el árbol, vuelve a cero:

```cpp
void ArbolAVL::limpiar() {
    liberar_rec(raiz);
    raiz = nullptr;
    claves_distintas = 0;
}
```

El método:

```cpp
int ArbolAVL::cantidad_claves() const { return claves_distintas; }
```

devuelve ese contador.

---

## 1. Qué representa esta implementación

Este árbol AVL funciona como un índice:

```text
clave -> lista de ids
```

Cada nodo contiene:

```cpp
clave
ids
izq
der
altura
```

La `clave` es el valor por el que se ordena el árbol.  
El vector `ids` almacena los registros asociados a esa clave.  
Los punteros `izq` y `der` apuntan a los hijos izquierdo y derecho.  
La `altura` se usa para calcular el balance del nodo.

Si se inserta una clave nueva, se crea un nodo.  
Si se inserta una clave repetida, se agrega el nuevo `id` al vector del nodo existente.

Ejemplo conceptual:

```text
          30 -> [4]
         /        \
  20 -> [1, 7]   40 -> [2]
```

En ese ejemplo hay tres claves distintas:

```text
20, 30, 40
```

Pero hay cuatro ids almacenados:

```text
1, 7, 4, 2
```

---

## 2. Constructor y destructor

El constructor deja el árbol vacío:

```cpp
ArbolAVL::ArbolAVL() : raiz(nullptr), claves_distintas(0) {}
```

Inicializa:

```text
raiz = nullptr
claves_distintas = 0
```

El destructor libera la memoria ocupada por los nodos:

```cpp
ArbolAVL::~ArbolAVL() {
    liberar_rec(raiz);
}
```

Como los nodos fueron creados con `new`, deben eliminarse con `delete`.  
Esa tarea se delega a la función recursiva `liberar_rec`.

---

## 3. Helpers de altura y balance

La función:

```cpp
int ArbolAVL::altura_nodo(Nodo* n) {
    return n ? n->altura : 0;
}
```

devuelve la altura del nodo. Si el nodo es `nullptr`, devuelve `0`.

Con esta convención:

```text
nodo nulo -> altura 0
hoja      -> altura 1
```

La función:

```cpp
int ArbolAVL::factor_balance(Nodo* n) {
    return n ? altura_nodo(n->izq) - altura_nodo(n->der) : 0;
}
```

calcula el factor de balance:

```text
factor = altura del hijo izquierdo - altura del hijo derecho
```

Un nodo se considera balanceado cuando su factor es:

```text
-1, 0 o 1
```

La función:

```cpp
void ArbolAVL::recalcular_altura(Nodo* n) {
    if (n) n->altura = 1 + std::max(altura_nodo(n->izq), altura_nodo(n->der));
}
```

actualiza la altura del nodo usando la altura de sus hijos.

Ejemplo:

```text
        30
       /
     20
    /
  10
```

Alturas:

```text
10 -> 1
20 -> 2
30 -> 3
```

El factor de balance de `30` sería:

```text
altura izquierda = 2
altura derecha   = 0
factor           = 2
```

Como `2` está fuera del rango permitido, ese nodo necesita una rotación.

---

## 4. Rotaciones

Las rotaciones modifican los enlaces entre nodos para recuperar el balance del árbol sin romper el orden de búsqueda.

### Rotación a la derecha

```cpp
ArbolAVL::Nodo* ArbolAVL::rotar_derecha(Nodo* y) {
    Nodo* x  = y->izq;
    Nodo* T2 = x->der;

    x->der = y;
    y->izq = T2;

    recalcular_altura(y);
    recalcular_altura(x);

    return x;
}
```

Antes:

```text
        y
       /
      x
       \
        T2
```

Después:

```text
      x
       \
        y
       /
      T2
```

Pasos principales:

```text
1. x sube y se convierte en la nueva raíz del subárbol.
2. y baja hacia la derecha de x.
3. T2 pasa a ser hijo izquierdo de y.
4. Se recalculan las alturas de y y x.
5. Se retorna x como nueva raíz del subárbol.
```

---

### Rotación a la izquierda

```cpp
ArbolAVL::Nodo* ArbolAVL::rotar_izquierda(Nodo* x) {
    Nodo* y  = x->der;
    Nodo* T2 = y->izq;

    y->izq = x;
    x->der = T2;

    recalcular_altura(x);
    recalcular_altura(y);

    return y;
}
```

Antes:

```text
    x
     \
      y
     /
    T2
```

Después:

```text
      y
     /
    x
     \
      T2
```

Pasos principales:

```text
1. y sube y se convierte en la nueva raíz del subárbol.
2. x baja hacia la izquierda de y.
3. T2 pasa a ser hijo derecho de x.
4. Se recalculan las alturas de x y y.
5. Se retorna y como nueva raíz del subárbol.
```

---

## 5. Función `balancear`

La función `balancear` revisa un nodo después de una inserción.

```cpp
ArbolAVL::Nodo* ArbolAVL::balancear(Nodo* n) {
    recalcular_altura(n);
    int balance = factor_balance(n);

    if (balance > 1 && factor_balance(n->izq) >= 0)
        return rotar_derecha(n);

    if (balance > 1 && factor_balance(n->izq) < 0) {
        n->izq = rotar_izquierda(n->izq);
        return rotar_derecha(n);
    }

    if (balance < -1 && factor_balance(n->der) <= 0)
        return rotar_izquierda(n);

    if (balance < -1 && factor_balance(n->der) > 0) {
        n->der = rotar_derecha(n->der);
        return rotar_izquierda(n);
    }

    return n;
}
```

Primero recalcula la altura:

```cpp
recalcular_altura(n);
```

Luego calcula el balance:

```cpp
int balance = factor_balance(n);
```

Después analiza los cuatro casos AVL.

---

### Caso Izquierda-Izquierda

```cpp
if (balance > 1 && factor_balance(n->izq) >= 0)
    return rotar_derecha(n);
```

Forma del desbalance:

```text
        n
       /
    n->izq
     /
```

Se aplica una rotación a la derecha sobre `n`.

---

### Caso Izquierda-Derecha

```cpp
if (balance > 1 && factor_balance(n->izq) < 0) {
    n->izq = rotar_izquierda(n->izq);
    return rotar_derecha(n);
}
```

Forma del desbalance:

```text
        n
       /
    n->izq
       \
```

Se aplican dos rotaciones:

```text
1. Rotación a la izquierda sobre n->izq.
2. Rotación a la derecha sobre n.
```

---

### Caso Derecha-Derecha

```cpp
if (balance < -1 && factor_balance(n->der) <= 0)
    return rotar_izquierda(n);
```

Forma del desbalance:

```text
    n
     \
    n->der
       \
```

Se aplica una rotación a la izquierda sobre `n`.

---

### Caso Derecha-Izquierda

```cpp
if (balance < -1 && factor_balance(n->der) > 0) {
    n->der = rotar_derecha(n->der);
    return rotar_izquierda(n);
}
```

Forma del desbalance:

```text
    n
     \
    n->der
     /
```

Se aplican dos rotaciones:

```text
1. Rotación a la derecha sobre n->der.
2. Rotación a la izquierda sobre n.
```

Si el nodo no necesita rotación, se retorna el mismo nodo:

```cpp
return n;
```

---

## 6. Inserción

La inserción se realiza con la función recursiva:

```cpp
ArbolAVL::Nodo* ArbolAVL::insertar_rec(Nodo* nodo, const Valor& clave, int id)
```

### Cuando el lugar está vacío

```cpp
if (nodo == nullptr) {
    Nodo* nuevo = new Nodo;
    nuevo->clave  = clave;
    nuevo->ids.push_back(id);
    nuevo->izq = nuevo->der = nullptr;
    nuevo->altura = 1;
    claves_distintas++;
    return nuevo;
}
```

Aquí se crea un nodo nuevo.  
Ese nodo empieza con altura `1` porque es una hoja.

También se incrementa:

```cpp
claves_distintas++;
```

porque acaba de aparecer una clave que antes no estaba en el árbol.

---

### Cuando la clave es menor

```cpp
if (cmp < 0) {
    nodo->izq = insertar_rec(nodo->izq, clave, id);
}
```

Si la clave nueva es menor que la clave del nodo actual, la inserción continúa por el subárbol izquierdo.

---

### Cuando la clave es mayor

```cpp
else if (cmp > 0) {
    nodo->der = insertar_rec(nodo->der, clave, id);
}
```

Si la clave nueva es mayor que la clave del nodo actual, la inserción continúa por el subárbol derecho.

---

### Cuando la clave ya existe

```cpp
else {
    nodo->ids.push_back(id);
    return nodo;
}
```

No se crea otro nodo.  
El nuevo `id` se agrega a la lista de ids de esa clave.

En este caso no cambia la forma del árbol, por eso se retorna el mismo nodo.

---

### Reequilibrio al volver de la recursión

```cpp
return balancear(nodo);
```

Después de insertar en el subárbol izquierdo o derecho, la función vuelve hacia arriba.  
En ese regreso, cada nodo del camino se revisa y se balancea si es necesario.

La función pública:

```cpp
void ArbolAVL::insertar(const Valor& clave, int id_registro) {
    raiz = insertar_rec(raiz, clave, id_registro);
}
```

actualiza `raiz` con el resultado de la inserción, porque una rotación puede cambiar la raíz del árbol o de un subárbol.

---

## 7. Búsqueda exacta

La búsqueda exacta se hace con:

```cpp
void ArbolAVL::buscar_elemento_rec(Nodo* nodo, const Valor& clave,
                                   std::vector<int>& salida) const
```

Si el nodo es nulo, termina:

```cpp
if (nodo == nullptr) return;
```

Luego compara la clave buscada con la clave del nodo actual:

```cpp
int cmp = comparar_valores(clave, nodo->clave);
```

Si la clave buscada es menor, baja por la izquierda:

```cpp
if (cmp < 0)
    buscar_elemento_rec(nodo->izq, clave, salida);
```

Si la clave buscada es mayor, baja por la derecha:

```cpp
else if (cmp > 0)
    buscar_elemento_rec(nodo->der, clave, salida);
```

Si la clave coincide, copia todos los ids asociados:

```cpp
else
    salida.insert(salida.end(), nodo->ids.begin(), nodo->ids.end());
```

La función pública crea el vector de salida y llama a la función recursiva:

```cpp
std::vector<int> ArbolAVL::buscar_elemento(const Valor& clave) const {
    std::vector<int> salida;
    buscar_elemento_rec(raiz, clave, salida);
    return salida;
}
```

Si la clave no existe, el vector se devuelve vacío.

---

## 8. Búsqueda por rango

La búsqueda por rango usa un recorrido in-order con poda:

```cpp
void ArbolAVL::buscar_rango_rec(Nodo* nodo, const Valor& min, const Valor& max,
                                std::vector<int>& salida) const
```

El rango es inclusivo:

```text
[min, max]
```

Primero compara la clave del nodo con el mínimo y el máximo:

```cpp
int cmp_min = comparar_valores(nodo->clave, min);
int cmp_max = comparar_valores(nodo->clave, max);
```

---

### Revisar el subárbol izquierdo

```cpp
if (cmp_min > 0)
    buscar_rango_rec(nodo->izq, min, max, salida);
```

Si la clave actual es mayor que `min`, entonces en el subárbol izquierdo podría haber claves dentro del rango.

Si la clave actual es menor o igual que `min`, no hace falta bajar por la izquierda, porque ahí habrá claves aún menores.

---

### Agregar la clave actual

```cpp
if (cmp_min >= 0 && cmp_max <= 0)
    salida.insert(salida.end(), nodo->ids.begin(), nodo->ids.end());
```

La clave actual se agrega si cumple:

```text
clave >= min
clave <= max
```

Como cada clave puede tener varios ids, se agregan todos los ids del nodo.

---

### Revisar el subárbol derecho

```cpp
if (cmp_max < 0)
    buscar_rango_rec(nodo->der, min, max, salida);
```

Si la clave actual es menor que `max`, entonces en el subárbol derecho podría haber claves dentro del rango.

Si la clave actual es mayor o igual que `max`, no hace falta bajar por la derecha, porque ahí habrá claves aún mayores.

---

### Función pública

```cpp
std::vector<int> ArbolAVL::buscar_rango(const Valor& minimo, const Valor& maximo) const {
    std::vector<int> salida;
    buscar_rango_rec(raiz, minimo, maximo, salida);
    return salida;
}
```

Crea el vector de salida, ejecuta la búsqueda recursiva y devuelve los ids encontrados.

La salida queda ordenada por clave porque el recorrido respeta el orden:

```text
izquierda -> nodo actual -> derecha
```

---

## 9. Mantenimiento y liberación de memoria

La función:

```cpp
void ArbolAVL::liberar_rec(Nodo* nodo) {
    if (nodo == nullptr) return;
    liberar_rec(nodo->izq);
    liberar_rec(nodo->der);
    delete nodo;
}
```

recorre el árbol en post-order:

```text
1. Libera el subárbol izquierdo.
2. Libera el subárbol derecho.
3. Libera el nodo actual.
```

Se usa post-order porque primero deben eliminarse los hijos y luego el nodo padre.

La función:

```cpp
void ArbolAVL::limpiar() {
    liberar_rec(raiz);
    raiz = nullptr;
    claves_distintas = 0;
}
```

elimina todos los nodos y deja el árbol en estado vacío.

Después de llamar a `limpiar`:

```text
raiz = nullptr
claves_distintas = 0
```

El método:

```cpp
int ArbolAVL::altura() const { return altura_nodo(raiz); }
```

devuelve la altura de la raíz.  
Como cada nodo guarda su altura, esta consulta toma tiempo constante.

El método:

```cpp
int ArbolAVL::cantidad_claves() const { return claves_distintas; }
```

devuelve la cantidad de claves distintas almacenadas en el árbol.
