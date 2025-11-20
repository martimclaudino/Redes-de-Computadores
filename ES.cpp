// Group 10

#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "ES.hpp"

#define PORT "58010"

int fd, newfd, errcode;
ssize_t n;
socklen_t addrlen;
struct addrinfo hints, *res;
struct sockaddr_in addr;
char buffer[128];

int main()
{
    fd = socket(AF_INET, SOCK_DGRAM, 0); // UDP socket
    if (fd == -1)                        /*error*/
        exit(1);
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;      // IPv4
    hints.ai_socktype = SOCK_DGRAM; // UDP socket
    hints.ai_flags = AI_PASSIVE;
    errcode = getaddrinfo(NULL, PORT, &hints, &res);
    if (errcode != 0) /*error*/
        exit(1);
    n = bind(fd, res->ai_addr, res->ai_addrlen);
    if (n == -1) /*error*/
        exit(1);

    while (1)
    {
        addrlen = sizeof(addr);
        n = recvfrom(fd, buffer, 128, 0,
                     (struct sockaddr *)&addr, &addrlen);
        if (n == -1) /*error*/
            exit(1);
        write(1, "received: ", 10);
        write(1, buffer, n);
        string hardcoded_response = "RLI OK\n";
        n = sendto(fd, hardcoded_response.c_str(), hardcoded_response.size(), 0,
                   (struct sockaddr *)&addr, addrlen);
        if (n == -1) /*error*/
            exit(1);
    }

    freeaddrinfo(res);
    close(fd);
    return 0;
}