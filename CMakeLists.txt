cmake_minimum_required(VERSION 3.16)

project(MeinProjekt LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Alle .cpp und .h Dateien rekursiv finden
file(GLOB_RECURSE SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/code/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/code/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/code/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/lib/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/lib/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/lib/*.hpp"
)

# Ausführbare Datei erstellen
add_executable(server 
    code/main.cpp
)

# Aktuelles Verzeichnis als Include-Pfad
target_include_directories(server PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_include_directories(server PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/lib"
)
