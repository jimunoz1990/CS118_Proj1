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

#define DEBUG 0
#define BUFFER_SIZE 1024
#define TIMEOUT 100
#define CHECK_CONNECTION_TIMEOUT 10

static Cache cache;
static int running = 1;

/* TODO List:
 * 1) DONE, JM Add logic to cache unless specified not to in response header
 * 2) DONE, SK (Add additional cache to keep track of proxy->remote server connection)
 * 3) DONE, SK (Implement proxy->remote server connection caching)
 * 4) DONE, JM Implement conditional GET
 * 5) DONE, SK Limit proxy->server connections to 100
 * 6) Review error logic (close corresponding socket or not)
 * 7) Timeout for proxy->server threads
 */

/*@brief Interrupt handler to handle graceful exit of proxy server upon user input
 */
void exitHandler()
{
    if (DEBUG) cout<< "In exitHander!" << endl;
    while (1)
    {
        char ch = cin.get();
        if (ch == 'x')
        {
            if (DEBUG) cout << "X key was pressed. Cleaning up..." << endl;
            // Close all connections
            cache.killAll();
            running = 0;
            break;
        }
        sleep(1);
    }
}

/*@brief Converts a time from string format to time_t
 */
time_t timeConvert(string s){
    const char* format = "%a, %d %b %Y %H:%M:%S %Z";
    struct tm tm = {0};
    if(strptime(s.c_str(), format, &tm) == NULL){
        return 0;
    }
    else{
        tm.tm_hour = tm.tm_hour-8; // To LA time
        return mktime(&tm);
    }
}

/*@brief Parse response header to find cache-control: public, private, no cache, no store
 */
long cacheParse(string s) {
    if(s.find("public") != string::npos || s.find("private") != string::npos || s.find("no-cache") != string::npos || s.find("no-store") != string::npos)
        return 0; // String::npos max value for size_t, could also use -1
    
    size_t position = string::npos;
    if ((position = s.find("max-age")) != string::npos) {
        size_t begin = s.find('=');
        if(begin != string::npos) { // Long vs int
            long maxAge = atol(s.substr(begin+1).c_str());
            //if (DEBUG) cout << "max time: " << time << endl;
            return maxAge;
        }
    }
    return 0;
}

