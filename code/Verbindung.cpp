#include "Verbindung.h"

Verbindung::Verbindung(string ip, int port, function<void(string daten)> empfangsFunktion)
{
    this->ip = ip;
    this->port = port;
    this->empfangsFunktion = empfangsFunktion;

    thread empfangsThread(&Verbindung::empfange, this);
}

Verbindung::~Verbindung()
{
    //dtor
}

void Verbindung::empfange(){
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if(serverSocket  == -1){
        perror("socket");
        return;
    }

    sockaddr_in serverAddr = {};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if(bind(serverSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == -1){
        perror("bind");
        close(serverSocket);
        return;
    }

    if(listen(serverSocket, 5) == -1){
        perror("listen");
        close(serverSocket);
        return;
    }

    while(true){
        sockaddr_in clientAddr = {};
        socklen_t clientLen = sizeof(clientAddr);

        int clientSocket = accept(serverSocket, reinterpret_cast<sockaddr*>(&clientAddr), &clientLen);

        if(clientSocket == -1){
            perror("accept");
            continue;
        }

        char clientIP[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, sizeof(clientIP));

        unsigned short clientPort = ntohs(clientAddr.sin_port);

        char buffer[1024];

        ssize_t bytes = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);

        if(bytes > 0){
            buffer[bytes] = '\0';
        }

        empfangsFunktion(buffer);

        close(clientSocket);
    }

    close(serverSocket);
}


void Verbindung::sende(string daten){
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock < 0){
        perror("socket");
        return;
    }

    sockaddr_in serverAddr = {};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);

    if(inet_pton(AF_INET, ip.c_str(), &serverAddr.sin_addr) != 1){
        cerr << "Fehler: IP-Ungültig" << endl;
        close(sock);
        return;
    }

    if(connect(sock, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) < 0){
        perror("connect");
        close(sock);
        return;
    }

    size_t gesendet = 0;
    size_t nachrichtLaenge = strlen(daten.c_str());

    while(gesendet < nachrichtLaenge){
        ssize_t bytes = send(sock, daten.c_str() + gesendet, nachrichtLaenge - gesendet, 0);

        if(bytes <= 0){
            perror("send");
            close(sock);
            return;
        }

        gesendet += static_cast<size_t>(bytes);
    }

    close(sock);
    return;
}
