/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include <iostream>

#include "http-connection.h"

using namespace std;

int main (int argc, char *argv[])
{
    // Create server listener
    int sockfd;
    if ((sockfd = makeServerConnection(PORT)) < 0) {
        fprintf(stderr, "fail to make server\n");
        return 1;
    }

    // Accept incoming connections
    while (1) {
        <#statements#>
    }
    

    return 0;
}