/*@brief Fetch data from remote host
 *@param sock_fd [in] Socket of the proxy->remote-server connection
         string [out] Response from the remote-server
 *@returns 1 on success and -1 on failure
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


/*@brief Make remote server connection
 * Function gets called if client request is not in cache or is expired.
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
        
        int poll_status;
        struct pollfd ufds;
        ufds.fd = proxy_server_sock_fd;
        ufds.events = POLLRDHUP;
        
        // Check if connection is still alive
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
    
    // Format response)string into HttpResponse
    HttpResponse response;
    response.ParseResponse(response_string.c_str(), response_string.size());
    
    string URL = request.GetHost() + ":" + port + "/" + request.GetPath();
    string status = response.GetStatusCode();
    
    if (status == "304") {
        if (DEBUG) cout << "HTTP:304 back from remote server, get data from cache and return to client." << endl;
        string cache_data = cache.getFromStore(URL)->getData();
        if (send(sock_fd, cache_data.c_str(), cache_data.size(), 0) == -1) {
            perror("Send");
        }
    }
    // Else return response, as-in from remote server, to original client
    else {
        if (DEBUG) cout << "Sending from:" << connections_key << " back to original client." << endl;
        if (send(sock_fd, response_string.c_str(), response_string.size(), 0) == -1) {
            perror("Send");
        }
    }
    
    // Caching logic
    if (status == "200") // OK status
    {
        string expireTime = response.FindHeader("Expires");
        string cacheCheck = response.FindHeader("Cache-Control");
        string eTag = response.FindHeader("ETag");
        string date = response.FindHeader("Date");
        string lastMod = response.FindHeader("Last-Modified");
        
        time_t current=time(NULL);
        time_t expireT;
        
        // Page has an expiration time
        if (expireTime != "") {
            if((expireT=timeConvert(expireTime))!=0 && difftime(expireT, current) > 0)
            { // Add to cache with normal expiration time
                if (DEBUG) cout << "Returned page response has expireT:" << expireT << " current:" << current << endl;
                Page pg = Page(timeConvert(expireTime), lastMod, eTag, response_string);
                cache.cache_store_mutex.lock();
                cache.addToStore(URL, pg);
                cache.cache_store_mutex.unlock();
            }
            else{ // Expiration time exists but is not valid
                if (DEBUG) cout << "Expire time is invalid, expireT:" << expireT << " current:" << current << endl;
                cache.cache_store_mutex.lock();
                cache.removeFromStore(URL);
                cache.cache_store_mutex.unlock();
            }
        }
        // If there is a max-age field
        else if (cacheCheck != "") {
            long maxT =0;
            if((maxT=cacheParse(cacheCheck))!=0 && date!="")
            {
                // Add to cache using cache-control
                expireT = timeConvert(date)+maxT;
                Page pg = Page(expireT, lastMod, eTag, response_string);
                cache.cache_store_mutex.lock();
                cache.addToStore(URL, pg);
                cache.cache_store_mutex.unlock();
                if (DEBUG) cout << "Max-age field maxT:" << maxT << endl;
                
            }
            else
            {
                // Cache not enabled
                if (DEBUG) cout << "Cache not enabled" << endl;
                cache.cache_store_mutex.lock();
                cache.removeFromStore(URL);
                cache.cache_store_mutex.unlock();
            }
        }
        // No expiration date
        else{
            if (DEBUG) cout << "Response has no expiration date." << endl;
            cache.cache_store_mutex.lock();
            Page pg = Page(timeConvert(expireTime), lastMod, eTag, response_string);
            cache.addToStore(URL, pg);
            cache.cache_store_mutex.unlock();
        }
    }
    // Not-Modified
    else if (status== "304"){
        // Do nothing
    }
}

/*@brief Handles connection for each client thread
 *@params sock_fd [in] Socket for the client->proxy connection
 */
void connectionHandler(int sock_fd)
{
    char buffer[BUFFER_SIZE];
    string temp;
    int rv;
    
    struct pollfd ufds;
    ufds.fd = sock_fd; // Open socket # corresponding to the connection
    ufds.events = POLLIN;
    
    // Accept client requests
    while (1) {
        temp = "";
        for(int i = 0; i < BUFFER_SIZE; i++) buffer[i] = '\0';
        
        // Wait for events on the socket with a half second timeout
        while((rv = poll(&ufds, 1, 500)) > 0 && recv(sock_fd, buffer, sizeof(buffer), 0) > 0) {
            temp.append(buffer);
        }
        
        HttpRequest request;
        
        // If there is no data, sleep for 1 sec and try again
        if(temp.size() == 0) {
            sleep(1);
            continue;
        }
        
        // Update client connection last-active-time upon reciving a request
        cache.cache_clients_mutex.lock();
        cache.addToClients(sock_fd);
        cache.cache_clients_mutex.unlock();
        
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
            char port[6];
            sprintf(port, "%u", request.GetPort());
            
            string key = request.GetHost() + ":" + port + "/" + request.GetPath();
            if (DEBUG) cout << "Checking if:" << key << " is in cache..." << endl;

            // Check cache for request
            cache.cache_store_mutex.lock();
            Page * this_page = cache.getFromStore(key);
            
            // If request is in cache
            if (this_page) {
                if (DEBUG) cout << "Client request in cache." << endl;
                
                // Conditional-GET logic
                // If page is in the cache and not expired
                if (!this_page->isExpired()) {
                    if (DEBUG) cout << "Page in cache and not expired." << endl;
                    cache.cache_store_mutex.unlock();
                    // Get page from cache and send to client
                    if (DEBUG) cout << "Sending page from cache back to client" << endl;
                    if (send(sock_fd, this_page->getData().c_str(), this_page->getData().size(), 0) == -1) {
                        perror("Send");
                    }
                }
                // Page is in cache, but expired
                else {
                    if (DEBUG) cout << "Page is in cache but expired." << endl;
                    if (this_page->getETag() != "") { //use the e-tag if available
                        request.AddHeader("If-None-Match", this_page->getETag());
                    }
                    else if (this_page->getLastModify() != "") { // use last modified version
                        request.AddHeader("If-Modified-Since", this_page->getLastModify());
                    }
                    cache.cache_store_mutex.unlock();
                    
                    // Create new thread to handle proxy->remote server connection
                    boost::thread remote_thread(makeRequestConnection, request, sock_fd);
                    if (DEBUG) cout << "Waiting for remote thread..." << endl;
                    // Wait for remote thread to exit
                    remote_thread.join();
                }
            }
            // If request not in cache
            else {
                cache.cache_store_mutex.unlock();
                if (DEBUG) cout << "Client request not in cache. Calling makeRequestConnection to host:" << request.GetHost() << " port:" << request.GetPort() << endl;
                
                // Create new thread to handle proxy->remote server connection
                boost::thread remote_thread(makeRequestConnection, request, sock_fd);
                if (DEBUG) cout << "Waiting for remote thread..." << endl;
                // Wait for remote thread to exit
                remote_thread.join();
            }
            
            // If the connection is non-persisent, close connection
            if(strcmp(request.FindHeader("Connection").c_str(), "close") == 0 ||
               strcmp(request.GetVersion().c_str(), "1.0") == 0)
                break;
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
        if (DEBUG) cout << "Done processing..." << endl;
    }
    
    if (DEBUG) cout << "Closing client connection, sock_fd:" << sock_fd << endl;
    close(sock_fd);
}

