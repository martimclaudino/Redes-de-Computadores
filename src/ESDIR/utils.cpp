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

ServerResponse receive_TCP_request(int fd)
{
    ServerResponse server_response;
    string msg = "";
    char ch;
    ssize_t n;

    while (true)
    {
        n = read(fd, &ch, 1);

        if (n == -1) 
        {
            // Read error
            server_response.status = -1;    
            perror("read tcp");
            return server_response;
        }
        else if (n == 0) 
        {
            // Connection closed by client (EOF)
            // If the message is not empty, return what was read so far
            if (msg.empty()) server_response.status = 0; // Clean close
            else server_response.status = 1; // Closed halfway (or end without \n)
            break; 
        }

        msg += ch;

        // If the end of the line is found, stop reading immediately
        if (ch == '\n') 
        {
            server_response.status = 1;
            break;
        }
    }

    server_response.msg = msg;
    return server_response;
}

ssize_t send_TCP_reply(int fd, const string &message)
{
    ssize_t total_sent = 0;
    ssize_t message_length = message.size();
    ssize_t n;
    while (total_sent < message_length)
    {
        // MSG_NOSIGNAL avoids server crashing if client disconnects abruptly (prevents SIGPIPE)
        n = send(fd, message.c_str() + total_sent, message_length - total_sent, MSG_NOSIGNAL);
        
        if (n == -1)
        {
            perror("Failed to send TCP reply");
            return -1;
        }
        if (n == 0) 
        {
            break; 
        }
        total_sent += n;
    }
    return total_sent;
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
// For UDP functions
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
        string msg = "RLO ERR\n";
        send_UDP_reply(fd, msg, addr, addrlen);
        return 1;
    }
    string UID = args[1];
    string password = args[2];
    struct stat st;

    if (!is_loggedin(UID)) 
    {
        string msg = "RLO NOK\n";
        send_UDP_reply(fd, msg, addr, addrlen);
        return 0;
    }
    if (!is_registered(UID)) 
    {
        string msg = "RLO UNR\n";
        send_UDP_reply(fd, msg, addr, addrlen);
        return 0;
    } 
    if (compare_passwords(UID, password)) 
    {
        logout_user(UID);
        string msg = "RLO OK\n";
        send_UDP_reply(fd, msg, addr, addrlen);
        return 0;
    } 
    string msg = "RLO WRP\n";

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

    if (event_time < current_time)
    {   
        ofstream out_file(end_file);
        out_file << date << " " << hour << "\n";
        out_file.close(); 
        return "0";
    }
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
        string msg = "RME ERR\n";
        send_UDP_reply(fd, msg, addr, addrlen);
        return 1;
    }
    string UID = args[1];
    string password = args[2];

    if (!is_loggedin(UID)) 
    {
        string msg = "RME NLG\n";
        send_UDP_reply(fd, msg, addr, addrlen);
        return 0;
    }
    if (!compare_passwords(UID, password)) 
    {
        string msg = "RME WRP\n";
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

int myreservations(vector<string> &args, int fd, struct sockaddr_in addr, socklen_t addrlen)
{
    ServerResponse myreservations = verify_credentials(args);
    if (myreservations.status == -1)
    {
        string msg = "RMR ERR\n";
        send_UDP_reply(fd, msg, addr, addrlen);
        return 1;
    }
    string UID = args[1];
    string password = args[2];

    if (!is_loggedin(UID)) 
    {
        string msg = "RMR NLG\n";
        send_UDP_reply(fd, msg, addr, addrlen);
        return 0;
    }
    if (!compare_passwords(UID, password)) 
    {
        string msg = "RMR WRP\n";
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
    int total_reservations = events.size();
    int limit = 50;
    int start_index = 0;
    int count_to_send = total_reservations;

    // Only the last 50 reservations
    if (total_reservations > limit) 
    {
        start_index = total_reservations - limit;
        count_to_send = limit;
    }
    string msg = "RMR OK "; 

    for (int i = start_index; i < total_reservations; i++) 
    {
        msg += " " + events[i];
    }
    msg += "\n";
    send_UDP_reply(fd, msg, addr, addrlen);
    
    return 0;
}

ServerResponse verify_create(const vector<string> &args)
{
    ServerResponse response;
    response.msg = "";
    response.status = 1;
    if (args[3].length() > 10)
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
    int day, month, year, hour, minute;
    if (sscanf(args[4].c_str(), "%2d-%2d-%4d", &day, &month, &year) != 3)
    {
        response.status = -1;  
        return response;
    }
    if (sscanf(args[5].c_str(), "%2d:%2d", &hour, &minute) != 2)
    {
        response.status = -1;
        return response;
    }
    if (day < 1 || day > 31 || month < 1 || month > 12 || hour < 0 || hour > 23 || minute < 0 || minute > 59)
    {
        response.status = -1;
        return response;
    }
    if (!std::filesystem::exists(args[7]))
    {
        response.status = -1;
        return response;
    }
    return response;
}

string get_next_eid() 
{
    int max_id = 0;
    string path = "src/ESDIR/EVENTS";

    if (!fs::exists(path)) 
    {
        fs::create_directories(path);
        return "001";
    }
    for (const auto & entry : fs::directory_iterator(path)) 
    {
        if (entry.is_directory()) {
            string dirname = entry.path().filename().string();
            int id = stoi(dirname);
            if (id > max_id) max_id = id;
        }
    }
    stringstream ss;
    ss << setfill('0') << setw(3) << (max_id + 1);
    return ss.str();
}

int create(vector<string> &args, int fd, struct sockaddr_in addr, socklen_t addrlen)
{
    ServerResponse create = verify_create(args);
    if (create.status == -1)
    {
        string msg = "RCE ERR\n";
        send_TCP_reply(fd, msg);
        return 1;
    }
    string UID = args[1];
    string password = args[2];
    if (!is_loggedin(UID)) 
    {
        string msg = "RCE NLG\n";
        send_TCP_reply(fd, msg);
        return 0;
    }
    if (!compare_passwords(UID, password)) 
    {
        string msg = "RCE WRP\n";
        send_TCP_reply(fd, msg);
        return 0;
    }
    string file_name = args[7];
    string file_size = args[8];
    string EID = get_next_eid();
    // EVENT DIR
    string event_path = "src/ESDIR/EVENTS/" + EID;
    if (!fs::create_directories(event_path)) 
    {
        string msg = "RCE NOK\n";
        send_TCP_reply(fd, msg);
        return 0;
    }
    // RESERVATION DIR
    string reservations_path = "src/ESDIR/EVENTS/" + EID +  "/RESERVATIONS";
    if (!fs::create_directories(reservations_path))
    {
        string msg = "RCE NOK\n";
        send_TCP_reply(fd, msg);
        return 0;
    }
    string event_name = args[3];
    string date = args[4];
    string hour = args[5];
    string attendance = args[6];
    // start.txt file
    string start_file_path = event_path + "/start.txt";
    ofstream start_file(start_file_path);
    start_file << UID << " " << event_name << " " << file_name << " " << attendance << " " << date << " " << hour << "\n";
    start_file.close(); 
    // res.txt
    string res_file_path = event_path + "/res.txt";
    ofstream res_file(res_file_path);
    res_file << "0" << "\n";
    // DESCRIPTION DIR
    string description_path = "src/ESDIR/EVENTS/" + EID + "/DESCRIPTION";
    if (!fs::create_directories(description_path)) 
    {
        string msg = "RCE NOK\n";
        send_TCP_reply(fd, msg);
        return 0;
    }
    string description_file_path = description_path + "/" + file_name;
    ofstream description_file(description_file_path);
    description_file << "\n";
    // Fdata starts at args[9]
    for (size_t i = 9; i < args.size(); i++) 
    {
        description_file << args[i];
        if (i < args.size() - 1) 
            description_file << " ";
    }
    description_file.close();
    string msg = "RCE OK " + EID + "\n";
    send_TCP_reply(fd, msg);

    return 0;
}