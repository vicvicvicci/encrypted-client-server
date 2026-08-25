// imports
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <arpa/inet.h>

using namespace std;

int main()
{
    // creating client socket
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    // IPv4, TCP
    if(clientSocket < 0) {
        perror("socket failed");
        return 1;
    }

    // defining server address
    sockaddr_in serverAddress = {};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(8888);
    // macOS rejects if = INADDR_ANY for this 
    serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1");

    // connect to server
   if(connect(clientSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
       perror("connect failed");
       return 1;
   }

    // send data to server
    const char* message = "Hello, Server!";
    send(clientSocket, message, strlen(message), 0);

    // close socket
    close(clientSocket);

    return 0;
}