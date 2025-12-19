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
#include <filesystem>
#include <fstream>
#include <filesystem>
#include <sys/file.h>
#include <unistd.h>
#include <fcntl.h>

using namespace std; 

#include "common.hpp"
#include "protocols.hpp"

vector<string> split(const string &line)
{
    istringstream iss(line);
    vector<string> tokens;
    string word;

    while (iss >> word)
        tokens.push_back(word);

    return tokens;
}

int acquire_lock(const string& filepath) 
{
    int fd = open(filepath.c_str(), O_CREAT | O_RDWR, 0666);
    if (fd == -1) 
    {
        perror("Lock file open error");
        return -1;
    }

    // LOCK_EX: Exclusive lock. If another process holds the lock,
    // this function BLOCKS (waits) until the other releases it.
    if (flock(fd, LOCK_EX) == -1) 
    {
        perror("Lock error");
        close(fd);
        return -1;
    }
    return fd;
}

void release_lock(int fd) 
{
    if (fd != -1) 
    {
        flock(fd, LOCK_UN);
        close(fd);          
    }
}