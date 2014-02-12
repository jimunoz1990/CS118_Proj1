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
#include "http-cache.h"
#include "http-page.h"

// Boost
#include <boost/thread.hpp>
#include <boost/bind.hpp>

using namespace std;

#define BUFFER_SIZE 1024
#define TIMEOUT 100

static Cache cache;


/* TODO List:
 * 1) Add logic to cache unless specified not to in response header
 * 2) DONE, SK (Add additional cache to keep track of proxy->remote server connection)
 * 3) DONE, SK (Implement proxy->remote server connection caching)
 * 4) Implement conditional GET
 */


/* TODO: clear up this code
// Do some parsing for the cache class
long cacheParse(string s) { //cache-control: public, private, no cache, no store
    if(s.find("public")!=string::npos || s.find("private")!=string::npos || s.find("no-cache")!=string::npos || s.find("no-store")!=string::npos)
        return 0; //string::npos max value for size_t- could also use -1
		
    size_t position = string::npos;
    if ((position = s.find("max-age"))!=string::npos) {
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

// How to implement:
time_t timeConvert(string s){
    const char* format = "%a, %d %b %Y %H:%M:%S %Z";
    struct tm tm;
    if(strptime(s.c_str(), format, &tm)==NULL){
    	return 0;
    }
    else{
    	tm.tm_hour = tm.tm_hour-8;//to la time
    	return mktime(&tm);
    }
}


//reply is our http-response

//Adding the page to the cache
//Taking into account expiretime and cache-control

string status=reply.GetStatusCode();

if (status=="200") // OK status
{
	string expireTime = reply.FindHeader("Expires");
	string cacheCheck = reply.FindHeader("Cache-Control");
	string eTag = reply.FindHeader("ETag");
	string date = reply.FindHeader("Date");
	string lastMod = reply.FindHeader("Last-Modified");

	time_t current=time(NULL);
	time_t expireT;

	if(expireTime!=""){ //page has an expiration time
		if((expireT=timeConvert(expireTime))!=0 && difftime(expireT, current)>0)
		{// add to cache with normal expiration time
			Page pg(expireTime, lastMod, eTag, data);
			cache.cache_store_mutex.lock();
			cache.addToStore(URL, pg);
			cache.cache_store_mutex.unlock();
		}
		else{ // expire exists but is not valid
			cache.cache_store_mutex.lock();
			cache.removeFromStore(URL, pg);
			cache.cache_store_mutex.unlock();
		}
	}
	else if (cacheCheck!=""){ // if there is a max-age field
		long maxT =0;
		if((maxT=cacheParse(cacheCheck))!=0 && date!="")
		{//add to cache using cache-control
			expireT = converTime(date)+maxT; //implement max-age
			Page pg(expireT, lastMod, eTag, data);
			cache.cache_store_mutex.lock();
			cache.addToStore(URL, pg);
			cache.cache_store_mutex.unlock();
		}
		else
		{// cache not enabled
			cache.cache_store_mutex.lock();
			cache.removeFromStore(URL);
			cache.cache_store_mutex.unlock();
		}
	}
	else{ // no data on cache
		cache.cache_store_mutex.lock();
		cache.removeFromStore(URL);
		cache.cache_store_mutex.unlock();
	}
}
else if(status== "304"){ // Not-Modified
        cache.cache_store_mutex.lock();
        data = cache.getFromStore(url)->getData();
		cache.cache_store_mutex.unlock();
}
// ********end adding to cache


//Conditional GET

string fetchResponse(HttpRequest request)
{
	cache.cache_store_mutex.lock();
	Page* pg = cache.get(url);
	if(pg!=NULL){ // if page was in cache
	  if (!pg->isExpired()){ // page in cache and not expired
		  string a = pg->getData(); // get the data from the cache
		  cache.cache_store_mutex.unlock();
		  return a;
	  }
	  else{// page is in cache, but expired
		  if (pg->getETag() != "") { //use the e-tag if available
			  request.AddHeader("If-None-Match", pg->getETag());
		  }
		  else if(pg->getLastModify() !=""){// use last modified version
			  request.AddHeader("If-Modified-Since", pg->getLastModify());
		  }
		  cache.cache_store_mutex.unlock();
		  return ;// fetch response from server
	  }
	}
	else{
		cache.cache_store_mutex.unlock();
		return ; // fetch from the server
	}
}*/


/*@brief Fetch data from remote host
 */
