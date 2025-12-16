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