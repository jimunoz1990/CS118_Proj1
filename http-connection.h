/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * @file Header file for http-connection
 *
 */

#ifndef __CS118_Lab1__http_connection__
#define __CS118_Lab1__http_connection__

#include <iostream>

#define DEBUG 1
#define PROXY_PORT "14886"
#define REMOTE_PORT "80"
#define BACKLOG 20

void *get_in_addr(struct sockaddr *sa);
int makeServerConnection(const char *port);
int makeClientConnection(const char *host, const char *port);

#endif /* defined(__CS118_Lab1__http_connection__) */