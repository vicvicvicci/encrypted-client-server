// imports
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <arpa/inet.h> // inet address
#include <string>

// for openssl functions
#include "crypto.h"

void sendFramedString(int socket, const std::string& message) {
    uint32_t payloadLength = message.length();
    uint32_t networkLength = htonl(payloadLength);

    // send 4-byte header first (to tell server how many bytes to expect)
    send(socket, &networkLength, sizeof(networkLength), 0);
    send(socket, message.c_str(), payloadLength, 0); // send actual data
}

// receiver data
std::string receiveFramedString(int clientSocket) {
    uint32_t netLen = 0;
    if (recv(clientSocket, &netLen, sizeof(netLen), MSG_WAITALL) <= 0) {
        return ""; // Connection closed or error
    }
    
    uint32_t len = ntohl(netLen);
    std::string buffer(len, '\0'); // Allocate memory space before receiving
    recv(clientSocket, &buffer[0], len, MSG_WAITALL);
    
    return buffer;
}

void authenticateClient(int clientSocket) {

    // take username and password
    std::string username, password;
    std::cout << "Enter username: ";
    std::cin >> username;
    std::cout << "Enter password: ";
    std::cin >> password;

    // hash password
    std::string hashedPassword = calculate_sha512_hex((const unsigned char*)password.data(), password.length());

    sendFramedString(clientSocket, username);
    sendFramedString(clientSocket, hashedPassword);

    // wait for server response
    std::string serverResponse = receiveFramedString(clientSocket);
    std::cout << "Server response: " << serverResponse << std::endl;

}
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

   authenticateClient(clientSocket);

    // get user choice between read/right before making a connection
    std::cout << "Enter 1 to read from server, 2 to write to server: ";
    int choice;
    std::cin >> choice;
    std::cin.ignore();

    if (choice!=1 && choice!=2) {
        std::cerr << "Invalid choice. Exiting." << std::endl;
        return 1;
    }

   // send choice to server first
   send(clientSocket, &choice, sizeof(choice), 0);
   std::cout << "Connected to server" << (choice == 1 ? " for reading." : " for writing.") << std::endl;

   if (choice == 1) {
       // loop to receive messages from server
       while (true) {
           std::string message = receiveFramedString(clientSocket);
           if (message.empty()) break;
           std::cout << "[Reader Socket " << clientSocket << "]: " << message << std::endl;
       }
   } else if (choice == 2) {

        // Shared testing credentials (must match server.cpp)
        const unsigned char key[] = "01234567890123456789012345678901"; // hardcoded 32-byte key for AES-256 (for now)
        const unsigned char iv[]  = "0123456789012345";

       // loop to send messages from user to server
       std::string userInput;
       while (true) {
           std::cout << "Send message to server: ";
           std::getline(std::cin, userInput);

           if (userInput == "exit") {
               break;
           }

           // encrypt the message using AES-256 before sending
           // into binary vector of unsigned char
           std::vector<unsigned char> encryptedBytes = encrypt_aes256(userInput, key, iv);
           // convert the encrypted bytes to a string for sending
           std::string encryptedMessage(encryptedBytes.begin(), encryptedBytes.end());

           sendFramedString(clientSocket, encryptedMessage);
       }
   }

    // close socket
    close(clientSocket);

    return 0;
}