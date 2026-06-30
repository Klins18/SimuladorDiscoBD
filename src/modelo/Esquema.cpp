// =============================================================================
//  Esquema.cpp
//  Implementacion del modelo de datos tipado y de la (de)serializacion binaria.
//  Ver Esquema.h para la descripcion del proposito de este modulo.
// =============================================================================
#include "Esquema.h"
#include <cstring>  // std::memcpy (copiar bytes en crudo)
#include <string>

// Devuelve el tamanio en bytes de un tipo de dato.
// Para VARCHAR se usa 'n' (tamanio definido por el usuario); el resto es fijo.
int tamanio_de_tipo(TipoDato tipo, int n) {
    switch (tipo) {
        case TipoDato::INTEGER:          return BYTES_INTEGER;  // 4 bytes
        case TipoDato::DOUBLE_PRECISION: return BYTES_DOUBLE;   // 8 bytes
        case TipoDato::VARCHAR:          return n;              // N bytes (lo elige el usuario)
        case TipoDato::BOOLEAN:          return BYTES_BOOLEAN;  // 1 byte
    }
    return 0;  // tipo desconocido (no deberia pasar)
}

// Tamanio total de un registro = suma de los tamanios de todas las columnas.
int Esquema::tamanio_registro() const {
    int total = 0;                          // acumulador
    for (const Columna& col : columnas) {   // recorre cada columna del esquema
        total += col.tamanio_bytes;         // suma sus bytes
    }
    return total;                           // tamano completo de una fila
}

// -----------------------------------------------------------------------------
// Serializacion: convierte los valores tipados en un buffer de bytes BINARIOS.
// Cada campo se copia en su representacion nativa (no como texto).
// -----------------------------------------------------------------------------
std::vector<unsigned char> serializar_registro(const Esquema& esquema,
                                               const std::vector<Valor>& valores) {
    // Buffer del tamano del registro, inicializado en 0 (asi VARCHAR queda "relleno").
    std::vector<unsigned char> bytes(esquema.tamanio_registro(), 0);
    int offset = 0;  // posicion donde escribir el campo actual dentro del buffer

    // Recorremos cada columna (i) junto con su valor.
    for (size_t i = 0; i < esquema.columnas.size(); ++i) {
        const Columna& col = esquema.columnas[i];   // definicion de la columna
        const Valor&   val = valores[i];            // valor a guardar

        switch (col.tipo) {
            case TipoDato::INTEGER: {
                int entero = val.entero;            // tomamos el entero
                // Copiamos sus 4 bytes tal cual (en binario) al buffer.
                std::memcpy(&bytes[offset], &entero, BYTES_INTEGER);
                break;
            }
            case TipoDato::DOUBLE_PRECISION: {
                double flotante = val.flotante;     // tomamos el double
                std::memcpy(&bytes[offset], &flotante, BYTES_DOUBLE); // sus 8 bytes
                break;
            }
            case TipoDato::VARCHAR: {
                int n = (int)val.cadena.size();             // largo del texto
                if (n > col.tamanio_bytes) n = col.tamanio_bytes; // si excede, se trunca
                // Copiamos los caracteres; lo que sobre del campo queda en 0 (relleno).
                std::memcpy(&bytes[offset], val.cadena.data(), n);
                break;
            }
            case TipoDato::BOOLEAN: {
                bytes[offset] = val.booleano ? 1 : 0;  // un solo byte: 1 = true, 0 = false
                break;
            }
        }
        offset += col.tamanio_bytes;   // avanzamos al lugar de la siguiente columna
    }
    return bytes;                      // registro listo en bytes
}

// -----------------------------------------------------------------------------
// Deserializacion: reconstruye los valores tipados desde el buffer binario.
// Es la operacion inversa de serializar_registro.
// -----------------------------------------------------------------------------
std::vector<Valor> deserializar_registro(const Esquema& esquema,
                                         const unsigned char* bytes) {
    std::vector<Valor> valores;   // valores reconstruidos
    int offset = 0;               // posicion de lectura dentro del buffer

    for (const Columna& col : esquema.columnas) {  // por cada columna del esquema
        Valor val;
        val.tipo = col.tipo;      // el valor tendra el tipo de su columna

        switch (col.tipo) {
            case TipoDato::INTEGER: {
                int entero = 0;
                // Leemos 4 bytes y los interpretamos como un entero.
                std::memcpy(&entero, &bytes[offset], BYTES_INTEGER);
                val.entero = entero;
                break;
            }
            case TipoDato::DOUBLE_PRECISION: {
                double flotante = 0.0;
                std::memcpy(&flotante, &bytes[offset], BYTES_DOUBLE); // 8 bytes -> double
                val.flotante = flotante;
                break;
            }
            case TipoDato::VARCHAR: {
                // Tomamos los N bytes del campo como texto.
                std::string s((const char*)&bytes[offset], col.tamanio_bytes);
                size_t fin = s.find('\0');                 // buscamos el primer '\0' (fin real)
                if (fin != std::string::npos) s.resize(fin); // recortamos el relleno
                val.cadena = s;
                break;
            }
            case TipoDato::BOOLEAN: {
                val.booleano = (bytes[offset] != 0);   // cualquier byte != 0 es true
                break;
            }
        }
        valores.push_back(val);        // guardamos el valor reconstruido
        offset += col.tamanio_bytes;   // avanzamos a la siguiente columna
    }
    return valores;
}

// Representacion textual de un valor (SOLO para mostrar en pantalla, no para guardar).
std::string valor_a_texto(const Valor& valor) {
    switch (valor.tipo) {
        case TipoDato::INTEGER:          return std::to_string(valor.entero);
        case TipoDato::DOUBLE_PRECISION: return std::to_string(valor.flotante);
        case TipoDato::VARCHAR:          return valor.cadena;
        case TipoDato::BOOLEAN:          return valor.booleano ? "true" : "false";
    }
    return "";
}

// Compara dos valores del mismo tipo (ver Esquema.h). Devuelve <0, 0 o >0.
int comparar_valores(const Valor& a, const Valor& b) {
    switch (a.tipo) {
        case TipoDato::INTEGER:
            // Truco para comparar enteros: (a>b) - (a<b) da 1, 0 o -1.
            return (a.entero > b.entero) - (a.entero < b.entero);
        case TipoDato::DOUBLE_PRECISION:
            if (a.flotante < b.flotante) return -1;
            if (a.flotante > b.flotante) return 1;
            return 0;
        case TipoDato::VARCHAR:
            // compare() de std::string ya devuelve <0, 0 o >0 (orden lexicografico).
            return a.cadena.compare(b.cadena);
        case TipoDato::BOOLEAN:
            // false(0) vs true(1): los pasamos a 0/1 y restamos.
            return (a.booleano ? 1 : 0) - (b.booleano ? 1 : 0);
    }
    return 0;
}
