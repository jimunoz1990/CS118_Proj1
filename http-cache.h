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

#include "http-page.h"

// Boost
#include <boost/thread.hpp>
#include <boost/bind.hpp>

using namespace std;

class Cache {
    
private:
    
    map<string, Page> store;
	
public:
    
    /*@brief Get value from cache
     */
    Page* get(string URL);
    
    /*@brief Remove from cache
     */
	void remove(string URL);
    
    /*@brief Add to cache
     */
    void add(string URL, Page webpg);
    
    boost::mutex cache_mutex;
    
}; //cache; // declare cache as a Cache object

#endif /* defined(____http_cache__) */
