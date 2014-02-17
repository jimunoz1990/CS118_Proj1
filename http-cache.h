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
    
    /*@brief Caches each proxy->remote-server connection and when connection was last used.
     *       Key: connection sock_fd, Value: time_t when last used
     */
    map<string, time_t> connections_age;
    
    /*@brief Caches each client->proxy connections and when connection was last active.
     *       Key: connection sock_fd, Value: time_t when last used
     */
    map<int, time_t> clients;
	
public:
    /*@brief Get value from cache store
     *@param URL [in] URL key
     *@returns Page corresponding to URL key
     */
    Page* getFromStore(string URL);
    
    /*@brief Remove <URL, Page> from cache store
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
    
    /*@brief Remove <URL, scok_fd> from cache connections and corresponding entry in connections_age
     */
    void removeFromConnections(string URL);
    
    /*@brief Cache connections replacement policy
     * When the number of proxy->remote-server connections surpasses MAX_SERVER_CONNECTIONS (100),
     * removes the oldest connection from the cache
     */
    void cacheReplacementPolicy();
    
    /*@brief Cache connection cleanup
     * Checks proxy->remote-server connections. If the connection's time-since-last-used 
     * surpasses CONNECTION_TIMEOUT (120 seconds), then the connection will be closed.
     */
    void cacheConnectionCleanup();
    
    /*@brief Add sock_fd to cache clients
     */
    void addToClients(int sock_fd);
    
    /*@brief Remove sock_fd from cache clients
     */
    void removeFromClients(int sock_fd);
    
    /*@brief Cache client cleanup
     * Checks client->proxy connections. If the connection's time-since-last-active
     * surpasses CONNECTION_TIMEOUT (120 seconds), then the connection will be closed.
     */
    void cacheClientCleanup();
    
    /*@brief Iterates through cached connections and closes them all
     */
    void killAll();

    /*@Returns number of persistent client connections we have open.
     */
    int getNumClients();
    
    /*@brief Mutex locks for cache store, cache connections/connections age, and cache clients
     */
    boost::mutex cache_store_mutex;
    boost::mutex cache_connections_mutex;
    boost::mutex cache_clients_mutex;
    
};

#endif /* defined(____http_cache__) */
