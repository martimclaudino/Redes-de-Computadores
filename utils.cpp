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
#include <filesystem>
#include <fstream>

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
    message = "LIN " + args[1] + " " + args[2] + "\n";
    
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
        cout << "Password doesn't match" << endl;
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
    message = "CPS " + activeUser.userId + " " + args[1] + " " + args[2] + "\n";

    if (send_TCP_message(fd, message) == -1)
    {
        cerr << "Error sending message to server." << endl;
    } 
    else 
    {
        ServerResponse server_response = receive_TCP_message(fd);
        if (server_response.status == -1)
        {
            cerr << "Error receiving response from server." << endl;
            return 1;
        }
        auto changePass_result = split(server_response.msg);

        if (changePass_result[1] == "NLG")
            cout << "No user is logged in" << endl;
        else if (changePass_result[1] == "NOK")         // FIX ME verificar se é mesmo NOK
            cout << "Password is not correct" << endl;
        else if (changePass_result[1] == "NID")
            cout << "User doesn't exist" << endl;
        else if (changePass_result[1] == "OK")
            {
                cout << "Password changed successfully" << endl;
                activeUser.password = args[2];
            }
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
    message = "UNR " + activeUser.userId + " " + activeUser.password + "\n";
    
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
    auto unregister_result = split(server_response.msg);

    if (unregister_result[1] == "OK")
    {
        cout << "Successfully unregistered" << endl;
        activeUser.loggedIn = false;
    }
    else if (unregister_result[1] == "NOK")
        cout << "No user logged in" << endl;
    else if (unregister_result[1] == "UNR")
    {
        cout << "The user isn't registered" << endl;
    }
    else if (unregister_result[1] == "WRP")
        cout << "Wrong password" << endl;
    
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
    message = "LOU " + activeUser.userId + " " + activeUser.password + "\n";
    
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

bool verify_create(const vector<string> &args)
{
    if (args.size() != 6)   // Date is 2 arguments: date and time
    {
        cout << "Invalid number of arguments for create. Usage: create <name> <event_fname> <date> <number_of_attendees>" << endl;
        return false;
    }
    if (args[1].length() > 10)
    {
        cout << "Max name length is 10 characters." << endl;
        return false;
    }
    int day, month, year, hour, minute;
    if (sscanf(args[3].c_str(), "%2d-%2d-%4d", &day, &month, &year) != 3)
    {
        cout << "Date format is incorrect. Please use DD-MM-YYYY HH:MM format." << endl;
        return false;
    }
    if (sscanf(args[4].c_str(), "%2d:%2d", &hour, &minute) != 2)
    {
        cout << "Date format is incorrect. Please use DD-MM-YYYY HH:MM format." << endl;
        return false;
    }
    if (day < 1 || day > 31 || month < 1 || month > 12 || hour < 0 || hour > 23 || minute < 0 || minute > 59)
    {
        cout << "Date values are out of range." << endl;
        return false;
    }
    if (!std::filesystem::exists(args[2]))
    {
        cout << "Event file does not exist." << endl;
        return false;
    }
    return true;
}

int create(const vector<string> &args, ActiveUser &activeUser, string &ip, string &port, struct addrinfo* &res, struct sockaddr_in &addr)
{
    if (!activeUser.loggedIn)
    {
        cout << "You must be logged in to create an event." << endl;
        return 1;
    }
    if (!verify_create(args))
        return 1;
    string eventName = args[1];
    string fileName = args[2];
    string date = args[3];
    string time = args[4];
    string attendance = args[5];
    
    ifstream file(fileName, ios::binary | ios::ate);

    if (!file.is_open())    // Just in case the file gets deleted or moved after verification
    {
        cout << "Error: File '" << fileName << "' not found or cannot be read." << endl;
        return 1;
    }

    streamsize fileSize = file.tellg();

    file.seekg(0, ios::beg);

    vector<char> fileData(fileSize);

    if (!file.read(fileData.data(), fileSize)) 
    {
        cout << "Error: Could not read file content." << endl;
        return 1;
    }
    file.close();

    stringstream header;
    header << "CRE " << activeUser.userId << " " << activeUser.password << " "
           << eventName << " " << date << " " << time << " " << attendance << " "
           << fileName << " " << fileSize << " ";

    string fullMessage = header.str();

    fullMessage.append(fileData.data(), fileSize);

    fullMessage += "\n";

    int fd = establish_TCP_connection(ip, port, res);

    if (send_TCP_message(fd, fullMessage) == -1) 
    {
        cout << "Error sending create request." << endl;
    }
    else
    {
    ServerResponse server_response = receive_TCP_message(fd);
    cout << "Server response: " << server_response.msg << endl;
    if (server_response.status == -1)
        {
            cerr << "Error receiving response from server." << endl;
            return 1;
        }
        auto create_result = split(server_response.msg);

        if (create_result[1] == "NOK")
            cout << "Event couldn't be created" << endl;
        else if (create_result[1] == "NLG")
            cout << "No user is logged in" << endl;
        else if (create_result[1] == "WRP")
            cout << "Password is not correct" << endl;
        else if (create_result[1] == "OK")
            cout << "Event created successfully. Event ID: " << create_result[2] << endl;
    }
    
    freeaddrinfo(res);
    close(fd);
    
    return 0;
}

#endif