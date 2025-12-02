// Group 10

#ifndef COMMON_HPP
#define COMMON_HPP

#include <string>
#include <vector>

using namespace std;

#define PORT "58000"
#define IPADDRESS "tejo.tecnico.ulisboa.pt"     // "MartimClaudino"       // Change into the lt5 address    

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

#endif