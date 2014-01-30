/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

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

#include "http-request.h"
#include "http-response.h"
#include "http-connection.h"

// Boost
#include <boost/thread.hpp>
#include <boost/bind.hpp>

using namespace std;

#define BUFFER_SIZE 1024


class Page{

private:
	time_t expire;
    string lastModify;
    string eTag;
    string data;
	
public:
    Page(time_t expTime, string lastMod, string entityTag, string dat){
        expire = expTime;
        eTag = entityTag;
        lastModify = lastMod;
        data = dat;
    }
	string getLastModify(){
        return lastModify;
    }
    
    time_t getExpire(void){
        return expireTime;
    }
	 
	string getETag(){
        return eTag;
    }
    
    string getData(void){
        return data;
    }
    
    bool isExpired(){
        time_t current = time(&timer); //get current time
		//debug
        //cout << "Difference: "<<difftime(expireTime, current)<<" \n Current: "<<current<<" \n Expire: " <<expireTime << endl;
        if(difftime(expireTime, current)<0)
            return true;
        else
            return false;
    }
};

// do some parsing for the cache class
long cacheParse(string s){ //cache-control: public, private, no cache, no store
    if(s.find("public")!=string::npos || s.find("private")!=string::npos || s.find("no-cache")!=string::npos || s.find("no-store")!=string::npos)
        return 0; //string::npos max value for size_t- could also use -1
		
    size_t position = string::npos;
    if ((pos = s.find("max-age"))!=string::npos) {
        size_t begin = s.find('=');
        if(begin != string::npos){ //long vs int
            long maxAge = atol(s.substr(begin+1).c_str());
			//debug
			//cout << "max time: " <<time<< endl;
            return maxAge;
        }
    }
    return 0;
}


class Cache{

private:
    map<string, Page> store;
	
public:
	
    Page* get(string URL){
        map<string,Page>::iterator iter;
        iter = store.find(URL); // check if the URL is stored in the cache
        if(iter == store.end())  // if not return null
			return NULL;
        else 
			return &iter->second; // if it is, return the Page
    }
    
	void remove(string URL){ // remove when time expires
        store.erase(URL);
    }
	
    void add(string URL, Page webpg){ // add to cache
        store.erase(URL);
        store.insert(map<string, Page>::value_type(URL, webpg));
    }
} cache; // declare cache as a cache object

/* How to implement:

//we may also have to add the locking and unlocking of threads, but since I'm not entirely sure how that works, so I didn't put it in

if (status==200)
{
	string expireTime = reply.FindHeader("Expires");
	string cacheCheck = reply.FindHeader("Cache-Control");
	string eTag = reply.FindHeader("ETag");
	string date = reply.FindHeader("Date");
	string lastMod = reply.FindHeader("Last-Modified");
	//or whatever our HttpResponse file is called now

	time_t current=time(&time);
	time_t expireT;

	if(expireTime!=""){ //page is not in the cache
	//error check
		if((expireT=convertTime(expireTime))!=0 && difftime(expireT, current)>0)
		{
			//add to cache with expire time
		}
		else{
			//expire time couldn't be read?? do not add to cache
		}
	}
	else if (cacheCheck!=""){
		//long vs int
		long maxT =0;
		if((maxT=cacheParse(cacheCheck))!=0 && date!="")
		{
			expireT = converTime(date)+maxT;
			//add to cache w/ cache control
			cache.add(URL, pg);
		}
		else
		{
			cache.remove(URL);
		}
	}
	else{ // no data on cache
		cache.remove(URL);
	}
}
else if(status== "something else"){ // don't know if we're gonna implement any others...
}
*/


/*@brief Fetch data from remote host
 */
int fetchFromRemoteHost(int sock_fd, string& response)
{
    char buffer[BUFFER_SIZE];
    while (1) {
        int recv_num = recv(sock_fd, buffer, sizeof(buffer), 0);
        if (recv_num < 0) {
            return -1;
        }
        else if (recv_num == 0) {
            break;
        }
        response.append(buffer, recv_num);
    }
    return 0;
}

/*@brief Handles connection for each client
 */
