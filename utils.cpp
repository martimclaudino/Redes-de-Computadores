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
#include "utils.hpp"

using namespace std;

CommandType parse_command(const string &cmd)
{
    if (cmd == "login") return CMD_LOGIN;
    if (cmd == "changePass") return CMD_CHANGEPASS;
    if (cmd == "unregister") return CMD_UNREGISTER;
    if (cmd == "logout") return CMD_LOGOUT;
    if (cmd == "exit") return CMD_EXIT;
    if (cmd == "create") return CMD_CREATE;
    if (cmd == "close") return CMD_CLOSE;
    if ((cmd == "myevents") || (cmd == "mye")) return CMD_MYEVENTS;
    if (cmd == "list") return CMD_LIST;
    if (cmd == "show") return CMD_SHOW;
    if (cmd == "reserve") return CMD_RESERVE;
    if ((cmd == "myreservations") || (cmd == "myres")) return CMD_MYRESERVATIONS;
    return CMD_INVALID;
}

vector<string> split(const string &line)
{
    istringstream iss(line);
    vector<string> tokens;
    string word;

    while (iss >> word)
        tokens.push_back(word);

    return tokens;
}

bool verify_login(const vector<string> &args)
{
    if (args.size() != 3)
    {
        cout << "Invalid number of arguments for login. Usage: login <username> <password>" << endl;
        return false;
    }
    string username = args[1];
    string password = args[2];

    if (username.length() != 6 || password.length() != 8)
    {
        cout << "Username or password have the wrong size. Username length is 6 and password length is 8." << endl;
        return false;
    }
    return true;
}

int login (const vector<string> &args, ActiveUser &activeUser, string &ip, string &port, struct addrinfo* &res, struct sockaddr_in &addr)
{
    if (activeUser.loggedIn)
    {
        cout << "You are already logged in. If you want to switch accounts please logout first." << endl;
        return 1;
    }

    if (!verify_login(args))
        return 1;

    int fd = establish_UDP_connection(ip, port, res);
    
    string message;
    message = "LIN " + args[1] + " " + args[2];
    
    if (send_UDP_message(fd, message, res) == -1)
    {
        cerr << "Error sending message to server." << endl;
        return 1;
    }
    ServerResponse server_response = receive_UDP_message(fd, addr);
    if (server_response.status == -1)
    {
        cerr << "Error receiving response from server." << endl;
        return 1;
    }
    auto login_result = split(server_response.msg);

    if (login_result[1] == "OK")
    {
        cout << "Successful login" << endl;
        activeUser.loggedIn = true;
        activeUser.userId = args[1];
        activeUser.password = args[2];
    }
    else if (login_result[1] == "NOK")
        cout << "Unsuccessful login" << endl;
    else if (login_result[1] == "REG")
    {
        cout << "New user registered" << endl;
        activeUser.loggedIn = true;
        activeUser.userId = args[1];
        activeUser.password = args[2];
    }
    
    freeaddrinfo(res);
    close(fd);

    return 0;
}

bool verify_changePass(const vector<string> &args){

    if (args.size() != 3)
    {
        cout << "Invalid number of arguments for changePass. Usage: changePass <old_password> <new_password>" << endl;
        return false;
    }
    string old_password = args[1];
    string new_password = args[2];

    if (old_password.length() != 8 || new_password.length() != 8)
    {
        cout << "Passwords have the wrong size. Password length is 8." << endl;
        return false;
    }
    return true;
}

int changePass(const vector<string> &args, ActiveUser &activeUser, string &ip, string &port, struct addrinfo* &res)
{
    if (!activeUser.loggedIn)
    {
        cout << "You need to be logged in to change your password." << endl;
        return 1;
    }

    if(!verify_changePass(args))
        return 1;

    int fd = establish_TCP_connection(ip, port, res);

    string message;
    message = "CPS " + activeUser.userId + " " + args[1] + " " + args[2];

    if (send_TCP_message(fd, message) == -1)
    {
        cerr << "Error sending message to server." << endl;
        return 1;
    }
    ServerResponse server_response = receive_TCP_message(fd);
    if (server_response.status == -1)
    {
        cerr << "Error receiving response from server." << endl;
        return 1;
    }
    auto changePass_result = split(server_response.msg);

    if (changePass_result[1] == "NLG")
        cout << "No user is logged in" << endl;
    else if (changePass_result[1] == "NOK")
        cout << "Password is not correct" << endl;
    else if (changePass_result[1] == "NID")
        cout << "User doesn't exist" << endl;
    else if (changePass_result[1] == "OK")
        {
            cout << "Password changed successfully" << endl;
            activeUser.password = args[2];
        }
    
    freeaddrinfo(res);
    close(fd);

    return 0;
}

