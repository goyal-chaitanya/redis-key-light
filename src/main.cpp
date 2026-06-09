#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <algorithm>
#include "parser.h"

#include <cerrno>
#include <cstring>

#include <fstream>

#include <sys/epoll.h>
#include <fcntl.h>

#include "store.h"

using namespace std;

void set_nonblocking(int fd){
    int flags = fcntl(fd, F_GETFL, 0);
    if(flags == -1) return;
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

void load_aof(KeyValueStore& store){
    ifstream file("appendonly.aof", ios::binary);
    if(!file.is_open()){
        cout << "No AOF file found. Starting with a fresh database!\n";
        return;
    }

    cout << "Loading data from AOF file...\n";
    string line;

    while(getline(file, line)){
        if(line.empty() || line[0] != '*') continue;

        if(line.back() == '\r') line.pop_back();

        int num_args = stoi(line.substr(1));
        vector<string> args;

        for(int i=0; i< num_args; i++){
            getline(file, line);
            getline(file, line);
            if(line.back() == '\r') line.pop_back();
            args.push_back(line);
        }

        if(!args.empty()){
            string cmd = args[0];
            transform(cmd.begin(), cmd.end(), cmd.begin(), ::toupper);

            if(cmd == "SET" && args.size() >= 3){
                int ttl = -1;
                if(args.size() == 5 && args[3] == "EX"){
                    ttl = stoi(args[4]);
                }
                store.set(args[1], args[2], ttl);
            } else if(cmd == "DEL" && args.size() >= 2){
                store.del(args[1]);
            }
        }
    }

    cout << "AOF succesfully loaded into the memory...\n";
}


int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(server_fd < 0){
        cerr << "Failed to create socket\n";
        return 1;
    }

    int reuse = 1;
    if(setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0){
        cerr << "Failed to set setsockopt(SO_REUSEADDR)\n";
        close(server_fd);
        return 1;
    }

    struct sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(6379);


    if(bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0){
        cerr << "Bind failed to port 6379\n";
        close(server_fd);
        return 1;
    }
    
    if(listen(server_fd, SOMAXCONN) < 0){
        cerr << "Listen failed\n";
        close(server_fd);
        return 1;
    }

    set_nonblocking(server_fd);

    int epoll_fd = epoll_create1(0);
    if(epoll_fd == -1){
        cerr << "Failed to create epoll file descriptor\n";
        return 1;
    }

    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = server_fd;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &event);

    cout << "Redis Key Light listening to port 6379 (Multiplexed)...\n";

    KeyValueStore kv_store;

    load_aof(kv_store);

    const int MAX_EVENTS = 10;
    struct epoll_event events[MAX_EVENTS];

    while(true){
        int num_events = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);

        for(int i=0; i<num_events; i++){
            if(events[i].data.fd == server_fd){
                struct sockaddr_in client_address{};
                socklen_t client_len = sizeof(client_address);
                int client_fd = accept(server_fd, (struct sockaddr*)&client_address, &client_len);

                if(client_fd >= 0){
                    cout << "New client connected! FD: " << client_fd << "\n";

                    set_nonblocking(client_fd);

                    struct epoll_event client_event;
                    client_event.events = EPOLLIN;
                    client_event.data.fd = client_fd;
                    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &client_event);
                }
            }
            else{
                int client_fd = events[i].data.fd;
                char buffer[1024];
                memset(buffer, 0, sizeof(buffer));
                ssize_t bytes_read = read(client_fd, buffer, sizeof(buffer)-1);

                if(bytes_read <= 0){
                    cout << "Client disconnected. FD: " << client_fd << "\n";
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
                    close(client_fd);
                } 
                else {
                    string_view view(buffer, bytes_read);
                    vector<string> parsed_command = parse_resp(view);

                    string reply;

                    if(!parsed_command.empty()){
                        string cmd = parsed_command[0];
                        transform(cmd.begin(), cmd.end(), cmd.begin(), ::toupper);

                        if(cmd == "COMMAND" || cmd == "PING"){
                            reply = "+PONG\r\n";
                        }
                        else if(cmd == "SET" && parsed_command.size() >= 3){
                            int ttl = -1;

                            if(parsed_command.size() == 5){
                                string flag = parsed_command[3];
                                transform(flag.begin(), flag.end(), flag.begin(), ::toupper);
                                if(flag == "EX"){
                                    ttl = stoi(parsed_command[4]);
                                }
                            }

                            kv_store.set(parsed_command[1], parsed_command[2], ttl);

                            ofstream aof("appendonly.aof", ios::app | ios::binary);
                            if(aof.is_open()){
                                aof.write(buffer, bytes_read);
                            }

                            reply = "+OK\r\n";
                        }
                        else if(cmd == "GET" && parsed_command.size() >= 2){
                            auto value = kv_store.get(parsed_command[1]);
                            if(value){
                                reply = "$" + to_string(value->length()) + "\r\n" + *value + "\r\n";
                            } else{
                                reply = "$-1\r\n";
                            }
                        }
                        else if(cmd == "DEL" && parsed_command.size() >= 2){
                            int count = kv_store.del(parsed_command[1]);

                            if(count > 0){
                                ofstream aof("appendonly.aof", ios::app | ios::binary);
                                if(aof.is_open()){
                                    aof.write(buffer, bytes_read);
                                }
                            }

                            reply = ":" + to_string(count) + "\r\n";
                        }
                        else{
                            reply = "-ERR unknown command or wrong number of arguments\r\n";
                        }
                    }
                    else{
                        cout << "Received unrecognised or empty command! \n";
                    }

                    write(client_fd, reply.c_str(), reply.length());
                }
            }
        }
    }

    close(server_fd);
    return 0;
}