cmake_minimum_required(VERSION 3.16)

project(MeinProjekt LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(MeinProgramm
    code/main.cpp MyProjectBase.cpp code/networkManager.h
    code/netBridge.cpp code/netBridge.h
)

target_include_directories(MeinProgramm PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

find_package(wxWidgets REQUIRED COMPONENTS core base)
find_package(asio CONFIG QUIET)
find_package(jsoncons CONFIG REQUIRED)

target_link_libraries(MeinProgramm PRIVATE wxWidgets::wxWidgets jsoncons::jsoncons)
if(asio_FOUND)
    target_link_libraries(MeinProgramm PRIVATE asio::asio)
else()
    find_package(Boost CONFIG REQUIRED)
    target_compile_definitions(MeinProgramm PRIVATE SCHEDA_USE_BOOST_ASIO)
    target_link_libraries(MeinProgramm PRIVATE Boost::headers)
endif()
