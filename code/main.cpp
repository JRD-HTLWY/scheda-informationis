#include "global.h"

using namespace std;

int main()
{
    cout << "Hello world!" << endl;

    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    if(serverSocket == -1){
        perror("socket");
        return 1;
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(1000);



    return 0;
}