void connectionHandler(int sock_fd)
{
    
    // Buffer
    char buffer[BUFFER_SIZE];
    string temp;
    int rv;
    
    struct pollfd ufds;
    ufds.fd = sock_fd; // open socket # corresponding to the connection
    ufds.events = POLLIN;
    
    while (1) {
        temp = "";
        for(int i = 0; i < BUFFER_SIZE; i++) buffer[i] = '\0'; //make sure it's empty- not sure if necessary
        
        //wait for events on the sockets with a half second timeout
        while((rv = poll(&ufds, 1, 500)) > 0 && recv(sock_fd, buffer, sizeof(buffer), 0) > 0)
        {
            temp.append(buffer);
        }
        
        HttpRequest request;
        
        if(temp.size() == 0) { // No data => sleep for 1 second then try again
            if (DEBUG) cout << "(Client thread) Still waiting..." << endl;
            sleep(1);
            continue;
        }

        try {
            if (DEBUG) cout << "In try..." << endl;
            request.ParseRequest (temp.c_str(), temp.size());
            
            
            if (DEBUG) {
                cout <<"Original \t \t "<<endl<<temp<<endl<<" \t \t Parsed \t"<<endl;
                
                cout <<"GET "<<request.GetHost()<<":"<<request.GetPort()<<request.GetPath()
                <<" HTTP/"<<request.GetVersion()<<endl;
                
                string header;
                if((header = request.FindHeader("Connection")).size() > 0)
                    cout<<"Connection: "<<header<<endl;
                if((header = request.FindHeader("User-Agent")).size() > 0)
                    cout<<"User-Agent: "<<header<<endl;
            }
            
            
            // If request in cache,
            
            // Else (not in cache, request to remote server)
            
                // Make client connection to remote server
            if (DEBUG) cout << "Making client connection from proxy to remote server..." << endl;
            int remote_fd = makeClientConnection(request.FindHeader("Host").c_str(), REMOTE_PORT);
            if (remote_fd < 0) {
                fprintf(stderr, "Fail to connect to remote server.\n");
                // Exit?
            }
            
            
            // Send request
            char request_string [request.GetTotalLength()];
            request.FormatRequest(request_string);
            if (send(remote_fd, request_string, request.GetTotalLength(), 0) == -1) {
                close(remote_fd);
                // Exit?
            }
            
                // Get data from remote host
            string response;
            if (fetchFromRemoteHost(remote_fd, response) < 0) {
                // Error with response
                // Exit?
            }
            
            if (DEBUG) cout << "Reponse string:" << response << endl;
                // Store in cache
                
                // Return request to original client
                
                // Close connection to remote server
            
            
            if(strcmp(request.FindHeader("Connection").c_str(), "close") == 0 ||
               strcmp(request.GetVersion().c_str(), "1.0") == 0)
                break; // If the connection is not persisent => close
        }
        catch (ParseException e) {
            const char* input = e.what(); //returns msg.c_str()
            HttpResponse reply;
            reply.SetVersion("1.1");
            
            cout<<"Parse exception: "<<e.what()<<endl;
            
            if(strcmp(input, "Request is not GET") == 0) {
                reply.SetStatusCode("501");
                reply.SetStatusMsg("Not Implemented");
            } else {
                reply.SetStatusCode("400");
                reply.SetStatusMsg("Bad Request");
            }
            
            char buff[reply.GetTotalLength()];
            reply.FormatResponse(buff);
            send(sock_fd, buff, reply.GetTotalLength(), 0); //
        }
    }
    
    if (DEBUG) cout << "Closing connection..." << endl;
    close(sock_fd);
}


int main (int argc, char *argv[])
{
    // Create server listener
    int sockfd;
    if ((sockfd = makeServerConnection(PROXY_PORT)) < 0) {
        fprintf(stderr, "fail to make server\n");
        return 1;
    }

    printf("Proxy server: waiting for connections...\n");
    
    
    // TODO: initialize cache
    
    
    // Create Boost
    boost::thread_group t_group;
    
    // Accept incoming connections
    while (1) {
        struct sockaddr_storage their_addr; // Connector's address info
        socklen_t sin_size = sizeof their_addr;
        char s[INET6_ADDRSTRLEN];
        
        int new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);  // New connection
        if (new_fd == -1) {
            perror("accept");
            continue;
        }
        else {
            inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof s);
            if (DEBUG) printf("server: got connection from %s\n", s);
            
            // Using Boost
            if (DEBUG) cout << "Making new thread with boost..." << endl;
            t_group.create_thread(boost::bind(&connectionHandler, new_fd));
            
        }
        
        //send(new_fd, "Hello and Goodbye\n", 17, 0);
        /*
        if (!fork()) { // this is the child process
            close(sockfd); // child doesn't need the listener
            if (send(new_fd, "Hello, world!", 13, 0) == -1)
            perror("send");
            close(new_fd);
            exit(0);
        }*/
        
        
        /* pthread implementation
        // Make thread to handle connection
        int *new_sock = (int *)malloc(sizeof(1));
        *new_sock = new_fd;
        pthread_t newThread;
        pthread_create(&newThread, NULL, connectionHandler, (void *)new_sock); 
         */
    }

    return 0;
}
