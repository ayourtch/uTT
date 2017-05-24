#include "Broker.h"
#include "TopicTree.h"
#include "MQTT.h"

#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <set>

// static
int REDIS = false;
TopicTree topicTree;

namespace uTT {

enum States {
    ACTIVE,
    PASSIVE,
    REDIS_STATE
};

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

            topicTree.subscribe(topic, static_cast<Connection *>(socket), (bool *) static_cast<Connection *>(socket)->getUserData());

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

            topicTree.publish(topic, data, length);
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
        Active::Message message;
        topicTree.drain([](void *user, char *data, size_t length) {
            Active::Message *messagePtr = (Active::Message *) user;
            messagePtr->data = data;
            messagePtr->length = length;
            messagePtr->callback = nullptr;
        }, [](void *user, void *connection) {
            static_cast<Connection *>(connection)->sendMessage(static_cast<Active::Message *>(user), false);
        }, &message);
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

    // is this really needed? drain?
    topicTree.reset();
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

void Connection::close() {
    uS::Berkeley<uS::Epoll>::Socket::close([](uS::Berkeley<uS::Epoll>::Socket *socket) {
        delete static_cast<Connection *>(socket);
    });
}

// private helper
void Connection::connect(std::string_view name) {
    size_t length = getConnectLength(name);
    unsigned char *connect = new unsigned char[length];
    formatConnect(connect, name);

    Active::Message message;
    message.data = (char *) connect;
    message.length = length;
    message.callback = nullptr;
    sendMessage(&message, false);
}

void Connection::subscribe(std::string_view topic) {
    if (!REDIS) {
        size_t length = getSubscribeLength(topic);
        unsigned char *subscribe = new unsigned char[length];
        formatSubscribe(subscribe, topic);

        Active::Message message;
        message.data = (char *) subscribe;
        message.length = length;
        message.callback = nullptr;
        sendMessage(&message, false);
    } else {      
        unsigned char subscribe[] = "*2\r\n$9\r\nSUBSCRIBE\r\n$9\r\neventName\r\n";

        Active::Message message;
        message.data = (char *) subscribe;
        message.length = sizeof(subscribe) - 1;
        message.callback = nullptr;
        sendMessage(&message, false);
    }
}

void Connection::publish(std::string_view topic, std::string_view data) {
    if (!REDIS) {
        size_t length = getPublishLength(topic, data);
        unsigned char *publish = new unsigned char[length];
        formatPublish(publish, topic, data);

        Active::Message message;
        message.data = (char *) publish;
        message.length = length;
        message.callback = nullptr;
        sendMessage(&message, false);
     } else {
        unsigned char publish[] = "*3\r\n$7\r\nPUBLISH\r\n$9\r\neventName\r\n$1\r\na\r\n";

        Active::Message message;
        message.data = (char *) publish;
        message.length = sizeof(publish) - 1;
        message.callback = nullptr;
        sendMessage(&message, false); 
    }
}

}
