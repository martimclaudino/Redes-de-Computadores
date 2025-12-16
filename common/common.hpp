// Group 10

#ifndef COMMON_HPP
#define COMMON_HPP

#include <string>
#include <vector>

using namespace std;

#define PORT "58000"
#define IPADDRESS "MartimClaudino"    // "tejo.tecnico.ulisboa.pt"        // Change into the lt5 address    

#define LOGIN_RESPONSE 7
#define CHANGEPASS_RESPONSE 7
#define UNREGISTER_RESPONSE 7
#define LOGOUT_RESPONSE 7
#define CREATE_RESPONSE 9
#define CLOSE_RESPONSE 7
#define MYEVENTS_RESPONSE 5007 // 7 + 1000 max events per user * 5 characters per event
#define LIST_RESPONSE 1000000 //3308 // 8 + 1000 max events * 3 characters per event + 4 spaces per event
#define SHOW_RESPONSE 100000
#define RESERVE_RESPONSE 11
#define MYRESERVATIONS_RESPONSE 32

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
    bool loggedIn;
    string userId;
    string password;
} ActiveUser;

typedef struct
{
    int status;
    string msg;
} ServerResponse;

vector<string> split(const string &line);

ServerResponse verify_login(const vector<string> &args);

#endif