int fetchFromRemoteHost(int sock_fd, string& response)
{
    if (DEBUG) cout << "In fetchFromRemoteHost..." << endl;

    int count = 0;
    char buffer[BUFFER_SIZE];
    int recv_num;
    
    struct pollfd ufds;
    ufds.fd = sock_fd;
    ufds.events = POLLIN;
    
    while (1) {
        if (DEBUG) cout << "Receive attempt:" << count << endl;
        
        while (poll(&ufds, 1, TIMEOUT) > 0) {
            recv_num = recv(sock_fd, buffer, sizeof(buffer), 0);
            if (DEBUG) cout << "Recv_num:" << recv_num << endl;
            if (recv_num < 0) {
                if (DEBUG) cout << "Receive error" << endl;
                return -1;
            }
            else if (recv_num == 0) {
                if (DEBUG) cout << "Rec = 0" << endl;
                break;
            }
            response.append(buffer, recv_num);
            if (DEBUG) cout << response << endl;
        }
        // Retry if count < 35 and no response
        if (count < 35 && response.size() == 0) {
            usleep(100000);
            count++;
        } else {
            break;
        }
    }
    if (response.size() == 0) {
        if (DEBUG) cout << "Empty fetch..." << endl;
        return -1;
    }
    else {
        return 1;
    }
}


/*@brief Make remote request connection
 */
void makeRequestConnection(HttpRequest request, int sock_fd)
{
    if (DEBUG) cout << "\nIn makeRequestConnection..." << endl;
    
    char port[6];
    sprintf(port, "%u", request.GetPort());
    
    // Check cache for existing proxy->remote server connection
    
    string connections_key = request.GetHost() + ":" + port;
    if (DEBUG) cout << "Checking if:" << connections_key << " is in connections cache..." << endl;
    
    cache.cache_connections_mutex.lock();
    int proxy_server_sock_fd = cache.getFromConnections(connections_key);
    cache.cache_connections_mutex.unlock();
    
    int remote_fd;
    
    // If proxy->remote-server connection exits
    if (proxy_server_sock_fd > 0) {
        if (DEBUG) cout << "Previous connection exists, sock_fd:" << proxy_server_sock_fd << endl;
        // Check if connection is still alive
        
        int poll_status;
        struct pollfd ufds;
        ufds.fd = proxy_server_sock_fd;
        ufds.events = POLLRDHUP;
        if ((poll_status = poll(&ufds, 1, 0)) == 0) {
            if (DEBUG) cout << "Connection alive" << endl;
            remote_fd = proxy_server_sock_fd;
        } else  {
            if (DEBUG) cout << "Connection dead, poll_status:" << poll_status << endl;
            // Close dead connection sock_fd
            close(proxy_server_sock_fd);
            
            // Remove from cache connections
            cache.cache_connections_mutex.lock();
            cache.removeFromConnections(connections_key);
            cache.cache_connections_mutex.unlock();
            
            // Make client connection
            remote_fd = makeClientConnection(request.GetHost().c_str(), port);
            if (remote_fd < 0) {
                perror("Remote connection");
                //close(sock_fd);
            }
            
            // Cache proxy->remote-server connection sock_fd
            if (DEBUG) cout << "Caching proxy->remote-server connection" << endl;
            string connection_key = request.GetHost();
            cache.cache_connections_mutex.lock();
            cache.addToConnections(connections_key, remote_fd);
            cache.cache_connections_mutex.unlock();
        }
    }
    // Else, make a new one
    else {
        // Make client connection
        remote_fd = makeClientConnection(request.GetHost().c_str(), port);
        if (remote_fd < 0) {
            perror("Remote connection");
            //close(sock_fd);
        }
        
        // Cache proxy->remote-server connection sock_fd
        if (DEBUG) cout << "Caching proxy->remote-server connection" << endl;
        string connection_key = request.GetHost();
        cache.cache_connections_mutex.lock();
        cache.addToConnections(connections_key, remote_fd);
        cache.cache_connections_mutex.unlock();
    }
    
    // Send request to remote server
    if (DEBUG) cout << "Sending request to remote server..." << endl;
    char request_string [request.GetTotalLength()];
    request.FormatRequest(request_string);
    if (send(remote_fd, request_string, request.GetTotalLength(), 0) == -1) {
        if (DEBUG) cout << "Error with send." << endl;
        perror("Send");
        close(remote_fd);
        // Exit?
    }
    
    // Get data from remote host
    if (DEBUG) cout << "Fetching data from server..." << endl;
    string response_string;
    if (fetchFromRemoteHost(remote_fd, response_string) < 0) {
        perror("Fetch");
    }
    
    //if (DEBUG) cout << "Reponse string:" << response_string << endl;
    
    // Return response, as-in from remote server, to original client
    if (DEBUG) cout << "Sending from:" << connections_key << " back to original client." << endl;
    if (send(sock_fd, response_string.c_str(), response_string.size(), 0) == -1) {
        perror("Send");
    }
    
    /* TODO: Need to add logic to add cache only if specified in header
     */
    
    // Format response)string into HttpResponse
    HttpResponse response;
    response.ParseResponse(response_string.c_str(), response_string.size());
    
    string expireTime = response.FindHeader("Expires");
	string lastMod = response.FindHeader("Last-Modified");
	string eTag = response.FindHeader("ETag");
    Page thisPage = Page(expireTime, lastMod, eTag, response_string);
    string key = request.GetHost() + "/" + request.GetPath();
    
    cache.cache_store_mutex.lock();
    cache.addToStore(key, thisPage);
    cache.cache_store_mutex.unlock();
}

