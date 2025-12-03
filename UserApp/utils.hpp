#ifndef UTILS_HPP
#define UTILS_HPP

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

// Parses the command string
// Returns the corresponding CommandType
CommandType parse_command(const string &cmd);

vector<string> split(const string &line);

bool verify_login(const vector<string> &args);

int login (const vector<string> &args, ActiveUser &activeUser, string &ip, string &port, struct addrinfo* &res, struct sockaddr_in &addr);

bool verify_changePass(const vector<string> &args);

int changePass(const vector<string> &args, ActiveUser &activeUser, string &ip, string &port, struct addrinfo* &res);

bool verify_unregister(const vector<string> &args);

int unregister(const vector<string> &args, ActiveUser &activeUser, string &ip, string &port, struct addrinfo* &res, struct sockaddr_in &addr);

bool verify_logout(const vector<string> &args);

int logout(const vector<string> &args, ActiveUser &activeUser, string &ip, string &port, struct addrinfo* &res, struct sockaddr_in &addr);

bool verify_exit(const vector<string> &args);

int exit(const vector<string> &args, ActiveUser &activeUser);

bool verify_create(const vector<string> &args);

int create(const vector<string> &args, ActiveUser &activeUser, string &ip, string &port, struct addrinfo* &res, struct sockaddr_in &addr);

bool verify_close(const vector<string> &args);

int close(const vector<string> &args, ActiveUser &activeUser, string &ip, string &port, struct addrinfo* &res);

bool verify_myEvents(const vector<string> &args);

int myevents(const vector<string> &args, ActiveUser &activeUser, string &ip, string &port, struct addrinfo* &res, struct sockaddr_in &addr);

#endif