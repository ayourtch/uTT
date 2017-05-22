#include "Broker.h"

#include <iostream>
#include <vector>
#include <map>

#include <arpa/inet.h>
#include <cstring>

#include <map>
#include <string>
#include <set>

int REDIS = false;

namespace uTT {

enum States {
    ACTIVE,
    PASSIVE,
    REDIS_STATE
};

enum MQTT {
    CONNECT = 1,
    CONNACK,
    PUBLISH,
    PUBACK,
    PUBREC,
    PUBREL,
    PUBCOMP,
    SUBSCRIBE,
    SUBACK,
    UNSUBSCRIBE,
    UNSUBACK,
    PINGREQ,
    PINGRESP,
    DISCONNECT
};

//class TopicTree {

//};

struct TopicNode : std::map<std::string, TopicNode *> {
    TopicNode *get(std::string path) {
        std::pair<std::map<std::string, TopicNode *>::iterator, bool> p = insert({path, nullptr});
        if (p.second) {
            return p.first->second = new TopicNode;
        } else {
            return p.first->second;
        }
    }

    std::vector<std::pair<Connection *, bool *>> subscribers;
    std::string sharedMessage;
};

TopicNode *topicTree = new TopicNode;
std::set<TopicNode *> pubNodes;

std::vector<TopicNode *> getMatchingTopicNodes(std::string_view topic) {
    std::vector<TopicNode *> matches;

    TopicNode *curr = topicTree;

    for (int i = 0; i < topic.length(); i++) {
        int start = i;
        while (topic[i] != '/' && i < topic.length()) {
            i++;
        }
        std::string path(topic.substr(start, i - start));

        // end wildcard consumes traversal
        auto it = curr->find("#");
        if (it != curr->end()) {
            curr = it->second;
            matches.push_back(curr);
            break;
        } else {
            it = curr->find(path);
            if (it == curr->end()) {
                break;
            } else {
                curr = it->second;
                if (i == topic.length()) {
                    matches.push_back(curr);
                    break;
                }
            }
        }
    }
    return std::move(matches);
}

void subscribe(std::string topic, Connection *connection) {
    TopicNode *curr = topicTree;

    for (int i = 0; i < topic.length(); i++) {
        int start = i;
        while (topic[i] != '/' && i < topic.length()) {
            i++;
        }

        curr = curr->get(topic.substr(start, i - start));
    }

    curr->subscribers.push_back({connection, (bool *) connection->getUserData()});
}

struct Active : uS::Berkeley<uS::Epoll>::Socket {


