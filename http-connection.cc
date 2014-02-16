/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * @file Implementation for making a server and client connection
 *
 */

#include "http-connection.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include <iostream>
#include <sstream>

#include <signal.h>
#include <sys/wait.h>

// Socket Libraries
#include <sys/socket.h>
#include <sys/poll.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

using namespace std;

#define DEBUG 1

void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int makeServerConnection(const char *port)
{
    if (DEBUG) cout << "Making server connection on port:" << port << endl;
    int sockfd;
    
    struct addrinfo hints;
    struct addrinfo *servinfo;
    struct addrinfo *p;
    
    int status;
    int yes=1;
    
    memset(&hints, 0, sizeof hints);    // Make sure struct is empty
    hints.ai_family = AF_INET;          // IPv4
    hints.ai_socktype = SOCK_STREAM;    // TCP stream sockets
    hints.ai_flags = AI_PASSIVE;        // Use my IP
    
    if ((status = getaddrinfo(NULL, port, &hints, &servinfo)) != 0) {
        perror("Getaddrinfo");
        return -1;
    }
    
    // Loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        // Create socket
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("Socket");
            continue;
        }
        // Set socket options
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
            perror("Setsockopt");
            exit(1); }
        // Bind
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("Bind");
            continue;
        }
        break;
    }
    // No binds
    if (p == NULL)  {
        perror("Bind");
        return -2;
    }

    freeaddrinfo(servinfo);             // Clean up struct
    // Start listening
    if (listen(sockfd, BACKLOG) == -1) {
        perror("Listen");
        exit(1);
    }
    if (DEBUG) cout << "makeServerConnection:Success" << endl;;
    return sockfd;
}


int makeClientConnection(const char *host, const char *port)
{
    if (DEBUG) cout << "Making client connection to host:" << host << " port:" << port << endl;
    
    int sockfd;
    struct addrinfo hints;
    struct addrinfo *servinfo;
    struct addrinfo *p;
    
    char s[INET6_ADDRSTRLEN];
    int status;
    
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    
    if ((status = getaddrinfo(host, port, &hints, &servinfo)) != 0) {
        perror("Getaddrinfo");
        return -1;
    }
    // Loop through all the results and connect to the first we can
    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("Socket");
            continue;
        }
        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("Connect");
            continue;
        }
        break;
    }
    // No binds
    if (p == NULL) {
        perror("Bind");
        return -2;
    }
    
    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof s);
    freeaddrinfo(servinfo);         // Clean up struct
    if (DEBUG) cout << "Client: connected to " << s << " on sock_fd:" << sockfd << endl;
    return sockfd;
}