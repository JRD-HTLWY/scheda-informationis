cmake_minimum_required(VERSION 3.16)

project(MeinProjekt LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Alle .cpp und .h Dateien rekursiv finden
file(GLOB_RECURSE SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)

# Ausführbare Datei erstellen
add_executable(MeinProjekt ${SOURCES})

# Aktuelles Verzeichnis als Include-Pfad
target_include_directories(MeinProjekt PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