    static void parseMqtt(uS::Berkeley<uS::Epoll>::Socket *socket, char *data, size_t length) {
        unsigned char messageType = ((unsigned char) data[0]) >> 4;
        unsigned char remainingLength = (unsigned char) data[1];


        switch (messageType) {

        // must be connect if not already connected!
        case CONNECT: {

//            for (int i = 0; i < length; i++) {
//                std::cout << (int) (unsigned char) data[i] << " ";
//            }
//            std::cout << std::endl;

            //std::cout << "Connection incomming" << std::endl;

            //std::cout << std::string(data + 4, 4) << std::endl;

            unsigned char protocolLevel = (unsigned char) data[2 + 6];

            //std::cout << "protocolLevel: " << protocolLevel << std::endl;

            if (protocolLevel != 4) {
                // 3.1.1 = 4
                std::cout << "Unsupported version" << std::endl;
            }

            //unsigned char connectFlags = (unsigned char) data[2 + 7];

            struct ConnectFlags {
                unsigned int cleanSession : 1;
                unsigned int willFlag : 1;
                unsigned int willQoS : 2;
                unsigned int willRetain : 1;
                unsigned int passwordFlag : 1;
                unsigned int userNameFlag : 1;
                unsigned int keepAlive : 1;
            } __attribute__((packed)) *connectionFlags = (ConnectFlags *) &data[2 + 7];

            //std::cout << "Sizeof: " << sizeof(ConnectFlags) << std::endl;

//            std::cout << "CleanSession: " << connectionFlags->cleanSession << std::endl;
//            std::cout << "WillFlag: " << connectionFlags->willFlag << std::endl;
//            std::cout << "WillQoS: " << connectionFlags->willQoS << std::endl;
//            std::cout << "WillRetain: " << connectionFlags->willRetain << std::endl;
//            std::cout << "PasswordFlag: " << connectionFlags->passwordFlag << std::endl;
//            std::cout << "UserNameFlag: " << connectionFlags->userNameFlag << std::endl;
//            std::cout << "KeepAlive: " << connectionFlags->keepAlive << std::endl;


            unsigned char *payload = (unsigned char *) &data[2 + 11];

            // client identifier
            unsigned char clientIdLength = (unsigned char) payload[0];

            //std::cout << "length: " << (int) clientIdLength << std::endl;

            //std::cout << "New connection - " << std::string((char *) payload + 1, clientIdLength) << std::endl;

            //willtopic

            //willmessage

            //username

            //password

            // send connack

            unsigned char connAck[4] = {CONNACK << 4, 2, 0, 0};

            Active::Message message;
            message.data = (char *) &connAck;
            message.length = 4;
            message.callback = nullptr;
            socket->sendMessage(&message, false);


            //std::cout << "Remaining length: " << (int) remainingLength << std::endl;
            break;
        }
        case SUBSCRIBE: {

            uint16_t topicLength = ntohs(*(uint16_t *) &data[4]);
            std::string topic(data + 6, topicLength);

            subscribe(topic, static_cast<Connection *>(socket));

            // send suback
            unsigned char subAck[5] = {SUBACK << 4, 3, data[2], data[3], 0};
            Active::Message message;
            message.data = (char *) &subAck;
            message.length = 5;
            message.callback = nullptr;
            socket->sendMessage(&message, false);
            break;
        }
        case PUBLISH: {
            // very basic tree here
            uint16_t topicLength = ntohs(*(uint16_t *) &data[2]);
            std::string topic(data + 4, topicLength);

            for (TopicNode *topicNode : getMatchingTopicNodes(topic)) {
                topicNode->sharedMessage.append(data, length);
                if (topicNode->subscribers.size()) {
                    pubNodes.insert(topicNode);
                }
            }
            break;
        }
        }
    }

    Active(uS::Berkeley<uS::Epoll> *context) : uS::Berkeley<uS::Epoll>::Socket(context) {
        setDerivative(ACTIVE);

        bool *valid = new bool(true);
        setUserData(valid);
    }

    static void onData(uS::Berkeley<uS::Epoll>::Socket *socket, char *data, size_t length) {

        for (char *cursor = data; cursor < data + length; ) {
            unsigned char messageType = ((unsigned char) cursor[0]) >> 4;
            unsigned char remainingLength = (unsigned char) cursor[1];

            parseMqtt(socket, cursor, remainingLength + 2);

            cursor += remainingLength + 2;
        }
    }

    static void onEnd(uS::Berkeley<uS::Epoll>::Socket *socket) {
        bool *valid = (bool *) socket->getUserData();
        *valid = false;
        socket->close([](uS::Berkeley<uS::Epoll>::Socket *socket) {
            delete (Active *) socket;
        });
    }

    static uS::Berkeley<uS::Epoll>::Socket *allocator(uS::Berkeley<uS::Epoll> *context) {
        return new Active(context);
    }
};

struct Passive {
    static void onData(uS::Berkeley<uS::Epoll>::Socket *socket, char *data, size_t length) {

        for (char *cursor = data; cursor < data + length; ) {
            unsigned char messageType = ((unsigned char) cursor[0]) >> 4;
            unsigned char remainingLength = (unsigned char) cursor[1];

            //switch (messageType)

            if (messageType == PUBLISH) {
                uint16_t topicLength = ntohs(*(uint16_t *) &cursor[2]);
                static_cast<Node *>(socket->getContext())->publishHandler(static_cast<Connection *>(socket),
                                                                          std::string_view(cursor + 4, topicLength),
                                                                          std::string_view(cursor + 4 + topicLength, remainingLength - topicLength - 2));
            } else if (messageType == CONNACK) {

                static_cast<Node *>(socket->getContext())->connAckHandler(static_cast<Connection *>(socket));

            } else if (messageType == SUBACK) {

                static_cast<Node *>(socket->getContext())->subAckHandler(static_cast<Connection *>(socket));
            }

            cursor += remainingLength + 2;
        }
    }

