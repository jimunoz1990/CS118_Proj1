/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * @file Header file for http-page
 *
 */

#ifndef ____http_page__
#define ____http_page__

#include <iostream>
#include <string>
#include <string.h>
#include <time.h>

using namespace std;

/*@brief Page is used to store the data returned from remote-server.
 * It is used in conjunction with the Cache class.
 */
class Page{
    
private:
    time_t expireTime;
    string lastModify;
    string eTag;
    string data;
    
public:
    /*@brief Constructor
     */
    Page(time_t expTime, string lastMod, string entityTag, string dat);
    
    
    /************************
     *   Getter functions   *
     ***********************/
    string getLastModify();
    time_t getExpire();
    string getETag();
    string getData();
    
    /*@brief Checks if time of page is expired
     */
    bool isExpired();
    
};

#endif /* defined(____http_page__) */
