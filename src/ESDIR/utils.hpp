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

int send_UDP_reply(int fd, const string message, struct sockaddr_in addr, socklen_t addrlen);

int respond_to_client(int fd, const string message, struct sockaddr_in addr, socklen_t addrlen);

int register_user(string UID, string password, string user_path, string pass_file, string login_file, struct stat &st);

// Arguments
// -pass_file - path to password file
// -input_pass - password to compare
// Returns true if passwords match, false otherwise
bool compare_passwords(string pass_file, string input_pass);

// Arguments
// -login_file - path to login file
// Returns true if the user is logged in, false otherwise
bool is_loggedin(string login_file);

// Arguments
// -user_path - path to user directory
// -pass_file - path to password file
// Returns true if the user registered
bool is_registered(string user_path, string pass_file);

ServerResponse verify_login(const vector<string> &args);

int login(vector<string> &args, int fd, struct sockaddr_in addr, socklen_t addrlen);

ServerResponse verify_unregister(const vector<string> &args);

int unregister(vector<string> &args, int fd, struct sockaddr_in addr, socklen_t addrlen);
