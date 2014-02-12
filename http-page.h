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

class Page{
    
private:
    time_t expireTime;
    string lastModify;
    string eTag;
    string data;
    
public:
    Page(time_t expTime, string lastMod, string entityTag, string dat);
    
    string getLastModify();
    
    time_t getExpire(void);
    
    string getETag();
    
    string getData(void);
    
    bool isExpired();
    
};

#endif /* defined(____http_page__) */
