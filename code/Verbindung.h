#ifndef VERBINDUNG_H
#define VERBINDUNG_H

//Wenn ip auf "" (Leerstring) steht, wird alles von allen Sendern empfangen(bei der empfange), sende gibt aber einen Fehler aus
//und macht nichts mehr!!!

#include "global.h"

class Verbindung
{
    public:
        Verbindung(string ip, int port, function<void(string daten)> empfangsFunktion);
        virtual ~Verbindung();

        void empfange();
        void sende();

    protected:

    private:
        function<void(string daten)> empfangsFunktion;
        string ip;
        int port;
};

#endif // VERBINDUNG_H