bool verify_unregister(const vector<string> &args)
{
    if (args.size() != 1)
    {
        cout << "Invalid number of arguments for unregister. Usage: unregister" << endl;
        return false;
    }
    return true;
}

int unregister(const vector<string> &args, ActiveUser &activeUser, string &ip, string &port,  struct addrinfo* &res, struct sockaddr_in &addr)
{
    if (!activeUser.loggedIn)
    {
        cout << "You need to be logged in to unregister." << endl;
        return 1;
    }
    if (!verify_unregister(args))
        return 1;

    int fd = establish_UDP_connection(ip, port, res);
    
    string message;
    message = "UNR" + activeUser.userId + " " + activeUser.password;
    
    if (send_UDP_message(fd, message, res) == -1)
    {
        cerr << "Error sending message to server." << endl;
        return 1;
    }
    ServerResponse server_response = receive_UDP_message(fd, addr);
    if (server_response.status == -1)
    {
        cerr << "Error receiving response from server." << endl;
        return 1;
    }
    auto login_result = split(server_response.msg);

    if (login_result[1] == "OK")
    {
        cout << "Successful login" << endl;
        activeUser.loggedIn = true;
    }
    else if (login_result[1] == "NOK")
        cout << "Unsuccessful login" << endl;
    else if (login_result[1] == "REG")
    {
        cout << "New user registered" << endl;
        activeUser.loggedIn = true;
    }
    
    freeaddrinfo(res);
    close(fd);
    return 0;
}

bool verify_logout(const vector<string> &args)
{
    if (args.size() != 1)
    {
        cout << "Invalid number of arguments for logout. Usage: logout" << endl;
        return false;
    }
    return true;
}

int logout(const vector<string> &args, ActiveUser &activeUser, string &ip, string &port, struct addrinfo* &res, struct sockaddr_in &addr)
{
    if (!activeUser.loggedIn)
    {
        cout << "You are not logged in." << endl;
        return 1;
    }
    if (!verify_logout(args))
        return 1;

    int fd = establish_UDP_connection(ip, port, res);
    
    string message;
    message = "LOU " + activeUser.userId + " " + activeUser.password;
    
    if (send_UDP_message(fd, message, res) == -1)
    {
        cerr << "Error sending message to server." << endl;
        return 1;
    }
    ServerResponse server_response = receive_UDP_message(fd, addr);
    if (server_response.status == -1)
    {
        cerr << "Error receiving response from server." << endl;
        return 1;
    }
    auto logout_result = split(server_response.msg);

    if (logout_result[1] == "OK")
    {
        cout << "Successful logout" << endl;
        activeUser.loggedIn = false;
    }
    else if (logout_result[1] == "NOK")
        cout << "Unsuccessful logout" << endl;
    else if (logout_result[1] == "UNR")
        cout << "User isn't registered" << endl;
    else if (logout_result[1] == "WRP")
        cout << "Wrong password" << endl;
    
    freeaddrinfo(res);
    close(fd);
    return 0;
}

bool verify_exit(const vector<string> &args)
{
    if (args.size() != 1)
    {
        cout << "Invalid number of arguments for exit. Usage: exit" << endl;
        return false;
    }
    return true;
}

int exit(const vector<string> &args, ActiveUser &activeUser)
{
    if (!verify_exit(args))
        return 1;

    if (activeUser.loggedIn)
    {
        cout << "You are currently logged in. Please logout before exiting." << endl;
        return 1;
    }
    cout << "Exiting application." << endl;
    // Close any open connections
    exit(0);
    return 0;
}

#endif