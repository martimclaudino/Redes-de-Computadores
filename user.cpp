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

#include "common.hpp"
#include "protocols.hpp"
#include "utils.hpp"

using namespace std;

string input_buffer;
string ip = IPADDRESS;
string port = PORT;

int main(int argc, char *argv[])
{
    struct addrinfo *res;
    struct sockaddr_in addr;

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
    activeUser.loggedIn = false;

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

                login(args, activeUser, ip, port, res, addr);

                break;
            }

            case CMD_CHANGEPASS: {

                changePass(args, activeUser, ip, port, res);

                break;
            }

            case CMD_UNREGISTER: {

                unregister(args, activeUser, ip, port, res, addr);

                break;
            }

            case CMD_LOGOUT: {

                logout(args, activeUser, ip, port, res, addr);

                break;
            }

            case CMD_EXIT: {
                
                exit(args, activeUser);
                
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