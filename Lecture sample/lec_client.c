/*
    This c file does not belong to me, but from the lecture coding vidoeo, 
    from university of Adelaide. Computer Networks and Applications. 
    Socket Programming simple client/server by Cheryl Pope 
    ---------------------------------------------------------
    This file was made for me to learn about how will my assignment 1 will be. 
    I will be using this file as a reference to help me understand how to write my own code.
    I will not be using this code in my assignment, but I will be using the concepts and ideas from this code.

----for linux users-----
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h> 
#include <string.h>
#include <unistd,h>
#include <stdbool.h>
*/
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#pragma comment(lib, "ws2_32.lib")

int main(int argc, char * argv[]){

    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("WSAStartup failed. Error Code: %d\n", WSAGetLastError());
        return 1;
    }

    char * SERVER_NAME = "localhost"; // server name
    char * SERVER_PORT = "7879"; // server port number

    

    struct  addrinfo hints;
    struct  addrinfo * server_addr;
    SOCKET client_socket;

    memset(&hints, 0, sizeof(hints)); // set all bytes to 0
    memset(&server_addr, 0, sizeof(server_addr)); // set all bytes to 0

    hints.ai_family = AF_INET; // IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; // TCP stream socket
    
    if (getaddrinfo(SERVER_NAME, SERVER_PORT, &hints, &server_addr)); // get address info
    

    // create a socket
    client_socket = socket(server_addr->ai_family, server_addr->ai_socktype, server_addr->ai_protocol);

    //connect to server
    if (connect(client_socket, server_addr->ai_addr,(int) server_addr->ai_addrlen) != 0);
        perror("connect failed");

    //read and write on socket 
    char character;
    while(true){
        puts("INput lowercase letter: ");
        character = getchar();
        send(client_socket, &character, sizeof(char), 0);

        puts("Server returned: ");
        recv(client_socket, &character, sizeof(char), 0);
        putchar(character);

        getchar();
        putchar('\n');
    }

    // close socket 
    closesocket (client_socket);
    freeaddrinfo(server_addr); // free address info

    WSACleanup();
    return 0;
}
