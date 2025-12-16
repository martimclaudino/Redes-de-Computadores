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

ServerResponse verify_login(const vector<string> &args) // FIX ME i could use this on common.cpp and in the UserApp
{
    ServerResponse response;
    response.msg = "";
    response.status = 1;
    if (args.size() != 3)
    {
        response.msg = "Invalid number of arguments for login. Usage: login <username> <password>\n";
        response.status = -1;
        return response;
    }
    string username = args[1];
    string password = args[2];

    if (username.length() != 6 || password.length() != 8)
    {
        response.msg = "Username or password have the wrong size. Username length is 6 and password length is 8.\n";
        response.status = -1;
        return response;
    }
    return response;
}