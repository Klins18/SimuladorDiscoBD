# =============================================================================
#  CMakeLists.txt
#  Compila el Simulador descargando automaticamente las dependencias:
#    - SFML 3 (graficos/ventana)
#    - Dear ImGui (widgets de la interfaz)
#    - ImGui-SFML (enlace entre ImGui y SFML)
#  No es necesario instalar nada a mano: FetchContent las trae y compila.
# =============================================================================
cmake_minimum_required(VERSION 3.16)
project(SimuladorDiscoBD LANGUAGES CXX)

set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Genera build/compile_commands.json para que el editor (extension C/C++)
# conozca los include paths exactos y no marque falsos errores de IntelliSense.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)

# ---- SFML 3 ----
FetchContent_Declare(SFML
  GIT_REPOSITORY https://github.com/SFML/SFML.git
  GIT_TAG 3.0.0
  GIT_SHALLOW ON)
set(SFML_BUILD_AUDIO   OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_NETWORK OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(SFML)

# ---- Dear ImGui ----
FetchContent_Declare(imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG v1.91.5
  GIT_SHALLOW ON)
FetchContent_MakeAvailable(imgui)

# ---- ImGui-SFML (binding para SFML 3) ----
FetchContent_Declare(ImGui-SFML
  GIT_REPOSITORY https://github.com/SFML/imgui-sfml.git
  GIT_TAG v3.0
  GIT_SHALLOW ON)
set(IMGUI_DIR ${imgui_SOURCE_DIR})
set(IMGUI_SFML_FIND_SFML OFF CACHE BOOL "" FORCE)
set(IMGUI_SFML_IMGUI_DEMO OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(ImGui-SFML)

# ---- Ejecutable del simulador ----
# El codigo esta organizado en carpetas por responsabilidad:
#   src/disco    -> geometria fisica del disco (platos, pistas, sectores)
#   src/modelo   -> la "base de datos": esquema tipado, datos y consultas
#   src/interfaz -> la interfaz grafica (GUI)
add_executable(SimuladorDiscoBD
  src/interfaz/SimuladorDiscoBD.cpp
  src/disco/Disco.cpp
  src/modelo/Esquema.cpp
  src/modelo/Consultas.cpp
  src/modelo/Datos.cpp
  src/estructuras/ArbolAVL.cpp)

# Carpetas de cabeceras: asi los #include "X.h" funcionan sin rutas largas.
target_include_directories(SimuladorDiscoBD PRIVATE
  src/disco
  src/modelo
  src/estructuras
  src/interfaz)

target_link_libraries(SimuladorDiscoBD PRIVATE ImGui-SFML::ImGui-SFML)

# Copia los archivos de ejemplo junto al ejecutable para poder cargarlos
# escribiendo solo su nombre en la interfaz.
configure_file(${CMAKE_SOURCE_DIR}/datos/datos_ejemplo.csv
               ${CMAKE_BINARY_DIR}/datos_ejemplo.csv COPYONLY)
configure_file(${CMAKE_SOURCE_DIR}/datos/esquema_ejemplo.txt
               ${CMAKE_BINARY_DIR}/esquema_ejemplo.txt COPYONLY)
