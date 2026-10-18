#include <iostream>
#include <string>
#include <fstream>
#include <array>
#include <asio.hpp>
#include "lib/jsoncons/json.hpp"

using namespace std;
using asio::ip::tcp;
using jsoncons::json;

asio::io_context io;

void init(){
    //init der JSON-Datei
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

void empfangeTickets(string jsonDaten){
    fstream jsons("alleNachrichten.txt");
    stringstream alleNachrichtenBuf;
    alleNachrichtenBuf << jsons.rdbuf();
    json alleNachrichtenJSON = json::parse(alleNachrichtenBuf.str());

    json jsonF = json::parse(jsonDaten);

    alleNachrichtenJSON.push_back(jsonF); //final hinzugefuegt
    jsons.close();
}

void sendeTickets(string ip, string nutzername){
    tcp::socket socket(io);
    auto endpoint = tcp::endpoint(asio::ip::make_address(ip), 8300);

    socket.connect(endpoint);

    fstream jsonDatei("alleNachrichten.txt");
    stringstream jsonStream;
    jsonStream << jsonDatei.rdbuf();
    json alleNachrichten = json::parse(jsonStream.str());

    json sendeDaten = json::array();

    for(uint32_t i = 0; i < alleNachrichten.size(); i++){//schickt nur Kundendaten
        if(alleNachrichten[i]["customer"] == nutzername){
            sendeDaten.push_back(alleNachrichten[i]);
        }
    }

    asio::write(socket, asio::buffer(sendeDaten.to_string())); //senden
}

void befehlLesen(){
    tcp::acceptor acceptor(io, tcp::endpoint(tcp::v4(), 8300));
    tcp::socket socket(io);
    auto endpoint = socket.remote_endpoint();
    asio::streambuf buffer;

    asio::read_until(socket, buffer, "\n");

    asio::error_code ec;
    size_t bytes;

    if(ec){
        if(ec == asio::error::eof){
            cerr << "Fehler: Client hat die Verbindung beendet.\n";
        }
        else{
            cerr << "Socket-Fehler: " << ec.message() << "\n";
        }
    }

    istream stream(&buffer);
    string jsonString;
    getline(stream, jsonString);

    string befehl = jsonString.substr(0, jsonString.find("@"));
    jsonString = jsonString.substr(jsonString.find("@")+1);
    string name = jsonString.substr(0, jsonString.find("@"));
    jsonString = jsonString.substr(jsonString.find("@")+1);
    string passwort = jsonString.substr(0, jsonString.find("@"));
    jsonString = jsonString.substr(jsonString.find("@")+1);//fertiges JSON ohne Anmeldeinfos

    string ip = endpoint.address().to_string();

    if(anmelden(name, passwort)){
        //Befehl ausführen
        if(befehl == "empfangeTickets"){
            empfangeTickets(jsonString);
        }
        if(befehl == "sendeTickets"){
            sendeTickets(ip, name);
        }
    }
}

int main(){
    befehlLesen();

    return 0;
}
