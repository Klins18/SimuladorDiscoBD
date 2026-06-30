#pragma once
// =============================================================================
//  Esquema.h
//  Modelo de datos TIPADO de la tabla relacional.
//
//  Objetivo de este modulo:
//  Resolver la observacion de la revision -> "se esta casteando todo a string".
//  En vez de aplanar el registro completo a una cadena de texto, aqui cada
//  campo conserva su TIPO y se almacena en su representacion BINARIA.
//
//  Los tipos usan los MISMOS nombres que PostgreSQL:
//      - INTEGER          -> 4 bytes (entero de 32 bits)
//      - DOUBLE PRECISION -> 8 bytes (double)
//      - VARCHAR(n)       -> N bytes fijos definidos por el usuario
//      - BOOLEAN          -> 1 byte (0 = falso, 1 = verdadero)
//
//  Asi, por ejemplo, el numero 20 se guarda como 4 bytes binarios (0x14 00 00 00)
//  y NO como los caracteres '2','0'.
// =============================================================================
#include <string>
#include <vector>

// Tipos de dato soportados por una columna (nombres iguales a los de PostgreSQL).
enum class TipoDato {
    INTEGER,           // entero            -> 4 bytes binarios
    DOUBLE_PRECISION,  // decimal (double)  -> 8 bytes binarios
    VARCHAR,           // texto             -> N bytes fijos (definido por el usuario)
    BOOLEAN            // booleano (si/no)  -> 1 byte (0 = falso, 1 = verdadero)
};

// Cantidad de bytes fijos para los tipos de tamanio constante.
const int BYTES_INTEGER = 4;   // tamanio binario de un INTEGER
const int BYTES_DOUBLE = 8;    // tamanio binario de un DOUBLE PRECISION (double)
const int BYTES_BOOLEAN = 1;   // tamanio binario de un BOOLEAN

// Una columna de la tabla relacional.
struct Columna {
    std::string nombre;        // nombre de la columna (p. ej. "edad")
    TipoDato    tipo;          // tipo de dato de la columna
    int         tamanio_bytes; // bytes que ocupa el campo en disco
};

// Estructura (esquema) de la tabla: lista ordenada de columnas.
struct Esquema {
    std::vector<Columna> columnas;

    // Devuelve el tamanio total en bytes de un registro completo
    // (suma de los tamanios de todas las columnas).
    int tamanio_registro() const;
};

// Valor TIPADO de un campo. Conserva el tipo y solo usa el miembro
// correspondiente (no se convierte a texto para almacenar/comparar).
struct Valor {
    TipoDato    tipo;     // tipo del valor
    int         entero;   // se usa si tipo == INTEGER
    double      flotante; // se usa si tipo == DOUBLE_PRECISION
    std::string cadena;   // se usa si tipo == VARCHAR
    bool        booleano; // se usa si tipo == BOOLEAN
};

// Devuelve el tamanio en bytes que corresponde a un tipo de dato.
// Para VARCHAR se usa el parametro 'n' (bytes definidos por el usuario);
// para los demas tipos 'n' se ignora.
int tamanio_de_tipo(TipoDato tipo, int n);

// Serializa un registro (vector de valores tipados) a un buffer de bytes
// crudos, segun el esquema. Cada campo se escribe en su forma BINARIA.
std::vector<unsigned char> serializar_registro(const Esquema& esquema,
                                               const std::vector<Valor>& valores);

// Operacion inversa: reconstruye los valores tipados de un registro a partir
// de su buffer de bytes crudos, segun el esquema.
std::vector<Valor> deserializar_registro(const Esquema& esquema,
                                         const unsigned char* bytes);

// Convierte un valor tipado a texto SOLO para mostrarlo en pantalla.
// (No se usa para almacenar: el almacenamiento es siempre binario.)
std::string valor_a_texto(const Valor& valor);

// Compara dos valores del MISMO tipo. Devuelve:
//   < 0 si a < b ;  0 si a == b ;  > 0 si a > b.
// La comparacion es por tipo (numeros como numeros, cadenas lexicograficamente),
// nunca como texto crudo. La reutilizan las consultas y las estructuras de indice.
int comparar_valores(const Valor& a, const Valor& b);
