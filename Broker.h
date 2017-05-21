#ifndef BROKER_H
#define BROKER_H

#include "uSockets/Berkeley.h"
#include "uSockets/Epoll.h"

#include <functional>
#include <string_view>

namespace uTT {

class Passive;
class Active;

class Connection : private uS::Berkeley<uS::Epoll>::Socket {
private:
    void connect(std::string_view name);

public:
    void subscribe(std::string_view topic);
    void publish(std::string_view topic, std::string_view message);
    void close();

    friend class Node;
    friend class Passive;
    friend class Active;
};

class Node : private uS::Berkeley<uS::Epoll>
{
private:
    uS::Epoll loop;
    std::function<void(Connection *)> connAckHandler;
    std::function<void(Connection *)> subAckHandler;
    std::function<void(Connection *, std::string_view, std::string_view)> publishHandler;

public:
    Node();
    void connect(std::string uri);
    void listen();
    void run();
    void close();

    // these are all client-only
    void onConnected(std::function<void(Connection *)> callback);
    void onSubscribed(std::function<void(Connection *)> callback);
    void onMessage(std::function<void(Connection *, std::string_view topic, std::string_view message)> callback);

    friend class Connection;
    friend class Passive;
};

}

#endif // BROKER_H
