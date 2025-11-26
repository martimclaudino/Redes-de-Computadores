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

using namespace std;

int establish_UDP_connection(string &ip, string &port, struct addrinfo* &res);

int send_UDP_message(int fd, const string &message, struct addrinfo* &res);

ServerResponse receive_UDP_message(int fd, struct sockaddr_in addr);

int establish_TCP_connection(string &ip, string &port, struct addrinfo* &res);

ssize_t send_TCP_message(int fd, const string &message);

ServerResponse receive_TCP_message(int fd);


#endif