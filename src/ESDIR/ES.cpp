// Group 10

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

#include "ES.hpp"
#include "common.hpp"
#include "protocols.hpp"
#include "utils.hpp"

using namespace std;

int udp_fd, tcp_listen_fd;
string port = PORT;
bool verbose = false;

int main(int argc, char *argv[]) {
    
    if (argc == 4)
    {
        port = argv[2];
        verbose = true;
    }
    else if (argc == 3)
    {
        port = argv[2];
    }
    else if (argc == 2)
    {
        verbose = true;
    }

    // Signal handler setup
    struct sigaction sa;
    sa.sa_handler = handle_sigchld;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    if (sigaction(SIGCHLD, &sa, NULL) == -1)
    {
        perror("sigaction");
        exit(1);
    }

    udp_fd = setup_UDP_server(port);
    if (udp_fd == -1)
    {
        perror("socket");
        return -1;
    }

    tcp_listen_fd = setup_TCP_server(port);
    if (tcp_listen_fd == -1)
    {
        perror("socket");
        return -1;
    }

    // Select
    fd_set inputs, testfds;
    FD_ZERO(&inputs);
    FD_SET(udp_fd, &inputs);        // Socket UDP
    FD_SET(tcp_listen_fd, &inputs); // Socket TCP Listen
    struct timeval timeout;

    // O max_fd tem de ser o maior número entre os FDs
    int max_fd = max(udp_fd, tcp_listen_fd); 

    cout << "Server running on port " << port << "..." << endl;

    while (true) 
    {
        testfds = inputs; // Reload mask (select changes testfds)

        memset((void *)&timeout,0,sizeof(timeout));
        timeout.tv_sec=10;

        // Blocking select (needs timout)
        int out_fds = select(max_fd + 1, &testfds, NULL, NULL, /*&timeout*/NULL);   // FIX ME  timout value

        switch (out_fds)
        {
            case -1:
                if (errno == EINTR)     // If interrupted by signal, restart select 
                    continue; 
                perror("select");
                exit(1);

            case 0:
                // Timeout
                printf("Timeout...\n");
                break;

            default:
                // --- UDP ---
                if (FD_ISSET(udp_fd, &testfds)) 
                {
                    struct sockaddr_in client_addr;
                    socklen_t addr_len = sizeof(client_addr);
                    
                    ServerResponse client_request = receive_UDP_request(udp_fd, client_addr, addr_len);
                    cout << "Received UDP request: " << client_request.msg << endl;
                    if (client_request.status == -1)
                    {
                        cerr << "Error receiving request from client." << endl;
                        continue;
                    }
                    
                    auto args = split(client_request.msg);

                    CommandType cmd = parse_command(args[0]);

                    switch (cmd)
                    {
                        case CMD_LOGIN: 
                            login(args, udp_fd, client_addr, addr_len);
                            break;

                        case CMD_UNREGISTER: 
                            cout << "Unregister command received." << endl;
                            unregister(args, udp_fd, client_addr, addr_len);
                            break;

                        case CMD_LOGOUT: 
                            //logout(args, activeUser, ip, port, res, addr);
                            break;

                        case CMD_MYEVENTS: 
                            //myevents(args, activeUser, ip, port, res, addr);
                            break;

                        case CMD_MYRESERVATIONS: 
                            //myreservations(args, activeUser, ip, port, res, addr);
                            break;

                        case CMD_INVALID: 
                            cout << "Invalid command." << endl;
                            break;
                    }
                }

                // --- VERIFICAR TCP ---
                if (FD_ISSET(tcp_listen_fd, &testfds)) 
                {
                    struct sockaddr_in client_addr;
                    socklen_t addr_len = sizeof(client_addr);
                    
                    int new_tcp_fd = accept(tcp_listen_fd, (struct sockaddr*)&client_addr, &addr_len);
                    if (new_tcp_fd == -1) 
                    {
                        perror("accept");
                        // Aqui não podemos fazer continue porque sairia do switch, não do while.
                        // Mas como estamos no fim do loop, o break funciona bem.
                        break; 
                    }

                    // AQUI FAZEMOS O FORK
                    pid_t pid = fork();

                    if (pid == 0) 
                    {
                        // === PROCESSO FILHO ===
                        close(tcp_listen_fd); // Filho não precisa do listener
                        close(udp_fd);        // Filho não precisa do UDP
                        
                        // Tratar do pedido TCP (Ler "SGL...", enviar ficheiro)
                        // process_tcp_request(new_tcp_fd);
                        
                        close(new_tcp_fd);
                        exit(0); // Filho morre aqui
                    } 
                    else if (pid > 0) 
                    {
                        // === PROCESSO PAI ===
                        close(new_tcp_fd); // Pai fecha o descritor da conexão
                    } 
                    else 
                    {
                        perror("fork");
                        close(new_tcp_fd);
                    }
                }
                break;
        }
    }

    // Limpeza final (se saíres do while)
    close(udp_fd);
    close(tcp_listen_fd);
    return 0;
}