#include "Verbindung.h"

Verbindung::Verbindung(const char* ip, int port)
{
    this->ip = ip;
    this->port = port;
    this->empfangsFunktion = empfangsFunktion;
}

Verbindung::~Verbindung()
{
    //dtor
}
