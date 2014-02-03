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

Page* Cache::get(string URL) {
    map<string,Page>::iterator iter;
    iter = store.find(URL); // check if the URL is stored in the cache
    if(iter == store.end())  // if not return null
        return NULL;
    else
        return &iter->second; // if it is, return the Page
}

void Cache::remove(string URL) {
    store.erase(URL);
}

void Cache::add(string URL, Page webpg) {
    store.erase(URL);
    store.insert(map<string, Page>::value_type(URL, webpg));
}