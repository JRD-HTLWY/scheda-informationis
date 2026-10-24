#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <cstdint>
#include <fstream>
#include <array>
#include <chrono>
#include <ctime>
#include <thread>
#include <atomic>
#include <asio.hpp>
#include <stdexcept>
#include "lib/jsoncons/json.hpp"

using namespace std;
using asio::ip::tcp;
using jsoncons::json;

asio::io_context io;

//ID-Vergabe beheben
uint32_t globaleID = 0;
const uint32_t port = 8300;

atomic<bool> stopGefordert{false};
asio::cancellation_signal stopSignal;

void init(){
    //init der JSON-Datei
    fstream jsons("alleNachrichten.txt", ios::out | ios::trunc);

    json initJSONArray = json::array();

    //string initJSONArrayString;
    //initJSONArray.dump(initJSONArrayString);
    jsons << initJSONArray.to_string();

    jsons.close();
}

bool anmelden(string nutzername, string passwort){
    fstream kontenDatei("konten.txt", ios::in | ios::app);
    string zeile;

    string name;
    string passw;
    bool neuesKonto = true;

    while(getline(kontenDatei, zeile)){
        name = zeile.substr(0, zeile.find("@"));
        passw = zeile.substr(zeile.find("@") + 1, zeile.rfind("@")-(zeile.find("@") + 1));

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
    fstream kontenDatei("konten.txt", ios::in);
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

void empfangeTickets(string name, string titel, string frage){
    fstream jsons("alleNachrichten.txt");
    stringstream alleNachrichtenBuf;
    alleNachrichtenBuf << jsons.rdbuf();
    jsons.close();
    json alleNachrichtenJSON = json::parse(alleNachrichtenBuf.str());

    json neueNachricht = json::object();
    neueNachricht["id"] = globaleID++;
    neueNachricht["title"] = titel;
    neueNachricht["status"] = 0;
    auto zeit = chrono::system_clock::now();
    time_t zeitA = chrono::system_clock::to_time_t(zeit);
    neueNachricht["date"] = ctime(&zeitA);
    neueNachricht["customer"] = name;
    neueNachricht["support"] = json::array();
    neueNachricht["question"] = frage;
    neueNachricht["answer"] = "-";
    json content = json::array();
    neueNachricht["content"] = content;

    alleNachrichtenJSON.push_back(neueNachricht);
    fstream jsons2("alleNachrichten.txt", ios::trunc);
    jsons2 << alleNachrichtenJSON.to_string();//final hinzugefuegt
    jsons2.close();
}

void sendeTickets(tcp::socket& socket, string nutzername, bool support){
    fstream jsonDatei("alleNachrichten.txt");
    stringstream jsonStream;
    jsonStream << jsonDatei.rdbuf();
    json alleNachrichten = json::parse(jsonStream.str());

    json sendeDaten = json::array();

    if(support){
        asio::write(socket, asio::buffer(alleNachrichten.to_string() + "\n"));//alle Tickets
    else{
        for(uint32_t i = 0; i < alleNachrichten.size(); i++){//schickt nur Kundendaten
            if(alleNachrichten[i]["customer"] == nutzername){
                sendeDaten.push_back(alleNachrichten[i]);
            }
        }
        asio::write(socket, asio::buffer(sendeDaten.to_string() + "\n")); //senden
    }

    jsonDatei.close();
}

void antworteTicket(int id, string name, string antwort){
    fstream alleNachrichten("alleNachrichten.txt");
    stringstream alleNachrichtenBuf;
    alleNachrichtenBuf << alleNachrichten.rdbuf();
    alleNachrichten.close();
    json alleNachrichtenJSON = json::parse(alleNachrichtenBuf.str());

    for(uint32_t i = 0; i < alleNachrichtenJSON.size(); i++){
        if(alleNachrichtenJSON[i]["id"] == id){
            json neueNachricht = json::object();
            neueNachricht["name"] = name;
            neueNachricht["matter"] = antwort;
            alleNachrichtenJSON[i]["content"].push_back(neueNachricht);

            bool supportEingetragen = false;
            for(uint32_t j = 0; j < alleNachrichtenJSON[i]["support"].size(); j++){
                if(alleNachrichtenJSON[i]["support"][j] == name){
                    supportEingetragen = true;
                    break;
                }
            }

            if(!supportEingetragen){
                alleNachrichtenJSON[i]["support"].push_back(name);
            }
        }
    }

    fstream alleNachrichten2("alleNachrichten.txt", ios::trunc);
    alleNachrichten2 << alleNachrichtenJSON.to_string();
    alleNachrichten2.close();
}

void loescheTicket(string name, int id){
    fstream alleNachrichten("alleNachrichten.txt", ios::in);
    stringstream alleNachrichtenBuf;
    alleNachrichtenBuf << alleNachrichten.rdbuf();
    alleNachrichten.close();
    json alleNachrichtenJSON = json::parse(alleNachrichtenBuf.str());
    json neuesJSON = json::array();

    for(uint32_t i = 0; i < alleNachrichtenJSON.size(); i++){
        if(alleNachrichtenJSON[i]["id"] != id){
            neuesJSON.push_back(alleNachrichtenJSON[i]);
        }
        else if(alleNachrichtenJSON[i]["customer"].as<string>() != name){
            neuesJSON.push_back(alleNachrichtenJSON[i]);
        }
    }

    fstream alleNachrichten2("alleNachrichten.txt", ios::trunc);
    alleNachrichten2 << neuesJSON.to_string();

    alleNachrichten2.close();
}

void befehlLesen(){
    tcp::acceptor acceptor(io, tcp::endpoint(tcp::v4(), port));

    while(!stopGefordert.load()){
        tcp::socket socket(io);
        asio::streambuf buffer;
        asio::error_code ec;
        asio::steady_timer timer(io);
        acceptor.non_blocking(true);

        timer.expires_after(chrono::milliseconds(5000));

        acceptor.accept(socket, ec);

        if(ec == asio::error::would_block || ec == asio::error::try_again){
            this_thread::sleep_for(chrono::milliseconds(5000));
            continue;
        }

        if(ec){
            cerr << "Acceptor-Fehler: " << ec.message() << "\n";
            continue;
        }


        auto endpoint = socket.remote_endpoint();

        timer.async_wait([&](asio::error_code ecd){
            if(!ecd){
                socket.close();
            }
        });

        asio::read_until(socket, buffer, '\n', ec);

        if(ec){
            if(ec == asio::error::eof || ec == asio::error::connection_reset){
                cerr << "Fehler: Client hat die Verbindung beendet.\n";
            }
            else{
                cerr << "Socket-Fehler: " << ec.message() << "\n";
            }
            continue;
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

        json jsonJ;
        try{
            jsonJ = json::parse(jsonString);
        }
        catch(const exception& e){
            cerr << "JSON konnte nicht uebersetzt werden.\n";
            continue;
        }

        string ip = endpoint.address().to_string();

        try{
            if(anmelden(name, passwort)){
                //Befehl ausführen
                if(befehl == "empfangeTickets"){
                    empfangeTickets(name, jsonJ["title"].as<string>(), jsonJ["question"].as<string>());
                }
                if(befehl == "sendeTickets"){
                    sendeTickets(socket, name, istSupport(name));
                }
                if(befehl == "antworteTicket"){
                    antworteTicket(jsonJ["id"].as<int>(), name, jsonJ["answer"].as<string>());
                }
                if(befehl == "loescheTicket"){
                    loescheTicket(name, jsonJ["id"].as<int>());
                }
            }
        }
        catch(const exception& e){
            cerr << "Fehlerhafter Funktionsaufruf\n";
            continue;
        }
    }

        //acceptor.accept(socket);
}

int main(){
    thread zweiterThread(befehlLesen);
    cout << "scheda-informationis-Server ist gestartet!\n";
    while(true){
        string eingabe;
        cout << ">";
        getline(cin, eingabe);

        if(eingabe == "help"){
            cout << "init //init the data for the first time (It DELETES EVERYTHING!!!)\n";
            cout << "upgrade <NAME> //upgrade a costumer to support\n";
            cout << "downgrade <NAME> //downgrade a support to a costumer\n";
            cout << "list-accounts //lists all accounts and displays the status\n";
            cout << "exit //exit the program\n";
        }
        if(eingabe == "init"){
            init();
            cout << "Please restart the server.\n";
        }
        try{
            if((eingabe.substr(0, 8) == "upgrade ") && eingabe.substr(8).find(" ") == string::npos){
                fstream kontenDatei("konten.txt", ios::in | ios::out);
                string zeile;
                vector<string> neueDatei;

                while(getline(kontenDatei, zeile)){
                    if((zeile.substr(0, zeile.find("@")) == eingabe.substr(8)) && zeile[zeile.length()-1] == '@'){
                        neueDatei.push_back(zeile + "S");
                    }
                    else{
                        neueDatei.push_back(zeile);
                    }
                }
                kontenDatei.close();
                fstream kontenDatei2("konten.txt", ios::trunc);

                for(uint32_t i = 0; i < neueDatei.size(); i++){
                    kontenDatei2 << neueDatei[i] << "\n";
                }
                kontenDatei2.close();
            }
            if(eingabe.substr(0, 10) == "downgrade " && eingabe.substr(10).find(" ") == string::npos){
                fstream kontenDatei("konten.txt", ios::in);
                string zeile;
                vector<string> neueDatei;

                while(getline(kontenDatei, zeile)){
                    if((zeile.substr(0, zeile.find("@")) == eingabe.substr(10)) && zeile[zeile.length()-1] != '@'){
                        neueDatei.push_back(zeile.substr(0, zeile.length()-1));
                    }
                    else{
                        neueDatei.push_back(zeile);
                    }
                }
                kontenDatei.close();
                fstream kontenDatei2("konten.txt", ios::trunc);

                for(uint32_t i = 0; i < neueDatei.size(); i++){
                    kontenDatei2 << neueDatei[i] << "\n";
                }
                kontenDatei2.close();
            }
            if(eingabe.substr(0, 13) == "list-accounts"){
                fstream kontenDatei("konten.txt", ios::in);
                string zeile;

                while(getline(kontenDatei, zeile)){
                    cout << zeile.substr(0, zeile.find("@")) << " " << zeile.substr(zeile.find("@")+1, zeile.rfind("@")-(zeile.find("@")+1)) << (zeile[zeile.length()-1] == '@' ? "-->Kunde" : "-->Supportmitglied") << "\n";
                }
                kontenDatei.close();
            }
            if(eingabe.substr(0, 4) == "exit"){
                break;
            }
        }
        catch(const exception& e){
        }
    }

    stopGefordert.store(true);
    zweiterThread.join();

    return 0;
}
