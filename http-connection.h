/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * @file Header file for http-connection
 *
 */

#ifndef __CS118_Lab1__http_connection__
#define __CS118_Lab1__http_connection__

#include <iostream>

#define PROXY_PORT "14886"
#define REMOTE_PORT "80"
#define BACKLOG 20

void *get_in_addr(struct sockaddr *sa);

/*@brief Starts the proxy server. Contains all the socket calls.
 * Used Beej Guide To Network Programming as reference
 *@param port [in] Port number to make proxy server on
 */
int makeServerConnection(const char *port);

/*@brief Starts an instance of a client connection. Contains all the socket calls.
 * Used Beej Guide To Network Programming as reference
 *@parem host [in] Host to connect to
 *       port [in] Port number to make client on
 */
int makeClientConnection(const char *host, const char *port);

#endif /* defined(__CS118_Lab1__http_connection__) */