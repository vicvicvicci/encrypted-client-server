// imports
#include <iostream>
#include <sys/socket.h> // part of posix socket api, supported on unix/linux based systems (this wouldn't work on arduinos)
#include <netinet/in.h> //structure for storing address info
#include <unistd.h>
#include <cstring>
#include <arpa/inet.h> // inet address

// (additional) for threading
#include <semaphore.h>
#include <pthread.h> // posix threads used in c
#include <thread> // c++ native threads
#include <stdio.h>
#include <stdlib.h> //go learn what these includes do
#include <string.h>

// for usernames, passwords
#include <unordered_map>
#include <string>

// for openssl functions
#include "crypto.h"

// need to create sha512 conversion function


// key: username, value: password (SHA512 password hash)
std::unordered_map<std::string, std::string> userDatabase = {
    {"alice", calculate_sha512_hex((const unsigned char*)"password123", strlen("password123"))}
};

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

// check user login
bool verifyUser (const std::string& username, const std::string& password) {
    auto it = userDatabase.find(username);
    if (it != userDatabase.end()) {
        // compare the stored hash with the hash of the provided password
        return it->second == password; // match found
    }
    return false; // user does not exist
}

// multiclient implementation : have reader thread and writer thread

// semaphore variables
// what is a semaphore ? 

sem_t x, y; // x as mutual exclusion (mutex) lock to protect reader count, y blocks writer when readers active and vice versa
int readercount =0; // keep track of number of readers

// reader function

void* reader(void* param){ // param accepts general pointer
    int clientSocket = (int)(intptr_t)param; // retreive socket value safely
    pthread_detach(pthread_self()); // detach the thread on exit (release resources)

    // lock semaphore
    sem_wait(&x); // &x passes memory address of x so sem_wait can modify it directly
    readercount ++;

    if (readercount ==1){
        sem_wait(&y); // readers-writers algorithm locks when readercount == 1. >1 would cause deadlock, first reader has to lock so writers cannot enter and subsequent readers no longer have to lock.
    }

    // unlock semaphore to allow other threads to modify readercount
    sem_post(&x);

    printf("Reader %ld is reading\n", (long)param);


    // receive messages from server in a loop
    while(true){

        uint32_t netLength = 0;

        // read 4-byte (exactly) from header
        int bytesReceived = recv(clientSocket, &netLength, sizeof(netLength), MSG_WAITALL); // wait for all 4 bytes
        if (bytesReceived <= 0) {
            break;
        }

        uint32_t payloadLength = ntohl(netLength); // convert network byte order back to int

        std::vector<char> buffer(payloadLength + 1, 0); // extra byte for null terminator
        recv(clientSocket, buffer.data(), payloadLength, MSG_WAITALL); // wait for all bytes
        std::cout << "Received (" << payloadLength << " bytes): " << buffer.data() << std::endl;
    }

    // lock the semaphore to update readercount when leaving
    sem_wait(&x);
    readercount--;

    if (readercount == 0) {
        sem_post(&y);
    }

    // unlock the semaphore
    sem_post(&x);

    printf("Reader %ld has finished reading\n", (long)param);
    close(clientSocket);
    return nullptr;
}

// writer function
void* write(void* param){
    int clientSocket = (int)(intptr_t)param;
    pthread_detach(pthread_self());

    std::cout<<"Writer "<<clientSocket<<" is waiting for access...\n";

    // lock semaphore
    sem_wait(&y);
    std::cout<<"Writer on socket "<<clientSocket<<" has entered critical section\n";

    while (true) {
        std::string message = receiveFramedString(clientSocket);
        if (message.empty()) break;
        std::cout << "[Writer Socket " << clientSocket << "]: " << message << std::endl;
    }
    sem_post(&y);
    std::cout<<"Writer "<<clientSocket<<" has finished writing\n";

    close(clientSocket);
    return nullptr;
}


// driver code - accept connection and hand them off
int main()
{
    // initialise semaphores
    sem_init(&x, 0, 1); // mutex for reader count
    sem_init(&y, 0, 1); // mutex for writer access

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
    if(listen(serverSocket, 50) < 0) { // 50 = max number of pending connections
        perror("listen failed");
        return 1;
    }

    std::cout << "Server is listening on port 8888\n";


    while (true){
        // extract first client connection in queue
        int clientSocket = accept(serverSocket, nullptr, nullptr);

        if (clientSocket < 0) {
            perror("accept failed");
            continue;
        }

        std::string username = receiveFramedString(clientSocket);
        std::string passwordHashed = receiveFramedString(clientSocket);

        if(!verifyUser(username, passwordHashed)) {
            std::cout << "Authentication failed for user: " << username << std::endl;
            sendFramedString(clientSocket, "Authentication failed. Closing connection.");
            close(clientSocket);
            continue;
        }

        std::cout << "Client connected: " << username << std::endl;
        sendFramedString(clientSocket, "Authentication successful");

        int choice = 0;
        recv(clientSocket, &choice, sizeof(choice),0);

        pthread_t tid; // thread id

        if (choice == 1){ // create readers thread
            if (pthread_create(&tid, nullptr, reader, (void*)(intptr_t)clientSocket) != 0){ // passing in &clientSocket creates data race so pass by val instead of ref
                perror("Failed to create reader thread");
                return 1;
            }
        }
        else if (choice == 2){ // create writers thread
            if (pthread_create(&tid, nullptr, write, (void*)(intptr_t)clientSocket) != 0){
                perror("Failed to create writer thread");
                return 1;
            }
        }
    }

    sem_destroy(&x);
    sem_destroy(&y);
    close(serverSocket);
    return 0;
}



/**    // accept client connection
    int clientSocket = accept(serverSocket, nullptr, nullptr);

    if(clientSocket < 0) {
        perror("accept failed");
        return 1;
    }

    // receiver data from client
    char buffer[1024] = {0};
    recv(clientSocket, buffer, sizeof(buffer), 0);
    std::cout << "Message from client: " << buffer << std::endl;

    // receive messages from client in a loop
    while(true){
        char buffer[1024] = {0};
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
        if(bytesReceived <= 0) {
            break;
        }
        std::cout << "Message from client: " << buffer << std::endl;
    }

    // close server socket
    close(serverSocket);
    close(clientSocket);
 */

 // if its the last client on, stop listening until new client join? or something