#include <iostream>
#include <string>
#include <fstream>
#include <array>
#include <asio.hpp>
#include "lib/jsoncons/json.hpp"

using namespace std;
using asio::ip::tcp;

void empfangeTickets(){
    fstream datei("datei.txt");

    asio::io_context io;

    tcp::acceptor acceptor(io, tcp::endpoint(tcp::v4(), 443));

    while(true){
        tcp::socket socket(io);

        acceptor.accept(socket);

        auto endpoint = socket.remote_endpoint();

        string ip = endpoint.address().to_string();

        array<char, 1024> buffer;

        asio::error_code ec;
        size_t bytes;

        while(true){
            bytes = socket.read_some(asio::buffer(buffer), ec);

            if(ec){
                if(ec == asio::error::eof){
                    cerr << "Fehler: Client hat die Verbindung beendet.\n";
                }
                else{
                    cerr << "Socket-Fehler: " << ec.message() << "\n";
                }
                break;
            }
        }

        datei.write(buffer.data(), bytes);
    }
}

int main(){
    empfangeTickets();

    return 0;
}
