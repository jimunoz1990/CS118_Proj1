/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * @file Header file for http-cache
 *
 */

#ifndef ____http_cache__
#define ____http_cache__

#include <iostream>
#include <string>
#include <string.h>
#include <time.h>

#include "http-page.h"

// Boost
#include <boost/thread.hpp>
#include <boost/bind.hpp>

using namespace std;

class Cache {
    
private:
    /*@brief Caches pages from remote-server
     *       Key: URL, Value: instance of Page class
     */
    map<string, Page> store;
    
    /*@brief Caches proxy->remote-server connections
     *       Key: URL, Value: connection sock_fd
     */
    map<string, int> connections;
    
    /*@brief Caches
     *       Key: connection sock_fd, Value: time_t when last used
     */
    map<string, time_t> connections_age;
	
public:
    /*@brief Get value from cache store
     */
    Page* getFromStore(string URL);
    
    /*@brief Remove from cache store
     */
	void removeFromStore(string URL);
    
    /*@brief Add to cache store
     */
    void addToStore(string URL, Page webpg);
    
    /*@brief Add proxy->remote-server connection sock_fd to connections
     */
    void addToConnections(string URL, int sock_fd);
    
    /*@brief Return sock_fd for existing proxy->remote-server connection
     */
    int getFromConnections(string URL);
    
    /*@brief Remove from cache connections and corresponding entry in connections_age
     */
    void removeFromConnections(string URL);
    
    /*@brief Cache connections replacement policy
     */
    void cacheReplacementPolicy();
    
    /*@brief Cache cleanup
     */
    void cacheConnectionCleanup();
     
    boost::mutex cache_store_mutex;
    boost::mutex cache_connections_mutex;
    
}; //cache; // declare cache as a Cache object

#endif /* defined(____http_cache__) */
