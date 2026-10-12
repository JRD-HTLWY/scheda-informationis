#include <iostream>
#include <string>
#include <fstream>
#include <array>
#include <asio.hpp>
#include "lib/jsoncons/json.hpp"

using namespace std;
using asio::ip::tcp;

void empfangeTickets(){
    asio::io_context io;

    tcp::acceptor acceptor(io, tcp::endpoint(tcp::v4(), 12345));

    while(true){
        tcp::socket socket(io);

        acceptor.accept(socket);

        auto endpoint = socket.remote_endpoint();

        string ip = endpoint.address().to_string();

        array<char, 1024> buffer;

        asio::error_code ec;

        while(true){
            size_t bytes = socket.read_some(asio::buffer(buffer), ec);

            if(ec == asio::error::eof){
                cert << "Fehler: Client hat die Verbindung beendet.\n";
            }
        }
    }
}

int main(){


    return 0;
}
