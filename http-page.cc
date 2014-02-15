/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * @file Implementation for page class
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

Page::Page(time_t expTime, string lastMod, string entityTag, string dat){
    expireTime = expTime;
    eTag = entityTag;
    lastModify = lastMod;
    data = dat;
}

string Page::getLastModify(){
    return lastModify;
}

time_t Page::getExpire(void){
    return expireTime;
}

string Page::getETag(){
    return eTag;
}

string Page::getData(void){
    return data;
}

bool Page::isExpired(){
    // Get current time
    time_t current = time(NULL);
    if (DEBUG) cout << "Difference: "<< difftime(expireTime, current) << " \n Current: " << current <<" \n Expire: " <<expireTime << endl;
    if (difftime(expireTime, current) < 0)
        return true;
    else
        return false;
}