/*@brief Periodically checks for idle client->proxy and proxy->remote-server connections
 */
void cacheCleanupHandler() {
    while (1) {
        // Check proxy->remote-server connections
        cache.cache_connections_mutex.lock();
        cache.cacheConnectionCleanup();
        cache.cache_connections_mutex.unlock();
        // Check client->proxy connections
        cache.cache_clients_mutex.lock();
        cache.cacheClientCleanup();
        cache.cache_clients_mutex.unlock();
        // Sleep for specificed amount of time before checking again
        sleep(CHECK_CONNECTION_TIMEOUT);
    }
}

int main (int argc, char *argv[])
{
    // Create proxy server
    int sockfd;
    if ((sockfd = makeServerConnection(PROXY_PORT)) < 0) {
        perror("Proxy server");
        return 1;
    }
    cout << "Proxy server: waiting for connections..." << endl;
    
    // Create Boost thread group
    boost::thread_group t_group;
    // Create thread to handle proxy->remote-server timeout
    t_group.create_thread(&cacheCleanupHandler);
    t_group.create_thread(&exitHandler);
    
    // Accept incoming connections
    while (running) {
        // Incoming connection's info
        struct sockaddr_storage their_addr;
        socklen_t sin_size = sizeof their_addr;
        char s[INET6_ADDRSTRLEN];
        
        // Accept new client connection
        cache.cache_clients_mutex.lock();
        if (cache.getNumClients() < 20) {
            cache.cache_clients_mutex.unlock();
            
            int new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
            
            if (new_fd < 0) {
                //perror("Accept");
                continue;
            }
            else {
                // Add new client to cache clients
                cache.cache_clients_mutex.lock();
                cache.addToClients(new_fd);
                cache.cache_clients_mutex.unlock();
                
                inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof s);
                if (DEBUG) cout<<"Cache numclients: "<<cache.getNumClients()<<endl;
                if (DEBUG) printf("server: got connection from %s\n", s);
                
                // Make new Boost thread in group and pass off to connectionHandler()
                if (DEBUG) cout << "Making new thread with boost..." << endl;
                t_group.create_thread(boost::bind(&connectionHandler, new_fd));
            }
        }
        else
        {
            cache.cache_clients_mutex.unlock();
            if (DEBUG) printf("20 clients connected; cannot accept any more client connections.");
        }
    }
    t_group.interrupt_all();
    
    cout << "Proxy server: shutting down..." << endl;
    close(sockfd);
    return 0;
}
