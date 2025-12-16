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
#include <sys/select.h>
#include <sys/wait.h>
#include <signal.h>

#include "common.hpp"
#include "protocols.hpp"

using namespace std;

CommandType parse_command(const string &cmd);

// Handler for Zombie processes 
void handle_sigchld(int sig);

int setup_UDP_server(string port);

int setup_TCP_server(string port);

ServerResponse receive_UDP_request(int fd, struct sockaddr_in &addr, socklen_t &addrlen);

int send_UDP_reply(int fd, string message, struct sockaddr_in addr, socklen_t addrlen);

ServerResponse verify_login(const vector<string> &args);

int login(vector<string> &args, int fd, struct sockaddr_in addr, socklen_t addrlen);
