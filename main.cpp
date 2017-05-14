#include "uSockets/Berkeley.h"
#include "uSockets/Epoll.h"

#include <iostream>
#include <vector>
#include <map>

#include <arpa/inet.h>

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

static const int SUBSCRIBERS = 100;
static const int PUBLISHERS = 10;

typedef uS::Berkeley<uS::Epoll>::Socket *uSocket;

std::vector<uSocket> subscribers;

std::string sharedMessage;

uS::Berkeley<uS::Epoll> *globalC;

void newConnection();

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


            //std::cout << "Subscribe: " << (int) remainingLength << std::endl;

            // precense

            //uint16_t packetId = *(uint16_t *) &data[2];
            //std::cout << "PacketId: " << packetId << std::endl;


            uint16_t topicLength = ntohs(*(uint16_t *) &data[4]);
            //std::cout << "topicLength: " << topicLength << std::endl;
            //std::cout << "topic: " << std::string(data + 6, topicLength) << std::endl;
            //std::cout << "Requested QoS: " << (int) data[6 + topicLength] << std::endl;

            std::cout << clock() << " Subscribe - " << std::string(data + 6, topicLength) << std::endl;

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

            static int pubs;
            if (++pubs == PUBLISHERS) {

                std::cout << "Broadcsting length: " << sharedMessage.length() << " over " << subscribers.size() << std::endl;

                HttpSocket::Message message;
                message.data = (char *) sharedMessage.data();
                message.length = sharedMessage.length();
                message.callback = nullptr;

                for (uSocket s : subscribers) {
                    //s->cork(true);
                    s->sendMessage(&message, false);
                    //s->cork(false);
                }

                // reset state
                sharedMessage.clear();

                // only remove the 100 last!
                //subscribers.clear();

                subscribers.erase(subscribers.begin() + SUBSCRIBERS, subscribers.end());

                std::cout << "We now have " << subscribers.size() << " subscribers!" << std::endl;

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

void newConnection() {
    globalC->connect("localhost", 1883, [](uS::Berkeley<uS::Epoll>::Socket *socket) {
        if (!socket) {
            std::cout << "Connection failed" << std::endl;
        } else {
            socket->setDerivative(PASSIVE);
            socket->setNoDelay(true);

//            16 27 0 4 77 81 84 84 4 2 0 60 0 15 109 113 116 116 106 115 95 102 53 49 52 55 49 99 56
//            16 27 0 4 77 81 84 84 4 2 0 60 0 15 109 113 116 116 106 115 95 97 51 102 49 57 102 56 50
//            16 27 0 4 77 81 84 84 4 2 0 60 0 15 109 113 116 116 106 115 95 102 52 54 101 99 100 98 48
//            16 27 0 4 77 81 84 84 4 2 0 60 0 15 109 113 116 116 106 115 95 53 49 48 49 49 97 102 53

            unsigned char connection[] = {16, 27, 0, 4, 77, 81, 84, 84, 4, 2, 0, 60, 0, 15, 109, 113, 116, 116, 106, 115, 95,
                                          'A' + rand() % 27, 'A' + rand() % 27, 'A' + rand() % 27, 'A' + rand() % 27, 'A' + rand() % 27, 'A' + rand() % 27, 'A' + rand() % 27, 'A' + rand() % 27};
            HttpSocket::Message message;
            message.data = (char *) &connection;
            message.length = 29;
            message.callback = nullptr;
            socket->sendMessage(&message, false);
        }
    }, HttpSocket::allocator);
}

std::vector<uSocket> clients;
#include <chrono>

std::chrono::high_resolution_clock::time_point startPoint = std::chrono::high_resolution_clock::now();

struct Passive {
    static void onData(uS::Berkeley<uS::Epoll>::Socket *socket, char *data, size_t length) {

        for (char *cursor = data; cursor < data + length; ) {
            unsigned char messageType = ((unsigned char) cursor[0]) >> 4;
            unsigned char remainingLength = (unsigned char) cursor[1];

            if (messageType == PUBLISH) {
                static int received = 0;
                if (++received == PUBLISHERS * SUBSCRIBERS) {
                    std::cout << "Delay: " << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - startPoint).count() << "ms" << std::endl;
                    received = 0;
                }
            } else if (messageType == CONNACK) {

                unsigned char subscribe[] = {130, 13, 144, 223, 0, 8, 112, 114, 101, 115, 101, 110, 99, 101, 0};
                HttpSocket::Message message;
                message.data = (char *) &subscribe;
                message.length = 15;
                message.callback = nullptr;
                socket->sendMessage(&message, false);
            } else if (messageType == SUBACK) {
                clients.push_back(socket);
                if (clients.size() == SUBSCRIBERS) {

                    startPoint = std::chrono::high_resolution_clock::now();
                    unsigned char publish[] = {48, 12, 0, 8, 112, 114, 101, 115, 101, 110, 99, 101, 72, 105};
                    HttpSocket::Message message;
                    message.data = (char *) &publish;
                    message.length = 14;
                    message.callback = nullptr;
                    for (int i = 0; i < PUBLISHERS; i++) {
                        clients[i]->sendMessage(&message, false);
                    }
                } else {
                    newConnection();
                }
            }

            cursor += remainingLength + 2;
        }
    }

    static void onEnd(uS::Berkeley<uS::Epoll>::Socket *socket) {

    }
};

int main() {
    uS::Epoll loop(true);
    uS::Berkeley<uS::Epoll> c(&loop);
    globalC = &c;
    c.registerSocketDerivative<HttpSocket>(HTTP_SOCKET);
    c.registerSocketDerivative<Passive>(PASSIVE);

    if (c.listen(nullptr, 1883, 0, [](uS::Berkeley<uS::Epoll>::Socket *socket) {

                 socket->setNoDelay(true);

    }, HttpSocket::allocator)) {
        std::cout << "Listening to port 1883" << std::endl;
    }


    newConnection();
    loop.run();
}
