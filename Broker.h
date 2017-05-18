#ifndef BROKER_H
#define BROKER_H

#include "uSockets/Berkeley.h"
#include "uSockets/Epoll.h"

#include <string>
#include <functional>

namespace uTT {

class Passive;

class Connection : private uS::Berkeley<uS::Epoll>::Socket {
private:
    void connect(std::string name);

public:
    void subscribe(std::string topic);
    void publish(std::string topic);
    void close();

    friend class Node;
    friend class Passive;
};

class Node : private uS::Berkeley<uS::Epoll>
{
private:
    uS::Epoll loop;
    std::function<void(Connection *)> connAckHandler;
    std::function<void(Connection *)> subAckHandler;
    std::function<void(Connection *)> publishHandler;

public:
    Node();
    void connect(std::string uri);
    void listen();
    void run();
    // these are all client-only
    void onConnected(std::function<void(Connection *)> callback);
    void onSubscribed(std::function<void(Connection *)> callback);
    void onMessage(std::function<void(Connection *)> callback);

    friend class Connection;
    friend class Passive;
};

}

#endif // BROKER_H