/*@brief Handles connection for each client thread
 */
void connectionHandler(int sock_fd)
{
    char buffer[BUFFER_SIZE];
    string temp;
    int rv;
    
    struct pollfd ufds;
    ufds.fd = sock_fd; // Open socket # corresponding to the connection
    ufds.events = POLLIN;
    
    while (1) {
        if (DEBUG) cout << "While looping.." << endl;
        temp = "";
        for(int i = 0; i < BUFFER_SIZE; i++) buffer[i] = '\0'; //make sure it's empty- not sure if necessary
        
        // Wait for events on the sockets with a half second timeout
        while((rv = poll(&ufds, 1, 500)) > 0 && recv(sock_fd, buffer, sizeof(buffer), 0) > 0) {
            temp.append(buffer);
        }
        
        HttpRequest request;

        if(temp.size() == 0) { // No data => sleep for 1 second then try again
            if (DEBUG) cout << "(Client thread) Still waiting..." << endl;
            sleep(1);
            continue;
        }

        try {
            if (DEBUG) cout << "Trying..." << endl;
            request.ParseRequest (temp.c_str(), temp.size());
            
            if (DEBUG) {
                cout << "\t \t Original" << endl << temp << endl << endl;
                cout << " \t \t Parsed" <<endl;
                cout << "GET " << request.GetHost() << ":" << request.GetPort() << request.GetPath()
                << " HTTP/" << request.GetVersion() << endl;
                string header;
                if((header = request.FindHeader("Connection")).size() > 0)
                    cout << "Connection: " << header << endl;
                if((header = request.FindHeader("User-Agent")).size() > 0)
                    cout << "User-Agent: " << header << endl;
                cout << endl;
            }
            
            // Cache logic
            string key = request.GetHost() + "/" + request.GetPath();
            if (DEBUG) cout << "Checking if:" << key << " is in cache..." << endl;
            
            // Check cache for request
            cache.cache_store_mutex.lock();
            Page * this_page = cache.getFromStore(key);
            cache.cache_store_mutex.unlock();
            
            // If request in cache
            if (this_page) {
                if (DEBUG) cout << "Client request in cache." << endl;
                
                // Conditional-Get logic
                
                
                // Get from cache and send to client
                if (send(sock_fd, this_page->getData().c_str(), this_page->getData().size(), 0) == -1) {
                    perror("Send");
                }
            }
            // If request not in cache
            else {
                if (DEBUG) cout << "Client request not in cache. Calling makeRequestConnection to host:" << request.GetHost() << " port:" << request.GetPort() << endl;
                
                // Create new thread to handle proxy->remote server connection
                boost::thread remote_thread(makeRequestConnection, request, sock_fd);
                if (DEBUG) cout << "Waiting for remote thread..." << endl;
                // Wait for remote thread to exit
                remote_thread.join();
                
                // Close connection to remote server
                if(strcmp(request.FindHeader("Connection").c_str(), "close") == 0 ||
                   strcmp(request.GetVersion().c_str(), "1.0") == 0)
                    break; // If the connection is not persisent => close
                
                if (DEBUG) cout << "Done processing..." << endl;
            }
        }
        catch (ParseException e) {
            const char* input = e.what(); //returns msg.c_str()
            HttpResponse reply;
            reply.SetVersion("1.1");
            
            cout << "Parse exception: " << e.what() <<endl;
            
            if(strcmp(input, "Request is not GET") == 0) {
                reply.SetStatusCode("501");
                reply.SetStatusMsg("Not Implemented");
            } else {
                reply.SetStatusCode("400");
                reply.SetStatusMsg("Bad Request");
            }
            
            char buff[reply.GetTotalLength()];
            reply.FormatResponse(buff);
            send(sock_fd, buff, reply.GetTotalLength(), 0);
        }
    }
    
    if (DEBUG) cout << "** Closing connection sock_fd:" << sock_fd << endl;
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
    cout << "Proxy server: waiting for connections..." << endl;
    
    // Create Boost
    boost::thread_group t_group;
    
    // Accept incoming connections
    while (1) {
        struct sockaddr_storage their_addr; // Connector's address info
        socklen_t sin_size = sizeof their_addr;
        char s[INET6_ADDRSTRLEN];
        
        // Accept new connection
        int new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd < 0) {
            perror("accept");
            continue;
        }
        else {
            inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof s);
            if (DEBUG) printf("server: got connection from %s\n", s);
            
            // Make new Boost thread in group
            if (DEBUG) cout << "Making new thread with boost..." << endl;
            t_group.create_thread(boost::bind(&connectionHandler, new_fd));
        }
    }

    close(sockfd);
    return 0;
}
