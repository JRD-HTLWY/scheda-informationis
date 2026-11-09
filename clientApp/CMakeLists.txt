cmake_minimum_required(VERSION 3.16)

project(clientApp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(clientApp
    code/main.cpp MyProjectBase.cpp code/networkManager.h
    code/netBridge.cpp code/netBridge.h
)

target_include_directories(clientApp PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

find_package(wxWidgets REQUIRED COMPONENTS core base)
find_package(asio CONFIG QUIET)
find_package(jsoncons CONFIG REQUIRED)

target_link_libraries(clientApp PRIVATE wxWidgets::wxWidgets)
if(asio_FOUND)
    target_link_libraries(clientApp PRIVATE asio::asio)
else()
    find_package(Boost CONFIG REQUIRED)
    target_compile_definitions(clientApp PRIVATE SCHEDA_USE_BOOST_ASIO)
    target_link_libraries(clientApp PRIVATE Boost::headers)
endif()
