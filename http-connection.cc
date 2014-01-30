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

void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

/* @brief Makes server listener
 * Used Beej's guide as reference
 */
int makeServerConnection(const char *port)
{
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
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        return 1;
    }
    
    // Loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        // Create socket
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }
        // Set socket options
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1); }
        // Bind
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: bind");
            continue;
        }
        break;
    }

    // No binds
    if (p == NULL)  {
        fprintf(stderr, "server: failed to bind\n");
        return 2;
    }
    
    freeaddrinfo(servinfo);             // Clean up struct
    
    // Start listening
    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }
    
    if (DEBUG) printf("makeServerConnection:Success\n");
    
    return sockfd;
}

/* @brief Makes client
 * Used Beej's guide as reference
 */
int makeClientConnection(const char *host, const char *port)
{
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
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        return 1;
    }
    
    // Loop through all the results and connect to the first we can
    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("client: socket");
            continue;
        }
        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("client: connect");
            continue;
        }
        break;
    }
    
    // No binds
    if (p == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        return 2;
    }
    
    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof s);
    printf("client: connecting to %s\n", s);
    
    freeaddrinfo(servinfo); // all done with this structure
    if (DEBUG) printf("makeServerConnection:Success\n");
    return sockfd;
}

/*

  const int MAXDATASIZE=64;
  char buffer[MAXDATASIZE];
  string temp;
  int rv;
  
  struct pollfd ufds;
  ufds.fd = sockfd; // open socket # corresponding to the connection
  ufds.events = POLLIN;
  
  while(true) {
    temp = "";
    for(int i=0;i<MAXDATASIZE; i++) buffer[i] = '\0'; //make sure it's empty- not sure if necessary


    //wait for events on the sockets with a half second timeout
    while((rv = poll(&ufds, 1, 500)) > 0 && recv(sockfd, buffer, sizeof(buffer), 0) > 0)
    {
      temp.append(buffer);
    } 

    HttpRequest request;

    if(temp.size() == 0) { // no data => sleep for half a second then try again
      sleep(500);
      continue;
    }
    
    try {
      request.ParseRequest (temp.c_str(), temp.size());

     //Debugging **************
        cout <<"Original \t \t "<<endl<<temp<<endl<<" \t \t Parsed \t"<<endl;

        cout <<"GET "<<request.GetHost()<<":"<<request.GetPort()<<request.GetPath()
          <<" HTTP/"<<request.GetVersion()<<endl;

        string header;
        if((header = request.FindHeader("Connection")).size() > 0)
          cout<<"Connection: "<<header<<endl;
        if((header = request.FindHeader("User-Agent")).size() > 0)
          cout<<"User-Agent: "<<header<<endl;
      //Debugging *********

    //set up proxy cache here.

      if(strcmp(request.FindHeader("Connection").c_str(), "close") == 0 ||
         strcmp(request.GetVersion().c_str(), "1.0") == 0)
        break;   // If the connection is not persisent => close
    } 
    catch (ParseException e) {
      const char* input = e.what(); //returns msg.c_str()
      HttpResponse reply;
      reply.SetVersion("1.1");

      cout<<"Parse exception: "<<e.what()<<endl;

      if(strcmp(input, "Request is not GET") == 0) {
        reply.SetStatusCode("501");
        reply.SetStatusMsg("Not Implemented");
      } else {
        reply.SetStatusCode("400");
        reply.SetStatusMsg("Bad Request");
      }

      char buff[reply.GetTotalLength()];
      reply.FormatResponse(buff);
      send(sockfd, buff, reply.GetTotalLength(), 0); // 
    }*/
