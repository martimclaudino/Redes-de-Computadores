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
#include <sys/stat.h>
#include <fstream>
#include <filesystem>

#include "common.hpp"
#include "protocols.hpp"
#include "utils.hpp"

using namespace std;
namespace fs = std::filesystem;

CommandType parse_command(const string &cmd)
{
    if (cmd == "LIN") return CMD_LOGIN;
    if (cmd == "CPS") return CMD_CHANGEPASS;
    if (cmd == "UNR") return CMD_UNREGISTER;
    if (cmd == "LOU") return CMD_LOGOUT;
    if (cmd == "CRE") return CMD_CREATE;
    if (cmd == "CLS") return CMD_CLOSE;
    if (cmd == "LME") return CMD_MYEVENTS;
    if (cmd == "LST") return CMD_LIST;
    if (cmd == "SED") return CMD_SHOW;
    if (cmd == "RID") return CMD_RESERVE;
    if (cmd == "LMR") return CMD_MYRESERVATIONS;
    return CMD_INVALID;
}

void handle_sigchld(int sig) 
{
    (void)sig; // Silence unused warning
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

int setup_UDP_server(string port)
{
    struct addrinfo hints, *res;
    int fd;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;      // IPv4
    hints.ai_socktype = SOCK_DGRAM; // UDP
    hints.ai_flags = AI_PASSIVE;    // Listens from any IP

    int errcode = getaddrinfo(NULL, port.c_str(), &hints, &res);
    if (errcode != 0) 
    {
        cerr << "getaddrinfo error: " << gai_strerror(errcode) << endl;
        return -1;
    }

    fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (fd == -1) 
    {
        perror("socket");
        freeaddrinfo(res);
        return -1;
    }
    
    if (bind(fd, res->ai_addr, res->ai_addrlen) == -1) 
    {
        perror("bind");
        freeaddrinfo(res);
        close(fd);
        return -1;
    }

    freeaddrinfo(res);
    return fd;
}

int setup_TCP_server(string port)
{
    struct addrinfo hints, *res;
    int fd;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM; // TCP
    hints.ai_flags = AI_PASSIVE;

    int errcode = getaddrinfo(NULL, port.c_str(), &hints, &res);
    if (errcode != 0) {
        cerr << "getaddrinfo error: " << gai_strerror(errcode) << endl;
        return -1;
    }

    fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (fd == -1) {
        perror("socket");
        freeaddrinfo(res);
        return -1;
    }

    // Avoid "Address already in use"
    int yes = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
        perror("setsockopt");
        freeaddrinfo(res);
        close(fd);
        return -1;
    }

    if (bind(fd, res->ai_addr, res->ai_addrlen) == -1) {
        perror("bind");
        freeaddrinfo(res);
        close(fd);
        return -1;
    }

    if (listen(fd, 5) == -1) {
        perror("listen");
        freeaddrinfo(res);
        close(fd);
        return -1;
    }

    freeaddrinfo(res);
    return fd;
}

ServerResponse receive_UDP_request(int fd, struct sockaddr_in &addr, socklen_t &addrlen)
{
    ServerResponse server_response;
    char buffer[MYEVENTS_RESPONSE];     // biggest size in UDP messages
    
    ssize_t n = recvfrom(fd, buffer, sizeof(buffer)-1, 0, (struct sockaddr *)&addr, &addrlen);

    server_response.status = n;
    
    if (n == -1) 
    {
        perror("recvfrom");
        return server_response;
    }
    string msg(buffer,n);
    server_response.msg = msg;
    return server_response;
}

int send_UDP_reply(int fd, string message, struct sockaddr_in addr, socklen_t addrlen)
{
    ssize_t n = sendto(fd, message.c_str(), message.size(), 0, (struct sockaddr*)&addr, addrlen);
    
    if (n == -1)
    {
        perror("sendto");
        exit(EXIT_FAILURE);
        return n;
    }
    return n;
}

ServerResponse verify_login(const vector<string> &args) // FIX ME i could use this on common.cpp and in the UserApp
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

int login(vector<string> &args, int fd, struct sockaddr_in addr, socklen_t addrlen)
{
    ServerResponse response = verify_login(args);
    if (response.status == -1)
    {
        cout << response.msg;
        return 1;
    }
    string UID = args[1];
    string password = args[2];
    string userpath = "src/ESDIR/USERS/" + UID;
    struct stat st;

    if (stat(userpath.c_str(), &st) == -1) 
    {
        // User has not been registered, needs to be registered
        if (mkdir(userpath.c_str(), 0700) == -1)
        {
            perror("mkdir");
            return 1;
        }
        mkdir((userpath + "/CREATED").c_str(), 0700);
        mkdir((userpath + "/RESERVED").c_str(), 0700);
        string pass_file = userpath + "/password.txt";
        ofstream p_file(pass_file);
        p_file << password;
        p_file.close();
        string login_file = userpath + "/login.txt";
        ofstream l_file(login_file);
        l_file.close();

        string msg = "RLI REG\n";
        
        if (send_UDP_reply(fd, msg, addr, addrlen) == -1)
        {
            cerr << "Error sending reply to client." << endl;
            return 1;
        }

        return 0;
    } else 
    {
        // Diretoria EXISTE -> Verificar password ou Re-registo
    }
    return 0;
}