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

void verbose_mode(string client_ip, int client_port, string command);

// Handler for Zombie processes 
void handle_sigchld(int sig);

int setup_UDP_server(string port);

ServerResponse receive_UDP_request(int fd, struct sockaddr_in &addr, socklen_t &addrlen);

int send_UDP_reply(int fd, const string message, struct sockaddr_in addr, socklen_t addrlen);

int setup_TCP_server(string port);

ServerResponse receive_TCP_request(int fd);

ssize_t send_TCP_reply(int fd, const string &message);

int register_user(string UID, string password, struct stat &st);

// Arguments
// -UID - user ID
// -input_pass - password to compare
// Returns true if passwords match, false otherwise
bool compare_passwords(string UID, string input_pass);

// Arguments
// -UID - user ID
// Returns true if the user is logged in, false otherwise
bool is_loggedin(string UID);

// Arguments
// -UID - user ID
// Returns true if the user registered
bool is_registered(string UID);

int delete_file(const string file_path);

ServerResponse verify_credentials(const vector<string> &args);

void login_user(string UID);

void logout_user(string UID);

int login(vector<string> &args, int fd, struct sockaddr_in addr, socklen_t addrlen);

int unregister(vector<string> &args, int fd, struct sockaddr_in addr, socklen_t addrlen);

int logout(vector<string> &args, int fd, struct sockaddr_in addr, socklen_t addrlen);

vector<string> get_event_data(string EID);

string get_event_state(string EID);

vector<string> list_created_events(string UID);

int myevents(vector<string> &args, int fd, struct sockaddr_in addr, socklen_t addrlen);

int get_reservations(string EID, string UID);

int myreservations(vector<string> &args, int fd, struct sockaddr_in addr, socklen_t addrlen);

ServerResponse verify_create(const vector<string> &args);

string get_next_eid();

int create(vector<string> &args, int fd, struct sockaddr_in addr, socklen_t addrlen);