#include <iostream>
#include <string>
#include <fstream>
#include <array>
#include <chrono>
#include <ctime>
#include <asio.hpp>
#include "lib/jsoncons/json.hpp"

using namespace std;
using asio::ip::tcp;
using jsoncons::json;

asio::io_context io;

//ID-Vergabe beheben
uint32_t globaleID = 0;
auto zeit = chrono::system_clock::now();
const uint32_t port = 8300;

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

void empfangeTickets(string name, string titel, string frage){
    fstream jsons("alleNachrichten.txt");
    stringstream alleNachrichtenBuf;
    alleNachrichtenBuf << jsons.rdbuf();
    json alleNachrichtenJSON = json::parse(alleNachrichtenBuf.str());

    json neueNachricht = json::object();
    neueNachricht["id"] = globaleID++;
    neueNachricht["title"] = titel;
    neueNachricht["status"] = 0;
    time_t zeitA = chrono::system_clock::to_time_t(zeit);
    neueNachricht["date"] = ctime(&zeitA);
    neueNachricht["customer"] = name;
    neueNachricht["support"] = "-";
    neueNachricht["question"] = frage;
    neueNachricht["answer"] = "-";

    alleNachrichtenJSON.push_back(neueNachricht);
    jsons << alleNachrichtenJSON.to_string();//final hinzugefuegt
    jsons.close();
}

void sendeTickets(string ip, string nutzername, bool support){
    tcp::socket socket(io);
    auto endpoint = tcp::endpoint(asio::ip::make_address(ip), port);

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

    jsonDatei.close();
}

void antworteTicket(int id, string name, string antwort){
    fstream alleNachrichten("alleNachrichten.txt");
    stringstream alleNachrichtenBuf;
    alleNachrichtenBuf << alleNachrichten.rdbuf();
    json alleNachrichtenJSON = json::parse(alleNachrichtenBuf.str());

    for(uint32_t i = 0; i < alleNachrichtenJSON.size(); i++){
        if(alleNachrichtenJSON[i]["id"] == id){
            json neueNachricht = json::object();
            neueNachricht["name"] = name;
            neueNachricht["matter"] = antwort;
            alleNachrichtenJSON[i]["content"].push_back(neueNachricht);
        }
    }

    alleNachrichten << alleNachrichtenJSON.to_string();

    alleNachrichten.close();
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

    alleNachrichten << alleNachrichtenJSON.to_string();

    alleNachrichten.close();
}

void befehlLesen(){
    tcp::acceptor acceptor(io, tcp::endpoint(tcp::v4(), port));

    while(true){
        tcp::socket socket(io);
        acceptor.accept(socket);
        auto endpoint = socket.remote_endpoint();
        asio::streambuf buffer;

        asio::error_code ec;
        asio::read_until(socket, buffer, "\n", ec);

        size_t bytes;

        if(ec){
            if(ec == asio::error::eof || ec == asio::error::connection_reset){
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

        json jsonJ;
        try{
            jsonJ = json::parse(jsonString);
        }
        catch(const exception& e){
            cerr << "JSON konnte nicht uebersetzt werden.\n";
            continue;
        }

        string ip = endpoint.address().to_string();

        if(anmelden(name, passwort)){
            //Befehl ausführen
            if(befehl == "empfangeTickets"){
                empfangeTickets(name, jsonJ["title"].to_string(), jsonJ["question"].to_string());
            }
            if(befehl == "sendeTickets"){
                sendeTickets(ip, name, istSupport(name));
            }
            if(befehl == "antworteTicket"){
                antworteTicket(jsonJ["id"].as<int>(), name, jsonJ["answer"].to_string());
            }
            if(befehl == "loescheTicket"){
                loescheTicket(jsonJ["id"].as<int>());
            }
        }
    }
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
                    neueDatei.push_back(zeile);
                }
            }
            if(eingabe.substr(0, 10) == "downgrade " && eingabe.substr(10).find(" ") == string::npos){
                fstream kontenDatei("konten.txt", ios::in | ios::out);
                string zeile;
                vector<string> neueDatei;

                while(getline(kontenDatei, zeile)){
                    if((zeile.substr(0, zeile.find("@")) == eingabe.substr(8)) && zeile[zeile.length()-1] != '@'){
                        neueDatei.push_back(zeile.substr(0, zeile.length()-1));
                    }
                    neueDatei.push_back(zeile);
                }
            }
            if(eingabe.substr(0, 14) == "list-accounts " && eingabe.substr(14).find(" ") == string::npos){
                //weiter machen
            }
        }
        catch(const exception& e){
        }
    }
    zweiterThread.join();

    return 0;
}
