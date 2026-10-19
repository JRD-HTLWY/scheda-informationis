cmake_minimum_required(VERSION 3.16)

project(MeinProjekt LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Alle .cpp und .h Dateien rekursiv finden
file(GLOB_RECURSE SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/code/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/code/*.h"
)

add_executable(MeinProgramm
    main.cpp
    MyProjectBase.cpp
# Ausführbare Datei erstellen
add_executable(MeinProjekt ${SOURCES}
        code/main.cpp
)

# Aktuelles Verzeichnis als Include-Pfad
target_include_directories(MeinProjekt PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

find_package(wxWidgets CONFIG REQUIRED)
find_package(asio CONFIG REQUIRED)
find_package(jsoncons CONFIG REQUIRED)
target_link_libraries(MeinProjekt PRIVATE wx::core wx::base asio::asio jsoncons::jsoncons)