#include "TopicTree.h"

void TopicTree::subscribe(std::string topic, void *connection, bool *valid) {
    Node *curr = root;
    for (int i = 0; i < topic.length(); i++) {
        int start = i;
        while (topic[i] != '/' && i < topic.length()) {
            i++;
        }
        curr = curr->get(topic.substr(start, i - start));
    }
    curr->subscribers.push_back({connection, valid});
}

void TopicTree::publish(std::string topic, char *data, size_t length) {
    Node *curr = root;
    for (int i = 0; i < topic.length(); i++) {
        int start = i;
        while (topic[i] != '/' && i < topic.length()) {
            i++;
        }
        std::string path(topic.data() + start, i - start);

        // end wildcard consumes traversal
        auto it = curr->find("#");
        if (it != curr->end()) {
            curr = it->second;
            //matches.push_back(curr);
            curr->sharedMessage.append(data, length);
            if (curr->subscribers.size()) {
                pubNodes.insert(curr);
            }
            break;
        } else {
            it = curr->find(path);
            if (it == curr->end()) {
                it = curr->find("+");
                if (it != curr->end()) {
                    goto skip;
                }
                break;
            } else {
skip:
                curr = it->second;
                if (i == topic.length()) {
                    //matches.push_back(curr);
                    curr->sharedMessage.append(data, length);
                    if (curr->subscribers.size()) {
                        pubNodes.insert(curr);
                    }
                    break;
                }
            }
        }
    }
}

void TopicTree::reset() {
    root = new Node;
}

void TopicTree::drain(void (*prepareCb)(void *, char *, size_t), void (*sendCb)(void *, void *), void (*refCb)(void *), void *user) {
    if (pubNodes.size()) {

        //if(pubNodes.size() > 1) {
            for (Node *topicNode : pubNodes) {
                for (std::pair<void *, bool *> p : topicNode->subscribers) {
                    if (*p.second) {
                        refCb(p.first);
                    }
                }
            }
        //}

        for (Node *topicNode : pubNodes) {
            prepareCb(user, (char *) topicNode->sharedMessage.data(), topicNode->sharedMessage.length());
            for (auto it = topicNode->subscribers.begin(); it != topicNode->subscribers.end(); ) {
                if (!*it->second) {
                    it = topicNode->subscribers.erase(it);
                } else {
                    sendCb(user, it->first);
                    it++;
                }
            }

            topicNode->sharedMessage.clear();
        }
        pubNodes.clear();
    }
}

TopicTree::Node *TopicTree::Node::get(std::string path) {
    std::pair<std::map<std::string, Node *>::iterator, bool> p = insert({path, nullptr});
    if (p.second) {
        return p.first->second = new Node;
    } else {
        return p.first->second;
    }
}
