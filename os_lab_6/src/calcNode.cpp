#include <string>
#include <chrono>
#include <sstream>
#include <zmq.hpp>
#include <csignal>
#include <iostream>
#include <unordered_map>

#include "socketRoutine.hpp"

int main(int argc, char* argv[]) {
    if(argc != 3) {
        std::cerr << "Not enough parameters" << std::endl;
        exit(-1);
    }
    int id = std::stoi(argv[1]);
    int parentPort = std::stoi(argv[2]);
    zmq::context_t ctx;
    zmq::socket_t parentSocket(ctx, ZMQ_REP);
    std::string portTemplate = "tcp://127.0.0.1:";
    parentSocket.connect(portTemplate + std::to_string(parentPort));
    std::unordered_map<int, zmq::socket_t> sockets;
    std::unordered_map<int, int> pids;
    std::unordered_map<int, int> ports;
    auto start = std::chrono::high_resolution_clock::now();
    auto stop = std::chrono::high_resolution_clock::now();
    auto time = 0;
    bool clockStarted = false;
    while(true) {
        std::string action = ReceiveMessage(parentSocket);
        std::stringstream s(action);
        std::string command;
        s >> command;
        if(command == "pid") {
            std::string reply = "Ok: " + std::to_string(getpid());
            SendMessage(parentSocket, reply);
        } else if(command == "create") {
            int size, nodeId;
            s >> size;
            std::vector<int> path(size);
            for(int i = 0; i < size; ++i) {
                s >> path[i];
            }
            s >> nodeId;
            if(size == 0) {
                auto socket = zmq::socket_t(ctx, ZMQ_REQ);
                socket.setsockopt(ZMQ_SNDTIMEO, 5000);
                socket.setsockopt(ZMQ_LINGER, 5000);
                socket.setsockopt(ZMQ_RCVTIMEO, 5000);
                socket.setsockopt(ZMQ_REQ_CORRELATE, 1);
                socket.setsockopt(ZMQ_REQ_RELAXED, 1);
                sockets.emplace(nodeId, std::move(socket));
                int port = BindSocket(sockets.at(nodeId));
                std::cout << port << std::endl;
                int pid = fork();
                if(pid == -1) {
                    SendMessage(parentSocket, "Unable to fork");
                } else if(pid == 0) {
                    CreateNode(nodeId, port);
                } else {
                    ports[nodeId] = port;
                    pids[nodeId] = pid;
                    SendMessage(sockets.at(nodeId), "pid");
                    SendMessage(parentSocket, ReceiveMessage(sockets.at(nodeId)));
                }
            } else {
                int nextId = path.front();
                path.erase(path.begin());
                std::stringstream msg;
                msg << "create " << path.size();
                for(int i : path) {
                    msg << " " << i;
                }
                msg << " " << nodeId;
                SendMessage(sockets.at(nextId), msg.str());
                SendMessage(parentSocket, ReceiveMessage(sockets.at(nextId)));
            }
        } else if(command == "remove") {
            int size, nodeId;
            s >> size;
            std::vector<int> path(size);
            for(int i = 0; i < size; ++i) {
                s >> path[i];
            }
            s >> nodeId;
            if(path.empty()) {
                SendMessage(sockets.at(nodeId), "kill");
                ReceiveMessage(sockets.at(nodeId));
                kill(pids[nodeId], SIGTERM);
                kill(pids[nodeId], SIGKILL);
                pids.erase(nodeId);
                sockets.at(nodeId).disconnect(portTemplate + std::to_string(ports[nodeId]));
                ports.erase(nodeId);
                sockets.erase(nodeId);
                SendMessage(parentSocket, "Ok");
            } else {
                int nextId = path.front();
                path.erase(path.begin());
                std::stringstream msg;
                msg << "remove " << path.size();
                for(int i : path) {
                    msg << " " << i;
                }
                msg << " " << nodeId;
                SendMessage(sockets.at(nextId), msg.str());
                SendMessage(parentSocket, ReceiveMessage(sockets.at(nextId)));
            }
        } else if(command == "exec") {
            int size;
            std::string subcommand;
            s >> subcommand >> size;
            std::vector<int> path(size);
            for(int i = 0; i < size; ++i) {
                s >> path[i];
            }
            if(path.empty()) {
                if(subcommand == "start") {
                    start = std::chrono::high_resolution_clock::now();
                    clockStarted = true;
                    SendMessage(parentSocket, "Ok:" + std::to_string(id));
                } else if(subcommand == "stop") {
                    if(clockStarted) {
                        stop = std::chrono::high_resolution_clock::now();
                        time += std::chrono::duration_cast<std::chrono::milliseconds>
                            (stop - start).count();
                        clockStarted = false;
                    }
                    SendMessage(parentSocket, "Ok:" + std::to_string(id));
                } else if(subcommand == "time") {
                    SendMessage(parentSocket, "Ok: " + std::to_string(id) + ": " 
                            + std::to_string(time));
                }
            } else {
                int nextId = path.front();
                path.erase(path.begin());
                std::stringstream msg;
                msg << "exec " << subcommand << " " << path.size();
                for(int i : path) {
                    msg << " " << i;
                }
                SendMessage(sockets.at(nextId), msg.str());
                SendMessage(parentSocket, ReceiveMessage(sockets.at(nextId)));
            }
        } else if(command == "ping") {
            int size;
            s >> size;
            std::vector<int> path(size);
            for(int i = 0; i < size; ++i) {
                s >> path[i];
            }
            if(path.empty()) {
                SendMessage(parentSocket, "Ok: 1");
            } else {
                int nextId = path.front();
                path.erase(path.begin());
                std::stringstream msg;
                msg << "ping " << path.size();
                for(int i : path) {
                    msg << " " << i;
                }
                std::string received;
                if(!SendMessage(sockets.at(nextId), msg.str())) {
                    received = "Node is unavailable";
                } else {
                    received = ReceiveMessage(sockets.at(nextId));
                }
                SendMessage(parentSocket, received);
            }
        } else if(command == "kill") {
            for(auto& item : sockets) {
                SendMessage(item.second, "kill");
                ReceiveMessage(item.second);
                kill(pids[item.first], SIGTERM);
                kill(pids[item.first], SIGKILL);
            }
            SendMessage(parentSocket, "Ok");
        }
        if(parentPort == 0) {
            break;
        }
    }
}