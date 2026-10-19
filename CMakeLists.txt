cmake_minimum_required(VERSION 3.16)

project(MeinProjekt LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(MeinProgramm
    code/main.cpp
    MyProjectBase.cpp
)

target_include_directories(MeinProgramm PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

find_package(wxWidgets CONFIG REQUIRED)
find_package(asio CONFIG REQUIRED)
find_package(jsoncons CONFIG REQUIRED)
target_link_libraries(MeinProgramm PRIVATE wx::core wx::base asio::asio jsoncons::jsoncons)