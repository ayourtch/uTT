#include "Broker.h"

#include <iostream>
#include <vector>
#include <map>

#include <arpa/inet.h>
#include <cstring>

namespace uTT {

enum States {
    HTTP_SOCKET,
    PASSIVE
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

std::vector<uS::Berkeley<uS::Epoll>::Socket *> subscribers;

std::string sharedMessage;

struct HttpSocket : uS::Berkeley<uS::Epoll>::Socket {


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

            HttpSocket::Message message;
            message.data = (char *) &connAck;
            message.length = 4;
            message.callback = nullptr;
            socket->sendMessage(&message, false);


            //std::cout << "Remaining length: " << (int) remainingLength << std::endl;
            break;
        }
        case SUBSCRIBE: {


            //unsigned char subscribe[] = {SUBSCRIBE << 4, payloadLength, packetId, packetId, topicLength, topicLength, 112, 114, 101, 115, 101, 110, 99, 101, qos};

            //std::cout << "Subscribe: " << (int) remainingLength << std::endl;

            // precense

            //uint16_t packetId = *(uint16_t *) &data[2];
            //std::cout << "PacketId: " << packetId << std::endl;


            uint16_t topicLength = ntohs(*(uint16_t *) &data[4]);
            //std::cout << "topicLength: " << topicLength << std::endl;
            //std::cout << "topic: " << std::string(data + 6, topicLength) << std::endl;
            //std::cout << "Requested QoS: " << (int) data[6 + topicLength] << std::endl;

            //std::cout << clock() << " Subscribe - " << std::string(data + 6, topicLength) << std::endl;

            //defaultRoom.addSubscriber(socket);

            subscribers.push_back(socket);

            // send suback
            unsigned char subAck[5] = {SUBACK << 4, 3, data[2], data[3], 0};
            HttpSocket::Message message;
            message.data = (char *) &subAck;
            message.length = 5;
            message.callback = nullptr;
            socket->sendMessage(&message, false);

            break;
        }
        case PUBLISH: {

            //std::cout << "Server got pub message!" << std::endl;

//                        for (int i = 0; i < length; i++) {
//                            std::cout << (int) (unsigned char) data[i] << " ";
//                        }
//                        std::cout << std::endl;

            uint16_t topicLength = ntohs(*(uint16_t *) &data[2]);
//            std::cout << "topicLength: " << topicLength << std::endl;
//            std::cout << "topic: " << std::string(data + 4, topicLength) << std::endl;

//            uint16_t packetId = *(uint16_t *) &data[4 + topicLength];
//            std::cout << "PacketId: " << packetId << std::endl;

            //std::cout << "Publish - " << std::string(data + 4, topicLength) << " - " << std::string(data + 4 + topicLength, remainingLength - topicLength - 2) << std::endl;

            sharedMessage.append(data, length);

            // assumes 10 pubs
            static int pubs;
            if (++pubs == 10) {

                std::cout << "Broadcsting length: " << sharedMessage.length() << " over " << subscribers.size() << std::endl;

                HttpSocket::Message message;
                message.data = (char *) sharedMessage.data();
                message.length = sharedMessage.length();
                message.callback = nullptr;

                for (uS::Berkeley<uS::Epoll>::Socket *s : subscribers) {
                    //s->cork(true);
                    s->sendMessage(&message, false);
                    //s->cork(false);
                }

                // reset state
                sharedMessage.clear();

                pubs = 0;
            }

            break;
        }
        }
    }

    HttpSocket(uS::Berkeley<uS::Epoll> *context) : uS::Berkeley<uS::Epoll>::Socket(context) {
        setDerivative(HTTP_SOCKET);
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
        socket->close([](uS::Berkeley<uS::Epoll>::Socket *socket) {
            delete (HttpSocket *) socket;
        });
    }

    static uS::Berkeley<uS::Epoll>::Socket *allocator(uS::Berkeley<uS::Epoll> *context) {
        return new HttpSocket(context);
    }
};

struct Passive {
    static void onData(uS::Berkeley<uS::Epoll>::Socket *socket, char *data, size_t length) {

        for (char *cursor = data; cursor < data + length; ) {
            unsigned char messageType = ((unsigned char) cursor[0]) >> 4;
            unsigned char remainingLength = (unsigned char) cursor[1];

            //switch (messageType)

            if (messageType == PUBLISH) {

                static_cast<Node *>(socket->getContext())->publishHandler(static_cast<Connection *>(socket));

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

Node::Node() : loop(true), uS::Berkeley<uS::Epoll>(&loop) {
    registerSocketDerivative<HttpSocket>(HTTP_SOCKET);
    registerSocketDerivative<Passive>(PASSIVE);
}

void Node::connect(std::string uri) {
    uS::Berkeley<uS::Epoll>::connect("localhost", 1883, [](uS::Berkeley<uS::Epoll>::Socket *socket) {
        if (!socket) {
            std::cout << "Connection failed" << std::endl;
        } else {
            socket->setDerivative(PASSIVE);
            socket->setNoDelay(true);
            std::string randomId;
            for (int i = 0; i < 10; i++) {
                randomId += 'A' + rand() % 27;
            }
            static_cast<Connection *>(socket)->connect(randomId);
        }
    }, HttpSocket::allocator);
}

void Node::listen() {
    if (uS::Berkeley<uS::Epoll>::listen(nullptr, 1883, 0, [](uS::Berkeley<uS::Epoll>::Socket *socket) {

                 socket->setNoDelay(true);

    }, HttpSocket::allocator)) {
        std::cout << "Listening to port 1883" << std::endl;
    }
}

void Node::run() {
    loop.run();
}

void Node::onConnected(std::function<void(Connection *)> callback) {
    connAckHandler = callback;
}

void Node::onSubscribed(std::function<void(Connection *)> callback) {
    subAckHandler = callback;
}

void Node::onMessage(std::function<void(Connection *)> callback) {
    publishHandler = callback;
}

// private helper
void Connection::connect(std::string name) {

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

    HttpSocket::Message message;
    message.data = (char *) connect;
    message.length = 14 + name.length();
    message.callback = nullptr;
    sendMessage(&message, false);
}

void Connection::subscribe(std::string topic) {
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

    HttpSocket::Message message;
    message.data = (char *) subscribe;
    message.length = payloadLength + 2;
    message.callback = nullptr;
    sendMessage(&message, false);
}

void Connection::publish(std::string topic) {
    unsigned char payloadLength = 2 + topic.length() + 2;

    unsigned char *publish = new unsigned char[2 + payloadLength];
    publish[0] = 48;
    publish[1] = payloadLength;

    uint16_t topicLength = htons(topic.length());
    memcpy(publish + 2, &topicLength, 2);

    memcpy(publish + 4, topic.data(), topic.length());

    uint16_t packetId = 1234;
    memcpy(publish + 4 + topic.length(), &packetId, 2);

    HttpSocket::Message message;
    message.data = (char *) publish;
    message.length = payloadLength + 2;
    message.callback = nullptr;
    sendMessage(&message, false);
}

}
