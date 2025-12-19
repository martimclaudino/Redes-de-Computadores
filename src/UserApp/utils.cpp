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
#include <filesystem>

#include "common.hpp"
#include "protocols.hpp"
#include "utils.hpp"

using namespace std;
namespace fs = std::filesystem;

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
    if ((cmd == "myreservations") || (cmd == "myr")) return CMD_MYRESERVATIONS;
    return CMD_INVALID;
}

ServerResponse verify_login(const vector<string> &args)
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

// FIX ME add the ERR case to each function
int login (const vector<string> &args, ActiveUser &activeUser, string &ip, string &port, struct addrinfo* &res, struct sockaddr_in &addr)
{
    if (activeUser.loggedIn)
    {
        cout << "You are already logged in. If you want to switch accounts please logout first." << endl;
        return 1;
    }
    ServerResponse login = verify_login(args);
    if (login.status == -1)
    {
        cout << login.msg;
        return 1;
    }
        
    int fd = establish_UDP_connection(ip, port, res);

    if (fd == -1)
    {
        cerr << "Error establishing TCP connection." << endl;
        return 1;
    }
    
    string message;
    message = "LIN " + args[1] + " " + args[2] + "\n";
    
    if (send_UDP_message(fd, message, res) == -1)
    {
        cerr << "Error sending message to server." << endl;
        if (res != nullptr) 
        {
            freeaddrinfo(res);
            res = nullptr;
        }
        freeaddrinfo(res);
        close(fd);
        return 1;
    }
    ServerResponse server_response = receive_UDP_message(fd, addr, LOGIN_RESPONSE);
    if (server_response.status == -1)
    {
        cerr << "Error receiving response from server." << endl;
        if (res != nullptr) {
            freeaddrinfo(res);
            res = nullptr;
        }
        close(fd);
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

    if (fd == -1)
    {
        cerr << "Error establishing TCP connection." << endl;
        return 1;
    }

    string message;
    message = "CPS " + activeUser.userId + " " + args[1] + " " + args[2] + "\n";

    if (send_TCP_message(fd, message) == -1)
    {
        cerr << "Error sending message to server." << endl;
        if (res != nullptr) 
        {
            freeaddrinfo(res);
            res = nullptr;
        }
        freeaddrinfo(res);
        close(fd);
        return 1;
    }
    ServerResponse server_response = receive_TCP_message(fd, CHANGEPASS_RESPONSE);
    if (server_response.status == -1)
    {
        cerr << "Error receiving response from server." << endl;
        if (res != nullptr) {
            freeaddrinfo(res);
            res = nullptr;
        }
        close(fd);
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

ServerResponse verify_unregister(const vector<string> &args)
{
    ServerResponse response;
    response.status = 1;
    response.msg = "";
    if (args.size() != 1)
    {
        response.msg = "Invalid number of arguments for unregister. Usage: unregister\n";
        response.status = -1;
        return response;
    }
    return response;
}

int unregister(const vector<string> &args, ActiveUser &activeUser, string &ip, string &port,  struct addrinfo* &res, struct sockaddr_in &addr)
{
    if (!activeUser.loggedIn)
    {
        cout << "You need to be logged in to unregister." << endl;
        return 1;
    }
    ServerResponse unregister = verify_unregister(args);
    if (unregister.status == -1)
    {
        cout << unregister.msg;
        return 1;
    }

    int fd = establish_UDP_connection(ip, port, res);

    if (fd == -1)
    {
        cerr << "Error establishing TCP connection." << endl;
        return 1;
    }
    
    string message;
    message = "UNR " + activeUser.userId + " " + activeUser.password + "\n";
    
    if (send_UDP_message(fd, message, res) == -1)
    {
        cerr << "Error sending message to server." << endl;
        if (res != nullptr) 
        {
            freeaddrinfo(res);
            res = nullptr;
        }
        freeaddrinfo(res);
        close(fd);
        return 1;
    }
    ServerResponse server_response = receive_UDP_message(fd, addr, UNREGISTER_RESPONSE);
    if (server_response.status == -1)
    {
        cerr << "Error receiving response from server." << endl;
        if (res != nullptr) {
            freeaddrinfo(res);
            res = nullptr;
        }
        close(fd);
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

    if (fd == -1)
    {
        cerr << "Error establishing TCP connection." << endl;
        return 1;
    }
    
    string message;
    message = "LOU " + activeUser.userId + " " + activeUser.password + "\n";
    
    if (send_UDP_message(fd, message, res) == -1)
    {
        cerr << "Error sending message to server." << endl;
        if (res != nullptr) 
        {
            freeaddrinfo(res);
            res = nullptr;
        }
        freeaddrinfo(res);
        close(fd);
        return 1;
    }
    ServerResponse server_response = receive_UDP_message(fd, addr, LOGOUT_RESPONSE);
    if (server_response.status == -1)
    {
        cerr << "Error receiving response from server." << endl;
        if (res != nullptr) {
            freeaddrinfo(res);
            res = nullptr;
        }
        close(fd);
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

int create(const vector<string> &args, ActiveUser &activeUser, string &ip, string &port, struct addrinfo* &res)
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

    if (fd == -1)
    {
        cerr << "Error establishing TCP connection." << endl;
        return 1;
    }

    if (send_TCP_message(fd, fullMessage) == -1)
    {
        cerr << "Error sending message to server." << endl;
        if (res != nullptr) 
        {
            freeaddrinfo(res);
            res = nullptr;
        }
        freeaddrinfo(res);
        close(fd);
        return 1;
    }
    ServerResponse server_response = receive_TCP_message(fd, CREATE_RESPONSE);
    if (server_response.status == -1)
    {
        cerr << "Error receiving response from server." << endl;
        if (res != nullptr) {
            freeaddrinfo(res);
            res = nullptr;
        }
        close(fd);
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
    
    
    freeaddrinfo(res);
    close(fd);
    
    return 0;
}

bool verify_close(const vector<string> &args)
{
    if (args.size() != 2)
    {
        cout << "Invalid number of arguments for close. Usage: close <event_id>" << endl;
        return false;
    }
    return true;
}

int close(const vector<string> &args, ActiveUser &activeUser, string &ip, string &port, struct addrinfo* &res)
{
    if (!activeUser.loggedIn)
    {
        cout << "You need to be logged in to close an event." << endl;
        return 1;
    }
    if (!verify_close(args))
        return 1;

    int fd = establish_TCP_connection(ip, port, res);

    if (fd == -1)
    {
        cerr << "Error establishing TCP connection." << endl;
        return 1;
    }
    
    string message;
    message = "CLS " + activeUser.userId + " " + activeUser.password + " " + args[1] + "\n";
    
    if (send_TCP_message(fd, message) == -1)
    {
        cerr << "Error sending message to server." << endl;
        if (res != nullptr) 
        {
            freeaddrinfo(res);
            res = nullptr;
        }
        freeaddrinfo(res);
        close(fd);
        return 1;
    }
    ServerResponse server_response = receive_TCP_message(fd, CLOSE_RESPONSE);
    if (server_response.status == -1)
    {
        cerr << "Error receiving response from server." << endl;
        if (res != nullptr) {
            freeaddrinfo(res);
            res = nullptr;
        }
        close(fd);
        return 1;
    }
    auto close_result = split(server_response.msg);

    if (close_result[1] == "OK")
    {
        cout << "Event closed successfully" << endl;
    }
    else if (close_result[1] == "NOK")
        cout << "User doesn't exist or password is incorrect" << endl;
    else if (close_result[1] == "NLG")
        cout << "No user is logged in" << endl;
    else if (close_result[1] == "NOE")
        cout << "Event doesn't exist" << endl;
    else if (close_result[1] == "EOW")
        cout << "Event wasn't created by the logged in user" << endl;
    else if (close_result[1] == "SLD")
        cout << "Event has already sold out" << endl;
    else if (close_result[1] == "PST")
        cout << "Event has already passed" << endl;
    else if (close_result[1] == "CLO")
        cout << "Event has already been closed" << endl;
    
    freeaddrinfo(res);
    close(fd);
    return 0;
}

bool verify_myEvents(const vector<string> &args)
{
    if (args.size() != 1)
    {
        cout << "Invalid number of arguments for myevents. Usage: myevents" << endl;
        return false;
    }
    return true;
}

int myevents(const vector<string> &args, ActiveUser &activeUser, string &ip, string &port, struct addrinfo* &res, struct sockaddr_in &addr)
{
    if (!activeUser.loggedIn)
    {
        cout << "You must be logged in to view your events." << endl;
        return 1;
    }
    if (!verify_myEvents(args))
        return 1;

    int fd = establish_UDP_connection(ip, port, res);

    if (fd == -1)
    {
        cerr << "Error establishing TCP connection." << endl;
        return 1;
    }
    
    string message;
    message = "LME " + activeUser.userId + " " + activeUser.password + "\n";
    
    if (send_UDP_message(fd, message, res) == -1)
    {
        cerr << "Error sending message to server." << endl;
        if (res != nullptr) 
        {
            freeaddrinfo(res);
            res = nullptr;
        }
        freeaddrinfo(res);
        close(fd);
        return 1;
    }
    ServerResponse server_response = receive_UDP_message(fd, addr, MYEVENTS_RESPONSE);
    if (server_response.status == -1)
    {
        cerr << "Error receiving response from server." << endl;
        if (res != nullptr) {
            freeaddrinfo(res);
            res = nullptr;
        }
        close(fd);
        return 1;
    }
    auto myevents_result = split(server_response.msg);

    if (myevents_result[1] == "NOK")
        cout << "User hasn't created any events" << endl;
    else if (myevents_result[1] == "NLG")
        cout << "No user is logged in" << endl;
    else if (myevents_result[1] == "OK")
    {
        cout << "Your events: " << endl;
        int i = 2;
        while (i < int(myevents_result.size()) - 1)
        {
            cout << myevents_result[i] << " " << myevents_result[i+1] << endl;
            i +=2;
        }
    }
    else if (myevents_result[1] == "WRP")
        cout << "Password is not correct" << endl;
    
    freeaddrinfo(res);
    close(fd);
    return 0;
}

bool verify_list(const vector<string> &args)
{
    if (args.size() != 1)
    {
        cout << "Invalid number of arguments for list. Usage: list" << endl;
        return false;
    }
    return true;
}

int list(const vector<string> &args, ActiveUser &activeUser, string &ip, string &port, struct addrinfo* &res)
{
    if (!activeUser.loggedIn)
    {
        cout << "You must be logged in to list events." << endl;
        return 1;
    }
    if (!verify_list(args))
        return 1;

    int fd = establish_TCP_connection(ip, port, res);

    if (fd == -1)
    {
        cerr << "Error establishing TCP connection." << endl;
        return 1;
    }
    
    string message;
    message = "LST\n";
    
    if (send_TCP_message(fd, message) == -1)
    {
        cerr << "Error sending message to server." << endl;
        if (res != nullptr) 
        {
            freeaddrinfo(res);
            res = nullptr;
        }
        freeaddrinfo(res);
        close(fd);
        return 1;
    }
    ServerResponse server_response = receive_TCP_message(fd, LIST_RESPONSE);
    if (server_response.status == -1)
    {
        cerr << "Error receiving response from server." << endl;
        if (res != nullptr) {
            freeaddrinfo(res);
            res = nullptr;
        }
        close(fd);
        return 1;
    }
    auto list_result = split(server_response.msg);

    if (list_result[1] == "NOK")
        cout << "No events have been created" << endl;
    else if (list_result[1] == "OK")
    {
        cout << "Available events: " << endl;
        int i = 2;
        while (i < int(list_result.size()) - 1)
        {
            cout << list_result[i] << " " << list_result[i+1] << " " << list_result[i+2] << " " << list_result[i+3] << " " << list_result[i+4] << endl;
            i +=5;
        }
    }
    
    freeaddrinfo(res);
    close(fd);
    return 0;
}

bool verify_show(const vector<string> &args)
{
    if (args.size() != 2)
    {
        cout << "Invalid number of arguments for show. Usage: show <event_id>" << endl;
        return false;
    }
    if (args[1].length() != 3)
    {
        cout << "Event ID must be 3 characters long." << endl;
        return false;
    }
    if (args[1] < "001" || args[1] > "999")
    {
        cout << "Event ID must be between 001 and 999." << endl;
        return false;
    }
    return true;
}

int show(const vector<string> &args, ActiveUser &activeUser, string &ip, string &port, struct addrinfo* &res)
{
    if (!activeUser.loggedIn)
    {
        cout << "You must be logged in to show event details." << endl;
        return 1;
    }
    if (!verify_show(args))
        return 1;

    int fd = establish_TCP_connection(ip, port, res);

    if (fd == -1)
    {
        cerr << "Error establishing TCP connection." << endl;
        return 1;
    }

    string message;
    message = "SED " + args[1] + "\n";

    if (send_TCP_message(fd, message) == -1)
    {
        cerr << "Error sending message to server." << endl;
        if (res != nullptr) 
        {
            freeaddrinfo(res);
            res = nullptr;
        }
        freeaddrinfo(res);
        close(fd);
        return 1;
    }
    ServerResponse server_response = receive_TCP_message(fd, SHOW_RESPONSE);
    if (server_response.status == -1)
    {
        cerr << "Error receiving response from server." << endl;
        if (res != nullptr) {
            freeaddrinfo(res);
            res = nullptr;
        }
        close(fd);
        return 1;
    }
    auto show_result = split(server_response.msg);

    if (show_result[1] == "NOK")
        cout << "Event does not exist, there is no file to be sent or another problem occurred" << endl;
    else if (show_result[1] == "OK")
    {
        cout << "Event details:" << endl;
        cout << "Event created by user with ID: " << show_result[2] << endl;
        cout << "Event Name: " << show_result[3] << endl;
        cout << "Date: " << show_result[4] << " " << show_result[5] << endl;
        cout << "Number of attendees: " << show_result[6] << endl;
        cout << "Seats reserved: " << show_result[7] << endl;
        cout << "Event File Name: " << show_result[8] << endl;
        cout << "Event File Size: " << show_result[9] << " bytes" << endl;

        long fileSize = stol(show_result[9]);

        stringstream ss(server_response.msg);
        string temp;
        
        // Removing all the header info to get to the file data
        for(int i = 0; i < 10; ++i) 
        {
            ss >> temp;
        }
        // remove the last space before the file data
        char space;
        ss.read(&space, 1);

        streampos dataStart = ss.tellg();

        if (dataStart + (streampos)fileSize > (streampos)server_response.msg.size()) 
        {
            cout << "Error: Incomplete file data received." << endl;
            freeaddrinfo(res);
            close(fd);
            return 1;
        }
        else 
        {
            string directoryName = "EVENTS";

            // Check if the directory exists, if not create it
            if (!fs::exists(directoryName))
                fs::create_directory(directoryName);

            fs::path filePath = fs::path(directoryName) / show_result[8];
            // Save the file
            ofstream outputFile(filePath, ios::binary);
            
            if (outputFile.is_open()) 
            {
                // write the file in the disk
                outputFile.write(&server_response.msg[dataStart], fileSize);
                
                outputFile.close();
                cout << "File '" << show_result[8] << "' saved successfully on " << filePath << endl;
            } 
            else 
            {
                cout << "Error: Could not save file '" << show_result[8] << endl;
                freeaddrinfo(res);
                close(fd);
                return 1;
            }
        }
    }

    freeaddrinfo(res);
    close(fd);
    return 0;
}

bool verify_reserve(const vector<string> &args)
{
    if (args.size() != 3)
    {
        cout << "Invalid number of arguments for show. Usage: reserve <event_id> <number_of_seats>" << endl;
        return false;
    }
    if (args[1].length() != 3)
    {
        cout << "Event ID must be 3 characters long." << endl;
        return false;
    }
    if (args[1] < "001" || args[1] > "999")
    {
        cout << "Event ID must be between 001 and 999." << endl;
        return false;
    }
    return true;
}

int reserve(const vector<string> &args, ActiveUser &activeUser, string &ip, string &port, struct addrinfo* &res)
{
    if (!activeUser.loggedIn)
    {
        cout << "You must be logged in to reserve seats." << endl;
        return 1;
    }
    if (!verify_reserve(args))
        return 1;

    int fd = establish_TCP_connection(ip, port, res);

    if (fd == -1)
    {
        cerr << "Error establishing TCP connection." << endl;
        return 1;
    }

    string message;
    message = "RID " + activeUser.userId + " " + activeUser.password + " " + args[1] + " " + args[2] + "\n";

    if (send_TCP_message(fd, message) == -1)
    {
        cerr << "Error sending message to server." << endl;
        if (res != nullptr) 
        {
            freeaddrinfo(res);
            res = nullptr;
        }
        freeaddrinfo(res);
        close(fd);
        return 1;
    }
    ServerResponse server_response = receive_TCP_message(fd, RESERVE_RESPONSE);
    if (server_response.status == -1)
    {
        cerr << "Error receiving response from server." << endl;
        if (res != nullptr) {
            freeaddrinfo(res);
            res = nullptr;
        }
        close(fd);
        return 1;
    }
    auto show_result = split(server_response.msg);

    if (show_result[1] == "NOK")
        cout << "Event " << args[1] << " is not active" << endl;
    else if (show_result[1] == "NLG")
        cout << "No user logged in" << endl;
    else if (show_result[1] == "CLS")
        cout << "The event is closed" << endl;
    else if (show_result[1] == "SLD")
        cout << "The event is sold out" << endl;
    else if (show_result[1] == "REJ")
        cout << "There aren't enough seats available. There are only " << show_result[2] << " seats available" << endl;
    else if (show_result[1] == "PST")
        cout << "The event has already passed" << endl;
    else if (show_result[1] == "WRP")
        cout << "Wrong password" << endl;
    else if (show_result[1] == "ACC")
        cout << "The reservation has been made successfully" << endl;

    freeaddrinfo(res);
    close(fd);
    return 0;
}

bool verify_myreservations(const vector<string> &args)
{
    if (args.size() != 1)
    {
        cout << "Invalid number of arguments for myreservations. Usage: myreservations" << endl;
        return false;
    }
    return true;
}

int myreservations(const vector<string> &args, ActiveUser &activeUser, string &ip, string &port, struct addrinfo* &res, struct sockaddr_in &addr)
{
    if (!activeUser.loggedIn)
    {
        cout << "You must be logged in to view your reservations." << endl;
        return 1;
    }
    if (!verify_myreservations(args))
        return 1;

    int fd = establish_UDP_connection(ip, port, res);

    if (fd == -1)
    {
        cerr << "Error establishing TCP connection." << endl;
        return 1;
    }
    
    string message;
    message = "LMR " + activeUser.userId + " " + activeUser.password + "\n";
    
    if (send_UDP_message(fd, message, res) == -1)
    {
        cerr << "Error sending message to server." << endl;
        if (res != nullptr) 
        {
            freeaddrinfo(res);
            res = nullptr;
        }
        freeaddrinfo(res);
        close(fd);
        return 1;
    }
    ServerResponse server_response = receive_UDP_message(fd, addr, MYRESERVATIONS_RESPONSE);
    if (server_response.status == -1)
    {
        cerr << "Error receiving response from server." << endl;
        if (res != nullptr) {
            freeaddrinfo(res);
            res = nullptr;
        }
        close(fd);
        return 1;
    }
    auto myreservations_result = split(server_response.msg);

    if (myreservations_result[1] == "NOK")
        cout << "User has not made any reservations" << endl;
    else if (myreservations_result[1] == "NLG")
        cout << "You must be logged in to see your reservations" << endl;
    else if (myreservations_result[1] == "OK")
    {
        cout << "Your reservations: " << endl;
        int i = 2;
        while (i < int(myreservations_result.size()) - 1)
        {
            cout << myreservations_result[i] << " " << myreservations_result[i+1] << " " << myreservations_result[i+2] << " " << myreservations_result[i+3] << endl;
            i +=4;
        }
    }
    
    freeaddrinfo(res);
    close(fd);
    return 0;
}

#endif