    static void onEnd(uS::Berkeley<uS::Epoll>::Socket *socket) {

    }
};

struct Redis {
    static void onData(uS::Berkeley<uS::Epoll>::Socket *socket, char *data, size_t length) {

        if (data[0] == '*') {
            // "*3\r\n$9\r\nsubscribe\r\n$9\r\neventName\r\n:1\r\n"
            if (length == 38) {
                static_cast<Node *>(socket->getContext())->subAckHandler(static_cast<Connection *>(socket));
            } else {
                // publishes
                if (length % 39 == 0) {
                    for (int i = 0, n = length / 39; i < n; i++) {
                        static_cast<Node *>(socket->getContext())->publishHandler(static_cast<Connection *>(socket),
                                                                                  std::string_view(nullptr, 0),
                                                                                  std::string_view(nullptr, 0));
                    }
                }
            }
        } else {
            //unknown data
        }
    }

    static void onEnd(uS::Berkeley<uS::Epoll>::Socket *socket) {
        std::cout << "Redis on end!" << std::endl;
    }
};

Node::Node() : loop(true), uS::Berkeley<uS::Epoll>(&loop) {
    registerSocketDerivative<Active>(ACTIVE);
    registerSocketDerivative<Passive>(PASSIVE);
    registerSocketDerivative<Redis>(REDIS_STATE);

    loop.postCb = [](void *) {
        // if tree has new publishes
        if (pubNodes.size()) {

            for (TopicNode *topicNode : pubNodes) {

                //std::cout << "Broadcsting length: " << topicNode->sharedMessage.length() << " over " << topicNode->subscribers.size() << std::endl;

                Active::Message message;
                message.data = (char *) topicNode->sharedMessage.data();
                message.length = topicNode->sharedMessage.length();
                message.callback = nullptr;

                for (auto it = topicNode->subscribers.begin(); it != topicNode->subscribers.end(); ) {
                    if (!*it->second) {
                        it = topicNode->subscribers.erase(it);
                    } else {
                        it->first->sendMessage(&message, false);
                        it++;
                    }
                }

                topicNode->sharedMessage.clear();
            }

            // reset state
            pubNodes.clear();
        }
    };
}

void Node::connect(std::string uri) {
    uS::Berkeley<uS::Epoll>::connect("127.0.0.1", REDIS ? 6379 : 1883, [](uS::Berkeley<uS::Epoll>::Socket *socket) {
        if (!socket) {
            std::cout << "Connection failed" << std::endl;
        } else {
            socket->setDerivative(PASSIVE);
            socket->setNoDelay(true);
            if (!REDIS) {
                std::string randomId;
                for (int i = 0; i < 10; i++) {
                    randomId += 'A' + rand() % 27;
                }
                static_cast<Connection *>(socket)->connect(randomId);
            } else {
                socket->setDerivative(REDIS_STATE);
                static_cast<Node *>(socket->getContext())->connAckHandler(static_cast<Connection *>(socket));
            }
        }
    }, Active::allocator);
}

void Node::listen() {
    if (uS::Berkeley<uS::Epoll>::listen(nullptr, 1883, 0, [](uS::Berkeley<uS::Epoll>::Socket *socket) {

                 socket->setNoDelay(true);

    }, Active::allocator)) {
        std::cout << "Listening to port 1883" << std::endl;
    } else {
        std::terminate();
    }
}

void Node::run() {
    loop.run();
}

void Node::close() {
    this->stopListening();

    topicTree = new TopicNode;
}

void Node::onConnected(std::function<void(Connection *)> callback) {
    connAckHandler = callback;
}

void Node::onSubscribed(std::function<void(Connection *)> callback) {
    subAckHandler = callback;
}

void Node::onMessage(std::function<void(Connection *, std::string_view topic, std::string_view message)> callback) {
    publishHandler = callback;
}

// private helper
void Connection::connect(std::string_view name) {

    unsigned char *connect = new unsigned char[14 + name.length()];

    connect[0] = 16;
    connect[1] = 12 + name.length(); // payload length
    connect[2] = 0; //version
    connect[3] = 4; // version
    connect[4] = 'M';
    connect[5] = 'Q';
    connect[6] = 'T';
    connect[7] = 'T';

    connect[8] = 4;
    connect[9] = 2;
    connect[10] = 0;
    connect[11] = 60;

    uint16_t nameLength = htons(name.length());
    memcpy(connect + 12, &nameLength, 2);

    memcpy(connect + 14, name.data(), name.length());

    //            16 27 0 4 77 81 84 84 4 2 0 60 0 15 109 113 116 116 106 115 95 102 53 49 52 55 49 99 56
    //            16 27 0 4 77 81 84 84 4 2 0 60 0 15 109 113 116 116 106 115 95 97 51 102 49 57 102 56 50
    //            16 27 0 4 77 81 84 84 4 2 0 60 0 15 109 113 116 116 106 115 95 102 52 54 101 99 100 98 48
    //            16 27 0 4 77 81 84 84 4 2 0 60 0 15 109 113 116 116 106 115 95 53 49 48 49 49 97 102 53

//    std::cout << (12 + name.length()) << std::endl;

//    unsigned char control[] = {16, 27, 0, 4, 77, 81, 84, 84, 4, 2, 0, 60, 0, 15, 109, 113, 116, 116, 106, 115, 95,
//                                  'A' + rand() % 27, 'A' + rand() % 27, 'A' + rand() % 27, 'A' + rand() % 27, 'A' + rand() % 27, 'A' + rand() % 27, 'A' + rand() % 27, 'A' + rand() % 27};

//    for (int i = 0; i < 29; i++) {
//        std::cout << (int) control[i] << " == " << (int) connect[i] << std::endl;
//    }
//    std::terminate();

    Active::Message message;
    message.data = (char *) connect;
    message.length = 14 + name.length();
    message.callback = nullptr;
    sendMessage(&message, false);
}

void Connection::subscribe(std::string_view topic) {

    if (REDIS) {

        unsigned char subscribe[] = "*2\r\n$9\r\nSUBSCRIBE\r\n$9\r\neventName\r\n";

        Active::Message message;
        message.data = (char *) subscribe;
        message.length = sizeof(subscribe) - 1;
        message.callback = nullptr;
        sendMessage(&message, false);

    } else {
        unsigned char payloadLength = 2 + 2 + topic.length() + 1;

        unsigned char *subscribe = new unsigned char[2 + payloadLength];
        subscribe[0] = 130;
        subscribe[1] = payloadLength;

        uint16_t packetId = 1234;
        memcpy(subscribe + 2, &packetId, 2);

        uint16_t topicLength = htons(topic.length());
        memcpy(subscribe + 4, &topicLength, 2);

        memcpy(subscribe + 6, topic.data(), topic.length());

        subscribe[payloadLength + 1] = 0; // qos

        Active::Message message;
        message.data = (char *) subscribe;
        message.length = payloadLength + 2;
        message.callback = nullptr;
        sendMessage(&message, false);
    }
}

void Connection::close() {
    uS::Berkeley<uS::Epoll>::Socket::close([](uS::Berkeley<uS::Epoll>::Socket *socket) {
        delete static_cast<Connection *>(socket);
    });
}

void Connection::publish(std::string_view topic, std::string_view data) {

    if (REDIS) {
        unsigned char publish[] = "*3\r\n$7\r\nPUBLISH\r\n$9\r\neventName\r\n$1\r\na\r\n";

        Active::Message message;
        message.data = (char *) publish;
        message.length = sizeof(publish) - 1;
        message.callback = nullptr;
        sendMessage(&message, false);

    } else {
        unsigned char payloadLength = 2 + topic.length() + data.length();

        unsigned char *publish = new unsigned char[2 + payloadLength];
        publish[0] = 48;
        publish[1] = payloadLength;

        uint16_t topicLength = htons(topic.length());
        memcpy(publish + 2, &topicLength, 2);

        memcpy(publish + 4, topic.data(), topic.length());

        memcpy(publish + 4 + topic.length(), data.data(), data.length());

        Active::Message message;
        message.data = (char *) publish;
        message.length = payloadLength + 2;
        message.callback = nullptr;
        sendMessage(&message, false);
    }
}

}
