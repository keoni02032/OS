#include <iostream>
#include <chrono>
#include <string>
#include <zmq.hpp>
#include <vector>
#include <csignal>
#include <sstream>
#include <memory>
#include <unordered_map>

#include "socketRoutine.hpp"

struct TreeNode {    
    TreeNode(int id, std::weak_ptr<TreeNode> parent) : id(id), parent(parent) {};
    int id;
    std::weak_ptr<TreeNode> parent;
    std::unordered_map<int, std::shared_ptr<TreeNode>> children;
};

class NTree {
public:
    bool Insert(int nodeId, int parentId) {
        if(root == nullptr) {
            root = std::make_shared<TreeNode>(nodeId, std::weak_ptr<TreeNode>());
            return true;
        }
        std::vector<int> pathToNode = PathTo(parentId);
        if(pathToNode.empty()) {
            return false;
        }
        pathToNode.erase(pathToNode.begin());
        std::shared_ptr<TreeNode> temp = root;
        for(const auto& node : pathToNode) {
            temp = temp->children[node];
        }
        temp->children[nodeId] = std::make_shared<TreeNode>(nodeId, temp);
        return true;
    }

    bool Remove(int nodeId) {
        std::vector<int> pathToNode = PathTo(nodeId);
        if(pathToNode.empty()) {
            return false;
        }
        pathToNode.erase(pathToNode.begin());
        std::shared_ptr<TreeNode> temp = root;
        for(const auto& node : pathToNode) {
            temp = temp->children[node];
        }
        if(temp->parent.lock()) {
            temp = temp->parent.lock();
            temp->children.erase(nodeId);
        } else {
            root = nullptr;
        }
        return true;
    }
    std::vector<int> PathTo(int id) const {
        std::vector<int> path;
        if(!findNode(root, id, path)) {
            return {};
        } else {
            return path;
        }
    }
private:
    bool findNode(const std::shared_ptr<TreeNode>& current, int id, std::vector<int>& path) const {
        if(!current) {
            return false;
        }
        if(current->id == id) {
            path.push_back(current->id);
            return true;
        }
        path.push_back(current->id);
        for(const auto& node : current->children) {
            if(findNode(node.second, id, path)) {
                return true;
            }
        }
        path.pop_back();
        return false;
    }
    std::shared_ptr<TreeNode> root = nullptr;
};

int main() {
    NTree calcs;
    std::string action;
    int childPid = 0;
    int childId = 0;
    zmq::context_t ctx(1);
    zmq::socket_t handlerSocket(ctx, ZMQ_REQ);
    handlerSocket.setsockopt(ZMQ_SNDTIMEO, 5000);
    handlerSocket.setsockopt(ZMQ_LINGER, 5000);
    handlerSocket.setsockopt(ZMQ_RCVTIMEO, 5000);
    handlerSocket.setsockopt(ZMQ_REQ_CORRELATE, 1);
    handlerSocket.setsockopt(ZMQ_REQ_RELAXED, 1);
    int portNumber = BindSocket(handlerSocket);
    std::cout << portNumber << std::endl;
    while(true) {
        std::cin >> action;
        if(action == "create") {
            int nodeId, parentId;
            std::string result;
            std::cin >> nodeId >> parentId;
            if(!childPid) {
                childPid = fork();
                if(childPid == -1) {
                    std::cout << "Unable to create process" << std::endl;
                    exit(-1);
                } else if(childPid == 0) {
                    CreateNode(nodeId, portNumber);
                } else {
                    parentId = 0;
                    childId = nodeId;
                    SendMessage(handlerSocket, "pid");
                    result = ReceiveMessage(handlerSocket);
                }
            } else {
                if(!calcs.PathTo(nodeId).empty()) {
                    std::cout << "Error: Already exists" << std::endl;
                    continue;
                }
                std::vector<int> path = calcs.PathTo(parentId);
                if(path.empty()) {
                    std::cout << "Error: Parent not found" << std::endl;
                    continue;
                }
                path.erase(path.begin());
                std::stringstream s;
                s << "create " << path.size();
                for(int id : path) {
                    s << " " << id;
                }
                s << " " << nodeId;
                SendMessage(handlerSocket, s.str());
                result = ReceiveMessage(handlerSocket);
            }

            if(result.substr(0, 2) == "Ok") {
                calcs.Insert(nodeId, parentId);                
            }
            std::cout << result << std::endl;
        } else if(action == "remove") {
            if(childPid == 0) {
                std::cout << "Error: Not found" << std::endl;
                continue;
            }
            int nodeId;
            std::cin >> nodeId;
            if(nodeId == childId) {
                SendMessage(handlerSocket, "kill");
                ReceiveMessage(handlerSocket);
                kill(childPid, SIGTERM);
                kill(childPid, SIGKILL);
                childId = 0;
                childPid = 0;
                std::cout << "Ok" << std::endl;
                calcs.Remove(nodeId);
                continue;
            }
            std::vector<int> path = calcs.PathTo(nodeId);
            if(path.empty()) {
                std::cout << "Error: Not found" << std::endl;
                continue;
            }
            path.erase(path.begin());
            std::stringstream s;
            s << "remove " << path.size() - 1;
            for(int i : path) {
                s << " " << i;
            }
            SendMessage(handlerSocket, s.str());
            std::string recieved = ReceiveMessage(handlerSocket);
            if(recieved.substr(0, 2) == "Ok") {
                calcs.Remove(nodeId);
            }
            std::cout << recieved << std::endl;
        } else if(action == "exec") {
            int nodeId;
            std::string subcommand;
            std::cin >> nodeId >> subcommand;
            std::vector<int> path = calcs.PathTo(nodeId);
            if(path.empty()) {
                std::cout << "Error: Not found" << std::endl;
                continue;
            }
            path.erase(path.begin());
            std::stringstream s;
            s << "exec " << subcommand << " " << path.size();
            for(int i : path) {
                s << " " << i;
            }
            SendMessage(handlerSocket, s.str());
            std::string received = ReceiveMessage(handlerSocket);
            std::cout << received << std::endl;
        } else if(action == "ping") {
            if(childPid == 0) {
                std::cout << "Error: Not found" << std::endl;
                continue;
            }
            int nodeId;
            std::cin >> nodeId;
            std::vector<int> path = calcs.PathTo(nodeId);
            if(path.empty()) {
                std::cout << "Error: Not found" << std::endl;
                continue;
            }
            path.erase(path.begin());
            std::stringstream s;
            s << "ping " << path.size();
            for(int i : path) {
                s << " " << i;
            }
            std::string received;
            if(!SendMessage(handlerSocket, s.str())) {
                received = "Node is unavailable";
            } else {
                received = ReceiveMessage(handlerSocket);
            }
            std::cout << received << std::endl;
        } else if(action == "exit") {
            SendMessage(handlerSocket, "kill");            
            ReceiveMessage(handlerSocket);
            kill(childPid, SIGTERM);
            kill(childPid, SIGKILL);
            break;
        } else {
            std::cout << "Unknown command" << std::endl;
        }
        action.clear();
    }
    return 0;
}