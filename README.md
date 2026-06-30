# Simulador de Almacenamiento Físico de Base de Datos
---
## Índice

1. [Estructura](#1-estructura)
2. [Conceptos del disco (glosario)](#2-conceptos-del-disco-glosario)
3. [Cómo se guarda un registro](#3-cómo-se-guarda-un-registro)
4. [La dirección física (P, S, Pi, Se)](#4-la-dirección-física-p-s-pi-se)
5. [Cómo se busca (el Árbol AVL)](#5-cómo-se-busca-el-árbol-avl)
6. [Recorrido del código (función por función)](#6-recorrido-del-código-función-por-función)
7. [Cómo compilar y ejecutar](#8-cómo-compilar-y-ejecutar)
8. [Formato de los archivos](#10-formato-de-los-archivos)

---

## 1. Estructura

El proyecto tiene **cuatro partes**, cada una en su carpeta. La mejor forma de entenderlo
es saber qué hace cada una:

```
   src/disco/        ┌─────────────────────────────────────────────┐
   "el hardware"     │  Simula la geometría física del disco:      │
                     │  platos, superficies, pistas y sectores.    │
                     │  Aquí VIVEN los datos (en bytes).           │
                     └─────────────────────────────────────────────┘
   src/modelo/       ┌─────────────────────────────────────────────┐
   "la base de datos"│  La tabla: qué columnas tiene y de qué tipo,│
                     │  cómo se convierte un dato en bytes, cómo se│
                     │  cargan (CSV / CREATE TABLE) y se consultan.│
                     └─────────────────────────────────────────────┘
   src/estructuras/  ┌─────────────────────────────────────────────┐
   "cómo buscar"     │  El Árbol AVL: un índice para encontrar     │
                     │  registros rápido, detrás de una interfaz.  │
                     └─────────────────────────────────────────────┘
   src/interfaz/     ┌─────────────────────────────────────────────┐
   "la pantalla"     │  Las ventanas, botones y tablas (SFML+ImGui)│
                     └─────────────────────────────────────────────┘
```

Resumen mental: **`disco` = dónde se guarda**, **`modelo` = qué se guarda**,
**`estructuras` = con qué se busca rápido**, **`interfaz` = cómo lo usas**.

---

## 2. Conceptos del disco (glosario)

Un disco duro es una pila de "platos" que giran. De lo más grande a lo más pequeño:

```
DISCO  (todo el dispositivo)
  ├── PLATO 0, PLATO 1, ...        (varios platos apilados, como CDs)
  │     ├── SUPERFICIE 0 y 1        (las 2 caras de cada plato)
  │     │     ├── PISTA 0, 1, ...   (anillos, como los de un vinilo)
  │     │     │     └── SECTOR 0, 1, ...  (trozos de la pista) ← unidad mínima
```

| Término        | Qué es                                                                          |
| -------------- | ------------------------------------------------------------------------------- |
| **Disco**      | Todo el dispositivo. Contiene varios platos.                                    |
| **Plato**      | Un disco físico (como un CD). El disco duro apila varios.                       |
| **Superficie** | Cada cara de un plato. **Cada plato tiene 2 superficies** (arriba y abajo).     |
| **Pista**      | Un anillo dentro de una superficie. Cada superficie tiene muchas pistas.        |
| **Sector**     | Un trozo de una pista. Es la **unidad mínima** de lectura/escritura (en bytes). |
| **Registro**   | Una fila de la tabla (p. ej. `Lima, 20, Ingeniería, Perú`).                     |
| **Byte**       | La unidad de información. Un sector "de 16 bytes" guarda 16 bytes de datos.     |

Estas piezas son las **estructuras** de `src/disco/Disco.h`:

| Estructura       | Qué contiene                                                                                                 |
| ---------------- | ------------------------------------------------------------------------------------------------------------ |
| `DireccionDisco` | Una ubicación: `posicion_plato`, `posicion_superficie`, `posicion_pista`, `posicion_sector` + `valida`.      |
| `Sector`         | `capacidad_bytes`, el buffer `char* datos`, `libre`, `es_continuacion`, su `direccion` y `siguiente_sector`. |
| `Pista`          | Un arreglo de `Sector`.                                                                                      |
| `Superficie`     | Un arreglo de `Pista`.                                                                                       |
| `Plato`          | Dos `Superficie` (caras 0 y 1).                                                                              |
| `Disco`          | Un arreglo de `Plato`.                                                                                       |

---

## 3. Cómo se guarda un registro

1. Defines la **estructura** de la tabla (el **esquema**): qué columnas tiene y de qué
   **tipo** es cada una. Los tipos llevan los **mismos nombres que PostgreSQL**:

   | Tipo (`TipoDato`)   | Qué guarda                       | Bytes en disco |
   |---------------------|----------------------------------|----------------|
   | `INTEGER`           | un número entero                 | 4              |
   | `DOUBLE PRECISION`  | un número con decimales (double) | 8              |
   | `VARCHAR(n)`        | texto de hasta `n` caracteres    | `n`            |
   | `BOOLEAN`           | verdadero / falso                | 1              |

2. Cada **registro** se convierte en **bytes** según el esquema. La suma de los bytes de
   todas las columnas es el **tamaño del registro**. Cada campo se guarda en **binario**:
   el número `20` ocupa 4 bytes (`20,0,0,0`) -> 00010100 00000000 00000000 00000000
    , **no** el texto `"20"`.

3. El programa busca **sectores libres** y guarda ahí los bytes. Si el registro no cabe en
   un sector, **se reparte en varios sectores encadenados** (cada uno apunta al siguiente):

```
Registro de 26 bytes, sectores de 16 bytes:

   ┌─ Sector A (16 bytes) ─┐   ┌─ Sector B (16 bytes) ─┐
   │ Lima........20.Ingen  │ → │ ieria..Peru.........  │   (A apunta a B)
   └───────────────────────┘   └───────────────────────┘
        ↑ primer sector             ↑ continuación
```

### ¿Cuántos registros caben? (capacidad y geometría)

```
total_sectores        = platos × 2 × pistas × sectores_por_pista
sectores_por_registro = techo( tamaño_registro / capacidad_sector )
registros_que_caben  ≈ total_sectores / sectores_por_registro
```

**Ejemplo** (registro de 69 bytes; disco 2×3×4 = 48 sectores):

| capacidad del sector | sectores por registro | registros que caben |
|:--------------------:|:---------------------:|:-------------------:|
| 16 | techo(69/16) = 5 | 48/5 = **9** |
| 8  | techo(69/8) = 9  | 48/9 = **5** |
| 69 | 1 | **48** |

> **Si bajas la capacidad**, cada registro se fragmenta en más sectores y **caben menos
> registros**. Si se acaban los sectores, al insertar aparece *"No hay sectores libres"*.

---

## 4. La dirección física (P, S, Pi, Se)

En la interfaz, cada registro muestra `Dir (P, S, Pi, Se)` con cuatro números. Son la
**ubicación del primer sector del registro**:

| Sigla  | Significa      | Indica                                          |
|:------:|----------------|-------------------------------------------------|
| **P**  | **P**lato      | En qué plato está.                              |
| **S**  | **S**uperficie | En qué cara (0 = arriba, 1 = abajo).            |
| **Pi** | **Pi**sta      | En qué pista (anillo).                          |
| **Se** | **Se**ctor     | En qué sector de esa pista.                     |

> `1, 0, 2, 3` se lee *"Plato 1, Superficie 0, Pista 2, Sector 3"*.

Es **única** por registro. Los sectores se llenan **en orden** (una pista entera → la
siguiente pista → la otra superficie → el siguiente plato), nunca al azar.

---

## 5. Cómo se busca (el Árbol AVL)

Para buscar no se recorre la tabla fila por fila: primero se **organizan** los valores de
la columna consultada en un **Árbol AVL** (un árbol de búsqueda que se mantiene
equilibrado) y se busca sobre él. Permite:

- **Búsqueda por elemento:** registros donde la columna es **igual** a un valor.
- **Búsqueda por rango:** registros donde la columna está **entre** un mínimo y un máximo.

> 📄 **La explicación completa del AVL —nodos, rotaciones, inserción, búsquedas y cada
> función— está en [`src/estructuras/AVL.md`](src/estructuras/AVL.md).** 

### El disco GUARDA, el AVL solo INDEXA

Esta es la duda más común. **Los datos no se guardan en el AVL.**

- Los **datos** viven en los **sectores del disco** (sección 3). Es lo permanente.
- El **catálogo** (`std::vector<EntradaCatalogo>`) es una lista que dice, por registro: su
  **id**, su **dirección física** y un **puntero a su primer sector**.
- El **AVL** es solo un **índice**: pares `valor de columna → id(s)`. No copia los datos,
  solo apunta a ellos.

Cada actor tiene un solo trabajo: el **AVL** dice **quiénes** coinciden (los ids), el
**catálogo** dice **dónde** están, y el **disco** da **qué** son (los datos). Por eso una
búsqueda pasa por los tres en cadena.

**Ejemplo `edad = 20`** (con los datos de prueba, la edad 20 la tienen el id 0 = *Lima* y
el id 6 = *Caracas*):

```
  Tú escribes:  "edad = 20"
       │
       ▼
  1) AVL  (índice por edad)
        buscar_elemento(20)  →  ids = [0, 6]      ← solo IDs, no los datos
       │   (por cada id...)
       ▼
  2) CATÁLOGO  (id → ubicación)
        catalogo[0] → dirección (0,0,0,0) + 1er sector
        catalogo[6] → dirección (1,0,1,2) + 1er sector
       │
       ▼
  3) DISCO  (los bytes reales)
        leer_registro  → sigue la cadena de sectores
        deserializar   → bytes → ("Lima", 20, ...)
       │
       ▼
   Resultados:  Lima/20/...   y   Caracas/20/...
```

En el código (`buscar_por_elemento`, en `Consultas.cpp`):

```cpp
indexar_columna(estructura, esquema, catalogo, indice_columna); // 1) arma el AVL
for (int id : estructura.buscar_elemento(objetivo))             // 2) AVL → ids
    resultados.push_back(armar_resultado(esquema, catalogo, id)); // 3-4) id → catálogo → disco
```

La búsqueda por rango es igual, pero el paso 2 usa `buscar_rango(min, max)`.

> La geometría (platos/pistas/sectores/capacidad) **no afecta al AVL**: el AVL solo
> trabaja con `(valor, id)`. La geometría solo decide **dónde** quedan los bytes.

---

## 6. Recorrido del código (función por función)

### `src/disco/` — la geometría del disco

Archivo `Disco_funciones.h` / `Disco.cpp`:

| Función | Qué hace |
|---|---|
| `inicializar_disco(platos, pistas, sectores, capacidad)` | Crea el disco: reserva memoria para todos los platos/superficies/pistas/sectores y marca cada sector como libre con su dirección. Devuelve el `Disco`. |
| `liberar_disco(disco)` | Libera toda la memoria reservada por el disco. |
| `buscar_sector_libre(disco)` | Recorre la geometría **en orden** y devuelve el primer `Sector` libre (o `nullptr` si no hay). |
| `escribir_registro(disco, bytes)` | Escribe un registro (bytes binarios) ocupando uno o varios sectores **encadenados**. Devuelve la `DireccionDisco` del primer sector (= dirección única del registro). |
| `leer_registro(primer_sector, tamaño)` | Sigue la cadena `siguiente_sector` y reúne `tamaño` bytes del registro. |
| `sector_en_direccion(disco, dir)` | Devuelve el `Sector*` que está en una dirección física dada. |
| `calcular_direccion(n, ...)` | Convierte un número de sector lineal `n` en su dirección `(P,S,Pi,Se)`. |
| `direccion_a_numero(dir, ...)` | Operación inversa: de una dirección física al número de sector lineal. |

### `src/modelo/Esquema.*` — tipos y conversión a bytes

Define `TipoDato` (los tipos PostgreSQL), y las estructuras `Columna`, `Esquema` y `Valor`
(un valor tipado: usa `entero`, `flotante`, `cadena` o `booleano` según su `tipo`).

| Función | Qué hace |
|---|---|
| `Esquema::tamanio_registro()` | Suma los bytes de todas las columnas → tamaño de un registro. |
| `tamanio_de_tipo(tipo, n)` | Bytes de un tipo (`INTEGER`=4, `DOUBLE`=8, `BOOLEAN`=1, `VARCHAR`=`n`). |
| `serializar_registro(esquema, valores)` | Convierte los valores tipados en un buffer de **bytes binarios** (escribe cada campo según su tipo). |
| `deserializar_registro(esquema, bytes)` | Operación inversa: reconstruye los `Valor` desde los bytes. |
| `valor_a_texto(valor)` | Pasa un valor a texto **solo para mostrarlo** (no para guardar). |
| `comparar_valores(a, b)` | Compara dos valores del mismo tipo (`<0`, `0`, `>0`). La usan las consultas y el AVL. |

### `src/modelo/Datos.*` — cargar datos y esquema

| Función | Qué hace |
|---|---|
| `linea_a_valores(esquema, linea)` | Convierte una línea CSV (`"Lima,20,..."`) en valores tipados según el esquema. |
| `insertar_registro(esquema, valores, disco, catalogo)` | Serializa → `escribir_registro` → agrega la entrada al catálogo. Devuelve la dirección. |
| `importar_csv(ruta, esquema, disco, catalogo, mensaje)` | Lee el archivo línea por línea e inserta cada registro. Devuelve cuántos importó. |
| `leer_create_table(ruta, esquema, mensaje)` | Lee un archivo `CREATE TABLE (.txt)` y construye el esquema (reconoce los tipos de PostgreSQL y sus sinónimos). |

Funciones internas de apoyo: `abrir_archivo` (busca el archivo en varias rutas, así
funciona desde cualquier carpeta), `separar_por_comas`, `interpretar_tipo`,
`a_mayusculas`, `recortar`.

### `src/modelo/Consultas.*` — las búsquedas

Define `EntradaCatalogo` (id + dirección + primer sector) y `ResultadoBusqueda` (la
entrada + los valores ya leídos).

| Función | Qué hace |
|---|---|
| `buscar_por_elemento(estructura, esquema, catalogo, columna, objetivo)` | Indexa la columna en la `estructura` (el AVL) y devuelve los registros **iguales** a `objetivo`. |
| `buscar_por_rango(estructura, esquema, catalogo, columna, min, max)` | Igual, pero devuelve los registros dentro de `[min, max]`. |

Internas: `indexar_columna` (recorre el catálogo y llena la estructura con `valor → id`)
y `armar_resultado` (lee un registro del disco a partir de su id).

> Fíjate que ambas reciben un `EstructuraDatos&`: **no dependen del AVL en concreto**.

### `src/estructuras/` — la estructura de datos

`EstructuraDatos.h` es la **interfaz** (un contrato). Cualquier estructura de índice debe
ofrecer estos métodos:

| Método | Qué hace |
|---|---|
| `insertar(clave, id)` | Asocia un valor de columna con el id de un registro. |
| `buscar_elemento(clave)` | Devuelve los ids cuya clave es igual. |
| `buscar_rango(min, max)` | Devuelve los ids con clave en `[min, max]`. |
| `limpiar()` | Vacía la estructura. |
| `nombre()` | Nombre de la estructura (p. ej. `"Arbol AVL"`). |

`ArbolAVL` **implementa** esa interfaz. Sus métodos públicos son los de arriba más
`altura()` y `cantidad_claves()` (diagnóstico). Por dentro usa funciones recursivas
(`insertar_rec`, `buscar_elemento_rec`, `buscar_rango_rec`, `liberar_rec`) y de balanceo
(`rotar_izquierda`, `rotar_derecha`, `balancear`, etc.).

> 📄 **Todo el AVL está explicado función por función en
> [`src/estructuras/AVL.md`](src/estructuras/AVL.md).**

**Lo importante del diseño:** como las consultas dependen de la interfaz `EstructuraDatos`
y no del AVL, **cambiar de estructura** (a una tabla hash, un Árbol B, etc.) solo requiere
crear otra clase que herede de `EstructuraDatos` y cambiar **una línea** en la interfaz
(`ArbolAVL indice;`).

### `src/interfaz/SimuladorDiscoBD.cpp` — la pantalla

Es el `main` y todo lo visual. Funciones de apoyo:

| Función | Qué hace |
|---|---|
| `bytes_por_defecto(tipo, bytes_varchar)` | Bytes por defecto de un tipo en el formulario. |
| `elegir_archivo(titulo)` | Abre el **Finder nativo** (vía `osascript`) y devuelve la ruta elegida. |
| `boton_explorar(id, titulo, destino, tam)` | Dibuja el botón `...` que usa `elegir_archivo`. |
| `tabla_registros(id, esquema, filas)` | Dibuja una tabla ImGui con los registros y su dirección física. |
| `main()` | Crea la ventana, mantiene el estado (disco, esquema, catálogo, AVL) y dibuja los paneles en cada frame. Dentro tiene los *lambdas* `vaciar_tabla` (reinicia disco + catálogo), `a_valor` (texto → valor tipado) y `leer_catalogo` (lee todos los registros para mostrarlos). |

---

## 7. Cómo compilar y ejecutar

Necesitas: un compilador de C++ (clang++ o g++)

```sh
cd /Users/exponentiadev/Downloads/Disk
cmake -B build        # descarga dependencias y prepara la compilación
cmake --build build   # compila (la PRIMERA vez tarda varios minutos)
./build/SimuladorDiscoBD
```

---

## 8. Formato de los archivos

**Esquema — `CREATE TABLE` (.txt)** (ejemplo en [`datos/esquema_ejemplo.txt`](datos/esquema_ejemplo.txt)):

```sql
CREATE TABLE estudiantes (
    ciudad   VARCHAR(20),
    edad     INTEGER,
    carrera  VARCHAR(30),
    pais     VARCHAR(15)
);
```

**Datos — CSV** (ejemplo en [`datos/datos_ejemplo.csv`](datos/datos_ejemplo.csv)). Una
línea por registro, campos separados por comas, **en el mismo orden** que las columnas:

```
Lima,20,Ciencias de la computacion,Peru
Quito,22,Medicina,Ecuador
Bogota,19,Derecho,Colombia
```

> El `.txt` y el `.csv` de ejemplo son **consistentes** entre sí (mismas 4 columnas, mismo
> orden). Para `BOOLEAN` se aceptan `true`/`false`, `1`/`0`, `si`/`no`, etc.
