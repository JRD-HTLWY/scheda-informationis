#include "global.h"
#include "Verbindung.h"

void gebeAus(string nachricht){
    cout << "Nachricht:" << nachricht << endl;
}

int main()
{
    Verbindung beispiel("127.0.0.1", 10000, gebeAus);



    return 0;
}
