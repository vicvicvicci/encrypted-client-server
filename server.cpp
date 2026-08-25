// imports
#include <iostream>
#include <sys/socket.h> // part of posix socket api, supported on unix/linux based systems (this wouldn't work on arduinos)
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>


int main()
{
    // creating server socket
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    // AF_INET = IPv4 protocol, SOCK_STREAM = TCP socket / stream socket

    if(serverSocket < 0) {
        perror("socket failed");
        return 1;
    }

    // macOS has a time wait, rerunning quickly will cause bind() to fail

    int opt = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // defining server address
    sockaddr_in serverAddress = {}; // Zero-initialize using = {} to clear sin_len and padding bytes
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(8888); // what does 8080 mean -> in macOS port is reserved for airplay . htons converts port to network byte order
    serverAddress.sin_addr.s_addr = INADDR_ANY; // accepts connections on any IP - we don't want to bind it to a specific IP 

    // sockaddr_in = data type used to store socket address

    // bind socket to address --> server socket to server address?
    if(bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0){
        perror("bind failed");
        return 1;
    }

    // listen for incoming connections
    if(listen(serverSocket, 5) < 0) { // 5 = max number of pending connections
        perror("listen failed");
        return 1;
    }

    // accept client connection
    int clientSocket = accept(serverSocket, nullptr, nullptr);

    if(clientSocket < 0) {
        perror("accept failed");
        return 1;
    }

    // receiver data from client
    char buffer[1024] = {0};
    recv(clientSocket, buffer, sizeof(buffer), 0);
    std::cout << "Message from client: " << buffer << std::endl;

    // close server socket
    close(serverSocket);
    close(clientSocket);

    return 0;
}