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

#define MAX_SERVER_CONNECTIONS 100
#define CONNECTION_TIMEOUT 30

/***************************************
 *        Cache store functions        *
 **************************************/
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

/***************************************
 *     Cache connections functions     *
 **************************************/
void Cache::addToConnections(string URL, int sock_fd) {
    // Check if there is space for a new entry
    if (connections.size() < 100) {
        connections.insert(map<string, int>::value_type(URL, sock_fd));
        
        // Add corresponding entry to cache connections_age
        time_t cur_time = time(NULL);
        connections_age.insert(map<string, int>::value_type(URL, cur_time));
        
    }
    else {
        // Cache replacement policy
        cacheReplacementPolicy();
        connections.insert(map<string, int>::value_type(URL, sock_fd));
        
        // Add corresponding entry to cache connections_age
        time_t cur_time = time(NULL);
        connections_age.insert(map<string, int>::value_type(URL, cur_time));
    }
}

int Cache::getFromConnections(string URL) {
    map<string, int>::iterator iter;
    iter = connections.find(URL);
    if (iter == connections.end())
        return -1;
    else {
        connections_age.erase(URL);
        // Update time for connection
        time_t cur_time = time(NULL);
        connections_age.insert(map<string, time_t>::value_type(URL, cur_time));
        return iter->second;
    }
}

void Cache::removeFromConnections(string URL) {
    connections.erase(URL);
    connections_age.erase(URL);
}

void Cache::cacheReplacementPolicy() {
    if (DEBUG) cout << "In cacheReplacementPolicy..." << endl;
    
    map<string, time_t>::iterator iter;
    time_t oldest = time(NULL);
    string oldest_key = "";
    
    // Find the oldest connection socket
    for (iter = connections_age.begin(); iter != connections_age.end(); iter++) {
        if (iter->second < oldest) {
            oldest = iter->second;
            oldest_key = iter->first;
        }
    }
    
    int sock_fd = getFromConnections(oldest_key);
    close(sock_fd);
    removeFromConnections(oldest_key);
}

void Cache::cacheConnectionCleanup() {
    if (DEBUG) cout << "In cacheConnectionCleanup()..." << endl;
    int sock_fd = 0;
    string key = "";

    if (DEBUG) {
        cout << "Cache (connections) size:" << connections.size() << endl;
        cout << "Cache (connections_age) size:" << connections_age.size() << endl;
    }
    
    time_t cur_time = time(NULL);
    map<string, time_t>::iterator iter;
    for (iter = connections_age.begin(); iter != connections_age.end(); iter++) {
        if (iter->second + CONNECTION_TIMEOUT < cur_time) {
            key = iter->first;
            sock_fd = getFromConnections(iter->first);
            close(sock_fd);
            removeFromConnections(key);
            if (DEBUG) cout << "Closed socket:" << sock_fd << endl;
        }
    }

    if (DEBUG) {
        cout << "Cache (connections) size:" << connections.size() << endl;
        cout << "Cache (connections_age) size:" << connections_age.size() << endl;
    }
}

/***************************************
 *       Cache clients functions       *
 **************************************/

void Cache::addToClients(int sock_fd) {
    clients.erase(sock_fd);
    time_t cur_time = time(NULL);
    clients.insert(map<int, time_t>::value_type(sock_fd, cur_time));
}

void Cache::removeFromClients(int sock_fd) {
    clients.erase(sock_fd);
}

void Cache::cacheClientCleanup() {
    if (DEBUG) {
        cout << "In cacheClientCleanup()..." << endl;
        cout << "Cache (clients) size:" << clients.size() << endl;
    }
    
    int sock_fd = 0;
    time_t cur_time = time(NULL);
    map<int, time_t>::iterator iter;
    for (iter = clients.begin(); iter != clients.end(); iter++) {
        if (iter->second + CONNECTION_TIMEOUT < cur_time) {
            if (DEBUG) cout << "cached time:" << iter->second + CONNECTION_TIMEOUT << " vs current:" << cur_time << endl;
            sock_fd = iter->first;
            close(sock_fd);
            removeFromClients(sock_fd);
            if (DEBUG) cout << "Closed socket:" << sock_fd << endl;
        }
    }
    if (DEBUG) {
        cout << "Cache (clients) size:" << clients.size() << endl;
    }
}