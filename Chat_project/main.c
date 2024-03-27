#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


void main(){
    int clientSocket;
    struct sockaddr_in serverAddr;
    char buffer[1024];

    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    menset(&serverAddr, '\0', sizeof(serverAddr));
    serverAddr.sin_family=AF_INET;
    serverAddr.sin_port=2000;
    serverAddr.sin_addr.s_addr=inet_addr("127.0.0.1");

    connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr));

    recv(clientSocket, buffer, 1024,0);
    printf("Data Received: %s", buffer);
}