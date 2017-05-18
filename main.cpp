#include "Broker.h"
#include <chrono>

static const int SUBSCRIBERS = 100;
static const int PUBLISHERS = 10;
std::chrono::high_resolution_clock::time_point startPoint;

int main() {

    srand(time(0));

    uTT::Node broker;
    std::vector<uTT::Connection *> clients;

    broker.onConnected([](uTT::Connection *connection) {
        connection->subscribe("some topic");
    });

    broker.onSubscribed([&clients, &broker](uTT::Connection *connection) {
        clients.push_back(connection);
        if (clients.size() == SUBSCRIBERS) {
            startPoint = std::chrono::high_resolution_clock::now();
            for (int i = 0; i < PUBLISHERS; i++) {
                clients[i]->publish("some topic");
            }
        } else {
            broker.connect("localhost");
        }
    });

    broker.onMessage([](uTT::Connection *connection) {
        static int received = 0;
        if (++received == PUBLISHERS * SUBSCRIBERS) {
            std::cout << "Delay: " << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - startPoint).count() << "ms" << std::endl;
            received = 0;
        }
    });

    //broker.listen();
    broker.connect("localhost");
    broker.run();
}
