/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * @file Implementation for cache class
 *
 */

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string>
#include <string.h>
#include <time.h>
#include <map>

#include <iostream>
#include <sstream>

// Socket Libraries
#include <sys/socket.h>
#include <sys/poll.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "http-cache.h"
#include "http-request.h"
#include "http-response.h"
#include "http-connection.h"
#include "http-page.h"

// Boost
#include <boost/thread.hpp>
#include <boost/bind.hpp>

using namespace std;

Cache::Cache() {
    num_connections = 0;
}

/* Cache store functions
 */
Page* Cache::getFromStore(string URL) {
    map<string, Page>::iterator iter;
    iter = store.find(URL); // check if the URL is stored in the cache
    if(iter == store.end())  // if not return null
        return NULL;
    else
        return &iter->second; // if it is, return the Page
}

void Cache::removeFromStore(string URL) {
    store.erase(URL);
}

void Cache::addToStore(string URL, Page webpg) {
    store.erase(URL);
    store.insert(map<string, Page>::value_type(URL, webpg));
}

/* Cache connections functions 
 */
void Cache::addToConnections(string URL, int sock_fd) {
    connections.insert(map<string, int>::value_type(URL, sock_fd));
}

int Cache::getFromConnections(string URL) {
    map<string, int>::iterator iter;
    iter = connections.find(URL);
    if (iter == connections.end())
        return -1;
    else
        return iter->second;
}

void Cache::removeFromConnections(string URL) {
    connections.erase(URL);
}
