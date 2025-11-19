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
#include "user.hpp"

using namespace std;

int fd, newfd, errcode;
ssize_t n;
socklen_t addrlen;

struct addrinfo hints, *res;
struct sockaddr_in addr;

string input_buffer;
string ip = IPADDRESS;
string port = PORT;

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

int establish_UDP_connection()
{

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;       // IPv4
    hints.ai_socktype = SOCK_DGRAM;  // UDP 
    errcode = getaddrinfo(ip.c_str(), port.c_str(), &hints, &res);
    if (errcode != 0)
    {
        cerr << "getaddrinfo error: " << gai_strerror(errcode) << endl;
        exit(EXIT_FAILURE);
    }

    // Create socket
    int fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (fd == -1)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    return fd;
}

int send_UDP_message(int fd, const string &message)
{
    n = sendto(fd, message.c_str(), message.size(), 0, res->ai_addr, res->ai_addrlen);
    
    if (n == -1)
    {
        perror("sendto");
        exit(EXIT_FAILURE);
        return n;
    }
    
    cout << "Sent message: " << message << endl;
    return n;
}

ServerResponse receive_UDP_message(int fd)
{
    ServerResponse server_response;
    addrlen = sizeof(addr);
    char buffer[289];

    n = recvfrom(fd, buffer, 128, 0,
                 (struct sockaddr *)&addr, &addrlen);
    
    server_response.status = n;
    
    if (n == -1) /*error*/
    {
        exit(1);
        return server_response;
    }

    string msg(buffer,n);
    server_response.msg = msg;
    return server_response;
}

int establish_TCP_connection()
{
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;       // IPv4
    hints.ai_socktype = SOCK_STREAM;  // TCP
    errcode = getaddrinfo(ip.c_str(), port.c_str(), &hints, &res);
    if (errcode != 0)
    {
        cerr << "getaddrinfo error: " << gai_strerror(errcode) << endl;
        exit(EXIT_FAILURE);
    }

    // Create socket
    int fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (fd == -1)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Connect to server
    if (connect(fd, res->ai_addr, res->ai_addrlen) == -1)
    {
        perror("connect");
        close(fd);
        exit(EXIT_FAILURE);
    }

    return fd;
}

ssize_t send_TCP_message(int fd, const string &message)
{
    ssize_t total_sent = 0;
    ssize_t message_length = message.size();

    while (total_sent < message_length)
    {
        ssize_t n = write(fd, message.c_str() + total_sent, message_length - total_sent);
        if (n == -1)
        {
            perror("send");
            return n;
        }
        total_sent += n;
    }
    return n;
}

ServerResponse receive_TCP_message(int fd)
{
    ServerResponse server_response;
    string result;
    char buffer[128];
    while (true)
    {
        ssize_t n = read(fd, buffer, sizeof(buffer) - 1);
        if (n == -1)
        {
            server_response.status = n;
            perror("receive");
            return server_response;
        }
        if (n == 0) {
            // connection closed
            break;
        }

        result.append(buffer, n);
        if (result.find('\n') != string::npos)
            break;  // end of message
    }
    
    string msg(buffer,n);
    server_response.msg = msg;
    return server_response;
}

int main(int argc, char *argv[])
{
    if (argc == 5)
    {
        ip = argv[2];
        port = argv[4];
    }
    else if (argc == 3)
    {
        /* ./user -n ESIP  OR  ./user -p ESport */
        if (strcmp(argv[1], "-n") == 0)
            ip = argv[2];
        
        else if (strcmp(argv[1], "-p") == 0)
            port = argv[2];
    }

    cout << "IP: " << ip << "\nPort: " << port << endl;

    ActiveUser activeUser;

    while(true)
    {

        cout << "> ";

        if (!getline(cin, input_buffer))
            break;

        auto args = split(input_buffer);
        if (args.empty())
            continue;

        CommandType cmd = parse_command(args[0]);

        switch (cmd)
        {
            case CMD_LOGIN: {

                if (activeUser.loggedIn)
                {
                    cout << "You are already logged in. If you want to switch accounts please logout first." << endl;
                    continue;
                }

                if (!verify_login(args))
                    continue;

                fd = establish_UDP_connection();
                
                string message;
                message = "LIN " + args[1] + " " + args[2];
                
                if (send_UDP_message(fd, message) == -1)
                {
                    cerr << "Error sending message to server." << endl;
                    continue;
                }
                ServerResponse server_response = receive_UDP_message(fd);
                if (server_response.status == -1)
                {
                    cerr << "Error receiving response from server." << endl;
                    continue;
                }
                auto args = split(server_response.msg);

                if (args[1] == "OK")
                    cout << "Successful login" << endl;
                else if (args[1] == "NOK")
                    cout << "Successful login" << endl;
                else if (args[1] == "REG")
                    cout << "New user registered" << endl;
                
                freeaddrinfo(res);
                close(fd);
                activeUser.loggedIn = true;

                break;
            }

            case CMD_CHANGEPASS: {

                if (!activeUser.loggedIn)
                {
                    cout << "You need to be logged in to change your password." << endl;
                    continue;
                }

                if(!verify_changePass(args))
                    continue;

                fd = establish_TCP_connection();

                string message;
                message = "CPS " + activeUser.userId + " " + args[1] + " " + args[2];

                if (send_TCP_message(fd, message) == -1)
                {
                    cerr << "Error sending message to server." << endl;
                    continue;
                }
                ServerResponse server_response = receive_TCP_message(fd);
                if (server_response.status == -1)
                {
                    cerr << "Error receiving response from server." << endl;
                    continue;
                }
                auto args = split(server_response.msg);

                if (args[1] == "NLG")
                    cout << "No user is logged in" << endl;
                else if (args[1] == "NOK")
                    cout << "Password is not correct" << endl;
                else if (args[1] == "NID")
                    cout << "User doesn't exist" << endl;
                else if (args[1] == "OK")
                    cout << "Password changed successfully" << endl;
                
                freeaddrinfo(res);
                close(fd);

                break;
            }

            case CMD_UNREGISTER: {

                if (!activeUser.loggedIn)
                {
                    cout << "You need to be logged in to unregister." << endl;
                    continue;
                }
                // UDP request to server
                activeUser.loggedIn = false; // if server says unregister was successful
                break;
            }

            case CMD_LOGOUT: {

                if (!activeUser.loggedIn)
                {
                    cout << "You are not logged in." << endl;
                    continue;
                }
                // UDP request to server
                activeUser.loggedIn = false; // if server says logout was successful
                break;
            }

            case CMD_EXIT: {
                
                if (activeUser.loggedIn)
                {
                    cout << "You are currently logged in. Please logout before exiting." << endl;
                    continue;
                }
                cout << "Exiting application." << endl;
                // Close any open connections
                exit(0);
                break;
            }

            case CMD_CREATE: {

                if (!activeUser.loggedIn)
                {
                    cout << "You need to be logged in to create an event." << endl;
                    continue;
                }
                // UDP request to server
                break;
            }
        }
    }

    return 0;
}

bool verify_login(vector<string> &args)
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

bool verify_changePass(vector<string> &args){

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