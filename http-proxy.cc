/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include <iostream>
#include <stdio.h>

// Socket Libraries
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "http-connection.h"

using namespace std;

int main (int argc, char *argv[])
{
    // Create server listener
    int sockfd;
    if ((sockfd = makeServerConnection(PORT)) < 0) {
        fprintf(stderr, "fail to make server\n");
        return 1;
    }

    printf("Proxy server: waiting for connectinos...\n");
    // Accept incoming connections
    while (1) {
        struct sockaddr_storage their_addr; // Connector's address info
        socklen_t sin_size = sizeof their_addr;
        char s[INET6_ADDRSTRLEN];
        
        int new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);;                         // New connection
        if (new_fd == -1) {
            perror("accept");
            continue;
        }
        inet_ntop(their_addr.ss_family,
                  get_in_addr((struct sockaddr *)&their_addr),
                  s, sizeof s);
        if (DEBUG) printf("server: got connection from %s\n", s);
        
        /*
        if (!fork()) { // this is the child process
            close(sockfd); // child doesn't need the listener
            if (send(new_fd, "Hello, world!", 13, 0) == -1)
            perror("send");
            close(new_fd);
            exit(0);
        }*/
        close(new_fd);  // parent doesn't need this
    }

    return 0;
}
