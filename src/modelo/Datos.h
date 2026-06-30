#pragma once
// =============================================================================
//  Datos.h
//  Importacion de registros desde un archivo CSV/TXT y conversion tipada.
//
//  El archivo de datos se entrega el dia de la revision. Cada linea es un
//  registro y cada campo (separado por comas) corresponde, en orden, a una
//  columna del esquema definido en la interfaz.
//
//  La conversion de texto a numero (stoi/stod) se usa SOLO para interpretar la
//  ENTRADA del archivo; el almacenamiento en disco siempre es BINARIO tipado.
// =============================================================================
#include "Disco.h"
#include "Esquema.h"
#include "Consultas.h"
#include <string>
#include <vector>

// Convierte una linea de texto (campos separados por ',') en un vector de
// valores TIPADOS, segun el esquema.
std::vector<Valor> linea_a_valores(const Esquema& esquema, const std::string& linea);

// Importa todos los registros de un archivo CSV/TXT:
//   - lee el archivo linea por linea,
//   - convierte cada linea a valores tipados (linea_a_valores),
//   - serializa el registro en binario y lo escribe en el disco,
//   - agrega la entrada al catalogo (con su direccion fisica unica).
// Devuelve la cantidad de registros importados; 'mensaje' recibe el resultado.
int importar_csv(const std::string& ruta,
                 const Esquema& esquema,
                 Disco& disco,
                 std::vector<EntradaCatalogo>& catalogo,
                 std::string& mensaje);

// Inserta un unico registro (ya construido como valores tipados) en el disco
// y lo agrega al catalogo. Devuelve la direccion fisica asignada.
DireccionDisco insertar_registro(const Esquema& esquema,
                                 const std::vector<Valor>& valores,
                                 Disco& disco,
                                 std::vector<EntradaCatalogo>& catalogo);

// Lee el esquema desde un archivo de texto con una sentencia CREATE TABLE, p. ej.:
//     CREATE TABLE estudiantes (
//         ciudad  VARCHAR(20),
//         edad    INTEGER,
//         activo  BOOLEAN
//     );
// Reconoce los tipos del proyecto (INTEGER, DOUBLE PRECISION, VARCHAR, BOOLEAN) y sinonimos
// SQL (INT, FLOAT/DOUBLE, VARCHAR/CHAR, BOOLEAN). Reemplaza 'esquema' con el
// leido y devuelve true si tuvo exito; 'mensaje' recibe el resultado.
bool leer_create_table(const std::string& ruta, Esquema& esquema, std::string& mensaje);
