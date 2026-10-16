#include <iostream>
#include <string>
#include <fstream>
#include <array>
#include <asio.hpp>
#include "lib/jsoncons/json.hpp"

using namespace std;
using asio::ip::tcp;

using jsoncons::json;

void init(){
    fstream jsons("alleNachrichten.txt");
    stringstream alleNachrichtenBuf;
    alleNachrichtenBuf << jsons.rdbuf();
    json alleNachrichtenJSON = json::parse(alleNachrichtenBuf.str());

    json initJSONArray = json::array();

    string initJSONArrayString;
    initJSONArray.dump(initJSONArrayString);
    jsons << initJSONArrayString;
}

bool anmelden(string nutzername, string passwort){
    fstream kontenDatei("konten.txt", ios::out | ios::app);
    string zeile;

    string name;
    string passw;
    bool neuesKonto = true;

    while(getline(kontenDatei, zeile)){
        name = zeile.substr(0, zeile.find("@"));
        passw = zeile.substr(zeile.find("@") + 1);

        if(name == nutzername){
            neuesKonto = false;
            if(passw == passwort){
                return true;
            }
            else{
                return false;
            }
        }
    }

    if(neuesKonto){
        //Konto anlegen
        kontenDatei << nutzername + "@" + passwort + "\n";
    }

    return true;
}

void empfangeTickets(){
    asio::io_context io;

    tcp::acceptor acceptor(io, tcp::endpoint(tcp::v4(), 443));

    asio::streambuf buffer;

    while(true){
        tcp::socket socket(io);

        acceptor.accept(socket);

        auto endpoint = socket.remote_endpoint();

        string ip = endpoint.address().to_string();

        //array<char, 1024> buffer;

        asio::error_code ec;
        size_t bytes;

        asio::read_until(socket, buffer, "\n");

        if(ec){
            if(ec == asio::error::eof){
                cerr << "Fehler: Client hat die Verbindung beendet.\n";
            }
            else{
                cerr << "Socket-Fehler: " << ec.message() << "\n";
            }
            break;
        }

        istream stream(&buffer);
        string jsonString;

        getline(stream, jsonString);

        string name = jsonString.substr(0, jsonString.find("@"));
        jsonString = jsonString.substr(jsonString.find("@"));
        string passwort = jsonString.substr(0, jsonString.find("@"));
        jsonString = jsonString.substr(jsonString.find("@"));//fertiges JSON ohne Anmeldeinfos

        if(anmelden(name, passwort)){
            fstream jsons("alleNachrichten.txt");
            stringstream alleNachrichtenBuf;
            alleNachrichtenBuf << jsons.rdbuf();
            json alleNachrichtenJSON = json::parse(alleNachrichtenBuf.str());

            json jsonF = json::parse(jsonString);

            alleNachrichtenJSON.push_back(jsonF); //final hinzugefuegt
            jsons.close();
        }
    }
}

int main(){
    empfangeTickets();

    return 0;
}
