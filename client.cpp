// imports
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <arpa/inet.h> // inet address


int main()
{
    // get user choice between read/right before making a connection
    std::cout << "Enter 1 to read from server, 2 to write to server: ";
    int choice;
    std::cin >> choice;
    std::cin.ignore();

    if (choice!=1 && choice!=2) {
        std::cerr << "Invalid choice. Exiting." << std::endl;
        return 1;
    }

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

   // send choice to server first
   send(clientSocket, &choice, sizeof(choice), 0);
   std::cout << "Connected to server" << (choice == 1 ? " for reading." : " for writing.") << std::endl;

    // send data to server
    const char* message = "Hello, Server!";
    send(clientSocket, message, strlen(message), 0);

    // loop to send messages from user to server
    std::string userInput;
    while(true){
        std::cout<<"Send message to server: ";
        std::getline(std::cin,userInput);
        if(userInput == "exit"){
            break;
        }
        send(clientSocket, userInput.c_str(), userInput.length(), 0);
    }

    // close socket
    close(clientSocket);

    return 0;
}



// write about the problems i faced