#ifndef PROTOCOLS_HPP
#define PROTOCOLS_HPP

#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "common.hpp"
#include "protocols.hpp"

using namespace std;

int establish_UDP_connection(string &ip, string &port, struct addrinfo* &res)
{
    struct addrinfo hints;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;       // IPv4
    hints.ai_socktype = SOCK_DGRAM;  // UDP 
    int errcode = getaddrinfo(ip.c_str(), port.c_str(), &hints, &res);
    if (errcode != 0)
    {
        cerr << "getaddrinfo error: " << gai_strerror(errcode) << endl;
        exit(EXIT_FAILURE);
    }

    // Create socket
    int fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (fd == -1)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    return fd;
}

int send_UDP_message(int fd, const string &message, struct addrinfo* &res)
{
    int n = sendto(fd, message.c_str(), message.size(), 0, res->ai_addr, res->ai_addrlen);
    
    if (n == -1)
    {
        perror("sendto");
        exit(EXIT_FAILURE);
        return n;
    }
    
    return n;
}

ServerResponse receive_UDP_message(int fd, struct sockaddr_in addr)
{
    ServerResponse server_response;
    socklen_t addrlen = sizeof(addr);
    char buffer[128];                                                    // FIX ME buffer size

    int n = recvfrom(fd, buffer, sizeof(buffer), 0,
                 (struct sockaddr *)&addr, &addrlen);
    
    server_response.status = n;
    
    if (n == -1) /*error*/
    {
        exit(1);
        return server_response;
    }

    string msg(buffer,n);
    server_response.msg = msg;
    return server_response;
}

int establish_TCP_connection(string &ip, string &port, struct addrinfo* &res)
{
    struct addrinfo hints;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;       // IPv4
    hints.ai_socktype = SOCK_STREAM;  // TCP
    int errcode = getaddrinfo(ip.c_str(), port.c_str(), &hints, &res);
    if (errcode != 0)
    {
        cerr << "getaddrinfo error: " << gai_strerror(errcode) << endl;
        exit(EXIT_FAILURE);
    }

    // Create socket
    int fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (fd == -1)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Connect to server
    if (connect(fd, res->ai_addr, res->ai_addrlen) == -1)
    {
        perror("connect");
        close(fd);
        exit(EXIT_FAILURE);
    }

    return fd;
}

ssize_t send_TCP_message(int fd, const string &message)
{
    ssize_t total_sent = 0;
    ssize_t message_length = message.size();
     ssize_t n;

    while (total_sent < message_length)
    {
        n = write(fd, message.c_str() + total_sent, message_length - total_sent);
        if (n == -1)
        {
            perror("send");
            return n;
        }
        total_sent += n;
    }
    return n;
}

ServerResponse receive_TCP_message(int fd)
{
    ServerResponse server_response;
    string result;
    char buffer[128];
    ssize_t n;
    
    while (true)
    {
        n = read(fd, buffer, sizeof(buffer) - 1);
        if (n == -1)
        {
            server_response.status = n;
            perror("receive");
            return server_response;
        }
        if (n == 0) {
            // connection closed
            break;
        }

        result.append(buffer, n);
        if (result.find('\n') != string::npos)
            break;  // end of message
    }
    
    string msg(buffer,n);
    server_response.msg = msg;
    return server_response;
}

#endif