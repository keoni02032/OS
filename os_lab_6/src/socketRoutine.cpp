#include "socketRoutine.hpp"
#include <iostream>

bool SendMessage(zmq::socket_t& socket, const std::string& message) {
    zmq::message_t m(message.size());
    memcpy(m.data(), message.c_str(), message.size());
    try {
        socket.send(m);
        return true;
    } catch(...) {
        return false;
    }
}

std::string ReceiveMessage(zmq::socket_t& socket) {
    zmq::message_t message;
    bool messageReceived;
    try {
        messageReceived = socket.recv(&message);
    } catch(...) {
        messageReceived = false;
    }

    std::string received(static_cast<char*>(message.data()), message.size());
    
    if(!messageReceived || received.empty()) {
        return "Error: Node is unavailable";
    } else {
        return received;
    }
}

int BindSocket(zmq::socket_t& socket) {
    int port = 30000;
    std::string portTemplate = "tcp://127.0.0.1:";
    while(true) {
        try {
            socket.bind(portTemplate + std::to_string(port));
            break;
        } catch(...) {
            port++;
        }
    }
    return port;
}

void CreateNode(int id, int portNumber) {
    char* arg0 = strdup("./calcNode");
    char* arg1 = strdup((std::to_string(id)).c_str());
    char* arg2 = strdup((std::to_string(portNumber)).c_str());
    char* args[] = {arg0, arg1, arg2, nullptr};
    execv("./calcNode", args);
}