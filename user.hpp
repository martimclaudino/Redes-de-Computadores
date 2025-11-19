// Group 10

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

using namespace std;

#define PORT "58010"
#define IPADDRESS "MartimClaudino"                  //    "tejo.tecnico.ulisboa.pt"     // Change into the lt5 address    

typedef enum
{
    CMD_INVALID = -1,
    CMD_LOGIN,
    CMD_CHANGEPASS,
    CMD_UNREGISTER,
    CMD_LOGOUT,
    CMD_EXIT,
    CMD_CREATE,
    CMD_CLOSE,
    CMD_MYEVENTS,
    CMD_LIST,
    CMD_SHOW,
    CMD_RESERVE,
    CMD_MYRESERVATIONS
} CommandType;

typedef struct
{
    int status;
    string msg;
} ServerResponse;

// Parses the command string
// Returns the corresponding CommandType
CommandType parse_command(const string &cmd);

vector<string> split(const string &line);

int establish_UDP_connection();

int send_UDP_message(int fd, const string &message);

ServerResponse receive_UDP_message(int fd);

int establish_TCP_connection();

ssize_t send_TCP_message(int fd, const string &message);

ssize_t receive_TCP_message(int fd);

bool verify_login(vector<string> &args);

bool verify_changePass(vector<string> &args);