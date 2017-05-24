#ifndef TOPICTREE_H
#define TOPICTREE_H

#include <map>
#include <string>
#include <vector>
#include <set>

class TopicTree {
private:
    struct Node : std::map<std::string, Node *> {
        Node *get(std::string path);
        std::vector<std::pair<void *, bool *>> subscribers;
        std::string sharedMessage;
    };

    Node *root = new Node;
    std::set<Node *> pubNodes;

public:
    void subscribe(std::string topic, void *connection, bool *valid);
    void publish(std::string topic, char *data, size_t length);
    void reset();
    void drain(void (*prepareCb)(void *user, char *, size_t), void (*sendCb)(void *, void *), void *user);
};

#endif // TOPICTREE_H
