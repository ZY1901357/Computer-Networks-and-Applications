/*
    This c file does not belong to me, but from the lecture coding vidoeo, 
    from university of Adelaide. Computer Networks and Applications. 
    Socket Programming simple client/server by Cheryl Pope 
    ---------------------------------------------------------
    This file was made for me to learn about how will my assignment 1 will be. 
    I will be using this file as a reference to help me understand how to write my own code.
    I will not be using this code in my assignment, but I will be using the concepts and ideas from this code.

*/

/*
Use this if your on linux im gonna used something simlar because I only have vsc 
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ctype.h>
#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <string.h>
*/

#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

#pragma comment(lib, "ws2_32.lib")


int main(int argc, char * argv[]){

    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
        printf("WSAStartup failed. Error Code: %d\n", WSAGetLastError());
        return 1;
    }

    struct  addrinfo * server_addr;
    struct  addrinfo hints;

    int listen_socket, connection_socket;

    int QLEN = 5; // maximum number of pending connections
    char *DEFAULT_PORT = "7879"; // default port number

    memset(&hints, 0, sizeof(hints)); // set all bytes to 0
    memset(&server_addr, 0, sizeof(server_addr)); // set all bytes to 0
    
    hints.ai_family = AF_UNSPEC; // IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; // TCP stream socket
    hints.ai_flags = AI_PASSIVE; // fill in my IP address

    getaddrinfo(NULL, DEFAULT_PORT, &hints, &server_addr); // get address info


    // Create a socket 
    listen_socket = socket(server_addr->ai_family, server_addr->ai_socktype, server_addr->ai_protocol);


    // bind socket 
    if (bind(listen_socket, server_addr->ai_addr, server_addr->ai_addrlen) != 0) {
        perror("bind failed"); // test if bind fails or not  
    }

    // listen on socket 
    listen ( listen_socket, QLEN);   // QLEN is the maximum number of connections in the queue)

    // accept connection on socket 
    connection_socket = accept(listen_socket, NULL, NULL); // accept connection from client

    // read and write to connect socket 
    char character, capital_character;
    while(recv(connection_socket, &character, sizeof(char), 0)) {
        capital_character = (char) toupper((int) character);
        send(connection_socket, &capital_character, sizeof(char), 0);

    }

    // close socket 
    closesocket(connection_socket); // close connection socket
    closesocket(listen_socket); // close listen socket

    WSACleanup();
    return 0; 



}
