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

int send_UDP_reply(int fd, const string message, struct sockaddr_in addr, socklen_t addrlen)
{
    ssize_t n = sendto(fd, message.c_str(), message.size(), 0, (struct sockaddr*)&addr, addrlen);
    
    if (n == -1)
    {
        perror("Failed to send UDP reply");
        return n;
    }
    return n;
}

int register_user(string UID, string password, string user_path, string pass_file, string login_file, struct stat &st)
{
    if (stat(user_path.c_str(), &st) == -1) 
    {
        if (mkdir(user_path.c_str(), 0700) == -1)
        {
            perror("mkdir");
            return 1;
        }
        mkdir((user_path + "/CREATED").c_str(), 0700);
        mkdir((user_path + "/RESERVED").c_str(), 0700);
    }
    ofstream p_file(pass_file);
    p_file << password;
    p_file.close();
    ofstream l_file(login_file);
    l_file.close();
    return 0;
}

bool compare_passwords(string pass_file, string input_pass)
{
    ifstream p_file(pass_file);
    string stored_pass;
    p_file >> stored_pass;
    p_file.close();
    return stored_pass == input_pass;
}

bool is_loggedin(string login_file)
{
    struct stat st;
    return (stat(login_file.c_str(), &st) != -1);
}

bool is_registered(string user_path, string pass_file)
{
    struct stat st;
    return ((stat(user_path.c_str(), &st) == 0) && (stat(pass_file.c_str(), &st) == 0));
}

ServerResponse verify_login(const vector<string> &args)
{
    ServerResponse response;
    response.msg = "";
    response.status = 1;
    if (args.size() != 3)
    {
        response.status = -1;
        return response;
    }
    string username = args[1];
    string password = args[2];

    if (username.length() != 6 || password.length() != 8)
    {
        response.status = -1;
        return response;
    }
    return response;
}

int login(vector<string> &args, int fd, struct sockaddr_in addr, socklen_t addrlen)
{
    ServerResponse login = verify_login(args);
    if (login.status == -1)
    {
        string msg = "RLI ERR\n";
        send_UDP_reply(fd, msg, addr, addrlen);
        return 1;
    }
    string UID = args[1];
    string password = args[2];
    string user_path = "src/ESDIR/USERS/" + UID;
    string pass_file = user_path + "/password.txt";
    string login_file = user_path + "/login.txt";
    struct stat st;

    if (!is_registered(user_path, pass_file)) 
    {
        // User has not been registered, needs to be registered
        register_user(UID, password, user_path, pass_file, login_file, st);

        string msg = "RLI REG\n";
        
        send_UDP_reply(fd, msg, addr, addrlen);

        return 0;
    } 
    // User is registered, verify password
    if (compare_passwords(pass_file, password)) 
    {
        ofstream l_file(login_file);
        l_file.close();

        string msg = "RLI OK\n";

        send_UDP_reply(fd, msg, addr, addrlen);
        return 0;
    } 
    string msg = "RLI NOK\n";

    send_UDP_reply(fd, msg, addr, addrlen);

    return 0;
}

ServerResponse verify_unregister(const vector<string> &args)
{
    ServerResponse response;
    response.status = 1;
    response.msg = "";
    if (args.size() != 3)
    {
        response.status = -1;
        return response;
    }
    string username = args[1];
    string password = args[2];

    if (username.length() != 6 || password.length() != 8)
    {
        response.status = -1;
        return response;
    }
    return response;
}

int unregister(vector<string> &args, int fd, struct sockaddr_in addr, socklen_t addrlen)
{
    ServerResponse unregister = verify_unregister(args);
    if (unregister.status == -1)
    {
        string msg = "RUR ERR\n";
        send_UDP_reply(fd, msg, addr, addrlen);
        return 1;
    }
    string UID = args[1];
    string password = args[2];
    string user_path = "src/ESDIR/USERS/" + UID;
    string pass_file = user_path + "/password.txt";
    string login_file = user_path + "/login.txt";
    if (!is_loggedin(login_file))
    {
        string msg = "RUR NOK\n";
        send_UDP_reply(fd, msg, addr, addrlen);
        return 0;
    }
    if (!is_registered(user_path, pass_file))
    {
        string msg = "RUR UNR\n";
        send_UDP_reply(fd, msg, addr, addrlen);
        return 0;
    }
    if (!compare_passwords(pass_file, password))
    {
        string msg = "RUR WRP\n";
        send_UDP_reply(fd, msg, addr, addrlen);
        return 0;
    }
    if (unlink(login_file.c_str()) != 0) {
        perror("Erro ao apagar login file");
        return 1;
    }
    if (unlink(pass_file.c_str()) != 0) {
        perror("Erro ao apagar pass file");
        return 1;
    }
    string msg = "RUR OK\n";
    send_UDP_reply(fd, msg, addr, addrlen);
    return 0;
}