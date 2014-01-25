/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * @file Header file for http-connection
 *
 */

#ifndef __CS118_Lab1__http_connection__
#define __CS118_Lab1__http_connection__

#include <iostream>

#define PORT "14886"
#define BACKLOG 20

int makeServerConnection(const char *port);
int makeClientConnection(const char *host, const char *port);

#endif /* defined(__CS118_Lab1__http_connection__) */