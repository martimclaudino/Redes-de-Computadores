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
#include <ctime>

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

void verbose_mode(string client_ip, int client_port, string command)
{
    cout << "-----------------------------" << endl;
    cout << "|IP: " << client_ip << "\n" << "|PORT: " << client_port << "\n" 
         << "|Command: " << command; 
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

int register_user(string UID, string password, struct stat &st)
{
    string user_path = "src/ESDIR/USERS/" + UID;
    string pass_file = user_path + "/password.txt";
    string login_file = user_path + "/login.txt";
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
    login_user(UID);
    return 0;
}

bool compare_passwords(string UID, string input_pass)
{
    string user_path = "src/ESDIR/USERS/" + UID;
    string pass_file = user_path + "/password.txt";
    ifstream p_file(pass_file);
    string stored_pass;
    p_file >> stored_pass;
    p_file.close();
    return stored_pass == input_pass;
}

bool is_loggedin(string UID)
{
    string user_path = "src/ESDIR/USERS/" + UID;
    string login_file = user_path + "/login.txt";
    struct stat st;
    return (stat(login_file.c_str(), &st) != -1);
}

bool is_registered(string UID)
{
    string user_path = "src/ESDIR/USERS/" + UID;
    string pass_file = user_path + "/password.txt";
    struct stat st;
    return ((stat(user_path.c_str(), &st) == 0) && (stat(pass_file.c_str(), &st) == 0));
}

int delete_file(const string file_path)
{
    if (unlink(file_path.c_str()) != 0) {
        perror("Failed to delete file");
        return -1;
    }
    return 0;
}

ServerResponse verify_credentials(const vector<string> &args)
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

void login_user(string UID)
{
    string user_path = "src/ESDIR/USERS/" + UID;
    string login_file = user_path + "/login.txt";
    ofstream l_file(login_file);
    l_file.close();
}

void logout_user(string UID)
{
    string user_path = "src/ESDIR/USERS/" + UID;
    string login_file = user_path + "/login.txt";
    delete_file(login_file);
}

int login(vector<string> &args, int fd, struct sockaddr_in addr, socklen_t addrlen)
{
    ServerResponse login = verify_credentials(args);
    if (login.status == -1)
    {
        string msg = "RLI ERR\n";
        send_UDP_reply(fd, msg, addr, addrlen);
        return 1;
    }
    string UID = args[1];
    string password = args[2];
    struct stat st;

    if (!is_registered(UID)) 
    {
        // User has not been registered, needs to be registered
        register_user(UID, password, st);
        string msg = "RLI REG\n";
        send_UDP_reply(fd, msg, addr, addrlen);
        return 0;
    } 
    // User is registered, verify password
    if (compare_passwords(UID, password)) 
    {
        login_user(UID);
        string msg = "RLI OK\n";
        send_UDP_reply(fd, msg, addr, addrlen);
        return 0;
    } 
    string msg = "RLI NOK\n";
    send_UDP_reply(fd, msg, addr, addrlen);
    return 0;
}

int unregister(vector<string> &args, int fd, struct sockaddr_in addr, socklen_t addrlen)
{
    ServerResponse unregister = verify_credentials(args);
    if (unregister.status == -1)
    {
        string msg = "RUR ERR\n";
        send_UDP_reply(fd, msg, addr, addrlen);
        return 1;
    }
    string UID = args[1];
    string password = args[2];
    if (!is_loggedin(UID))
    {
        string msg = "RUR NOK\n";
        send_UDP_reply(fd, msg, addr, addrlen);
        return 0;
    }
    if (!is_registered(UID))
    {
        string msg = "RUR UNR\n";
        send_UDP_reply(fd, msg, addr, addrlen);
        return 0;
    }
    if (!compare_passwords(UID, password))
    {
        string msg = "RUR WRP\n";
        send_UDP_reply(fd, msg, addr, addrlen);
        return 0;
    }
    logout_user(UID);
    string user_path = "src/ESDIR/USERS/" + UID;
    string pass_file = user_path + "/password.txt";
    delete_file(pass_file.c_str());
    string msg = "RUR OK\n";
    send_UDP_reply(fd, msg, addr, addrlen);
    return 0;
}

int logout(vector<string> &args, int fd, struct sockaddr_in addr, socklen_t addrlen)
{
    ServerResponse logout = verify_credentials(args);
    if (logout.status == -1)
    {
        string msg = "RLI ERR\n";
        send_UDP_reply(fd, msg, addr, addrlen);
        return 1;
    }
    string UID = args[1];
    string password = args[2];
    struct stat st;

    if (!is_loggedin(UID)) 
    {
        string msg = "RLI NOK\n";
        send_UDP_reply(fd, msg, addr, addrlen);
        return 0;
    }
    if (!is_registered(UID)) 
    {
        string msg = "RLI UNR\n";
        send_UDP_reply(fd, msg, addr, addrlen);
        return 0;
    } 
    if (compare_passwords(UID, password)) 
    {
        logout_user(UID);
        string msg = "RLI OK\n";
        send_UDP_reply(fd, msg, addr, addrlen);
        return 0;
    } 
    string msg = "RLI WRP\n";

    send_UDP_reply(fd, msg, addr, addrlen);

    return 0;
}

vector<string> get_event_data(string EID)
{
    string event_path = "src/ESDIR/EVENTS/" + EID;
    string start_file = event_path + "/start.txt";
    struct stat st;
    if (stat(start_file.c_str(), &st) == -1) 
    {
        return {};
    }
    ifstream e_file(start_file);
    vector<string> event_data;
    string arg;
    while (e_file >> arg)
    {
        event_data.push_back(arg);
    }
    e_file.close();
    return event_data;
}

string get_event_state(string EID)
{
    string event_path = "src/ESDIR/EVENTS/" + EID;
    string start_file = event_path + "/start.txt";
    struct stat st;
    string end_file = event_path + "/end.txt"; 
    if (stat(end_file.c_str(), &st) == 0) 
    {
        return "3";
    }
    string uid, event_name, desc_fname, attendees, date, hour;
    vector<string> event_data = get_event_data(EID);
    uid = event_data[0];
    event_name = event_data[1];
    desc_fname = event_data[2];
    attendees = event_data[3];
    date = event_data[4];
    hour = event_data[5];
    struct tm t = {0};
    t.tm_mday = stoi(date.substr(0, 2));
    t.tm_mon  = stoi(date.substr(3, 2)) - 1;
    t.tm_year = stoi(date.substr(6, 4)) - 1900; // Number of years since 1900
    t.tm_hour = stoi(hour.substr(0, 2));  
    t.tm_min  = stoi(hour.substr(3, 2));
    t.tm_sec  = 0;

    time_t event_time = mktime(&t);
    time_t current_time = time(NULL);

    if (event_time < current_time)      // FIX ME tenho de criar o ficheiro end.txt aqui
        return "0";
    string reservations;
    string res_file = event_path + "/res.txt";
    ifstream r_file(res_file);
    r_file >> reservations;
    r_file.close();

    if (!(reservations < attendees))
        return "2";
    
    return "1";
}

vector<string> list_created_events(string UID)
{
    string user_path = "src/ESDIR/USERS/" + UID;
    string created_path = user_path + "/CREATED";
    string event_list = "";
    int count = 0;

    for (const auto & entry : fs::directory_iterator(created_path)) 
    {
        if (entry.is_regular_file())    // Ignores dir's, ".", ".."
        { 
            string filename = entry.path().filename().string();
            // Removes ".txt"
            string EID = filename.substr(0, filename.find(".txt"));
            string state = get_event_state(EID);
            event_list += EID + " " + state + " ";
            count++;
        }
    }
    if (count == 0)
    {
        return {};
    }
    return split(event_list);
}

int myevents(vector<string> &args, int fd, struct sockaddr_in addr, socklen_t addrlen)
{
    ServerResponse myevents = verify_credentials(args);
    if (myevents.status == -1)
    {
        string msg = "RLI ERR\n";
        send_UDP_reply(fd, msg, addr, addrlen);
        return 1;
    }
    string UID = args[1];
    string password = args[2];

    if (!is_loggedin(UID)) 
    {
        string msg = "RLI NLG\n";
        send_UDP_reply(fd, msg, addr, addrlen);
        return 0;
    }
    if (!compare_passwords(UID, password)) 
    {
        string msg = "RLI WRP\n";
        send_UDP_reply(fd, msg, addr, addrlen);
        return 0;
    }
    vector<string> events = list_created_events(UID);
    if (events.empty())
    {
        string msg = "RME NOK\n";
        send_UDP_reply(fd, msg, addr, addrlen);
        return 0;
    }

    string msg = "RME OK";
    for (const auto &event_info : events) 
    {
        msg += " " + event_info;
    }
    msg += "\n";

    send_UDP_reply(fd, msg, addr, addrlen);

    return 0;
}

int get_reservations(string EID, string UID, string date, string time)
{
    string reservation_path = "src/ESDIR/EVENTS/" + EID + "/RESERVATIONS";
    string reservation_file = reservation_path + "/" + UID + "-" + date + "-" + time + ".txt";
    struct stat st;
    if (stat(reservation_file.c_str(), &st) == -1) 
    {
        return 0;
    }
    // Our reservation file only has the number of reservations
    // If UID and date-time are on the name, it's redundant to store them in the file
    ifstream r_file(reservation_file);
    string reservations;
    r_file >> reservations;
    r_file.close();
    return stoi(reservations);
}

vector<string> list_reserved_events(string UID)
{
    string user_path = "src/ESDIR/USERS/" + UID;
    string reserved_path = user_path + "/RESERVED";
    string event_list = "";
    int count = 0;

    for (const auto & entry : fs::directory_iterator(reserved_path)) 
    {
        if (entry.is_regular_file())    // Ignores dir's, ".", ".."
        { 
            string filename = entry.path().filename().string();
            // EID-date-time.txt
            string EID = filename.substr(0, 3);
            string date = filename.substr(4, 10);
            string time = filename.substr(15, 8);
            int reservations = get_reservations(EID, UID, date, time);                  // FIX ME eu posso só ver no proprio ficheiro em vez de ir aos EVENTS/
            event_list += EID + " " + date + " " + time + " " + to_string(reservations) + " ";
            count++;
        }
    }
    if (count == 0)
    {
        return {};
    }
    return split(event_list);
}

int myreservations(vector<string> &args, int fd, struct sockaddr_in addr, socklen_t addrlen)    // FIX ME como é que vejo só as ultimas 50?
{
    ServerResponse myreservations = verify_credentials(args);
    if (myreservations.status == -1)
    {
        string msg = "RLI ERR\n";
        send_UDP_reply(fd, msg, addr, addrlen);
        return 1;
    }
    string UID = args[1];
    string password = args[2];

    if (!is_loggedin(UID)) 
    {
        string msg = "RLI NLG\n";
        send_UDP_reply(fd, msg, addr, addrlen);
        return 0;
    }
    if (!compare_passwords(UID, password)) 
    {
        string msg = "RLI WRP\n";
        send_UDP_reply(fd, msg, addr, addrlen);
        return 0;
    }
    vector<string> events = list_reserved_events(UID);
    if (events.empty())
    {
        string msg = "RMR NOK\n";
        send_UDP_reply(fd, msg, addr, addrlen);
        return 0;
    }
    string msg = "RMR OK";
    for (const auto &event_info : events) 
    {
        msg += " " + event_info;
    }
    msg += "\n";
    send_UDP_reply(fd, msg, addr, addrlen);
    
    return 0;
}