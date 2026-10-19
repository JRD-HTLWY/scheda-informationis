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

//ID-Vergabe beheben

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
        kontenDatei << nutzername + "@" + passwort + "@\n";
    }

    return true;
}

bool istSupport(string nutzername){//nur nach dem Anmelden verwenden, keine Passwortkontrolle!!!
    fstream kontenDatei("konten.txt", ios::out | ios::app);
    string zeile;

    string name;

    while(getline(kontenDatei, zeile)){
        name = zeile.substr(0, zeile.find("@"));

        if(name == nutzername){
            if(zeile.rfind("@") != zeile.length()-1){
                return true;
            }
        }
    }
    return false; //kann auch bedeuten dass das Konto nicht existiert
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

void sendeTickets(string ip, string nutzername, bool support){
    tcp::socket socket(io);
    auto endpoint = tcp::endpoint(asio::ip::make_address(ip), 8300);

    socket.connect(endpoint);

    fstream jsonDatei("alleNachrichten.txt");
    stringstream jsonStream;
    jsonStream << jsonDatei.rdbuf();
    json alleNachrichten = json::parse(jsonStream.str());

    json sendeDaten = json::array();

    if(support){
        asio::write(socket, asio::buffer(alleNachrichten.to_string()));//alle Tickets
    }
    else{
        for(uint32_t i = 0; i < alleNachrichten.size(); i++){//schickt nur Kundendaten
            if(alleNachrichten[i]["customer"] == nutzername){
                sendeDaten.push_back(alleNachrichten[i]);
            }
        }
    }

    asio::write(socket, asio::buffer(sendeDaten.to_string())); //senden
}

void antworteTicket(int id, string antwort){//nur fuer den Support
    fstream alleNachrichten("alleNachrichten.txt");
    stringstream alleNachrichtenBuf;
    alleNachrichtenBuf << alleNachrichten.rdbuf();
    json alleNachrichtenJSON = json::parse(alleNachrichtenBuf.str());

    for(uint32_t i = 0; i < alleNachrichtenJSON.size(); i++){
        if(alleNachrichtenJSON[i]["id"] == id){
            alleNachrichtenJSON[i]["answer"] = antwort;
        }
    }
}

void loescheTicket(int id){
    fstream alleNachrichten("alleNachrichten.txt");
    stringstream alleNachrichtenBuf;
    alleNachrichtenBuf << alleNachrichten.rdbuf();
    json alleNachrichtenJSON = json::parse(alleNachrichtenBuf.str());

    for(uint32_t i = 0; i < alleNachrichtenJSON.size(); i++){
        if(alleNachrichtenJSON[i]["id"] == id){
            auto arrayIT = alleNachrichtenJSON.array_range();
            auto it = arrayIT.begin();
            alleNachrichtenJSON.erase(it + i);
            return;
        }
    }
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
            sendeTickets(ip, name, istSupport(name));
        }
    }
}

int main(){
    befehlLesen();

    return 0;
}
