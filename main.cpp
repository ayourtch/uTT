#include "Broker.h"
#include <chrono>

void testBroadcastingPerformance() {
    uTT::Node broker;
    std::vector<uTT::Connection *> clients;
    std::chrono::high_resolution_clock::time_point startPoint;

    static const int SUBSCRIBERS = 500;
    static const int PUBLISHERS = 100;

    broker.onConnected([](uTT::Connection *connection) {
        connection->subscribe("hallå/#");
    });

    broker.onSubscribed([&](uTT::Connection *connection) {
        clients.push_back(connection);
        if (clients.size() == SUBSCRIBERS) {
            startPoint = std::chrono::high_resolution_clock::now();
            for (int i = 0; i < PUBLISHERS; i++) {
                clients[i]->publish("hallå/some topic", "hallåja");
            }
        } else {
            broker.connect("localhost");
        }
    });

    broker.onMessage([&](uTT::Connection *connection, std::string_view topic, std::string_view message) {
        static int received = 0;
        if (++received == PUBLISHERS * SUBSCRIBERS) {
            std::cout << "Delay: " << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - startPoint).count() << "ms" << std::endl;
            received = 0;
        }
    });

    broker.listen();
    broker.connect("localhost");
    broker.run();
}

void testTopicTree() {
    uTT::Node broker;
    int subs = 0;

    broker.onConnected([](uTT::Connection *connection) {
        connection->subscribe("fruit/apple");
        connection->subscribe("fruit/orange/yolo");
        connection->subscribe("fruit/#");
    });

    broker.onSubscribed([&subs](uTT::Connection *connection) {
        if (++subs == 3) {
            connection->publish("fruit/appleä", "hallåja");
            connection->publish("fruit/orange/r", "HALLÅÅÅÅJA");
        }
    });

    broker.onMessage([](uTT::Connection *connection, std::string_view topic, std::string_view message) {
        std::cout << "Topic: " << topic << ", message: " << message << std::endl;
    });

    broker.listen();
    broker.connect("localhost");
    broker.run();
}

int main() {
    srand(time(0));

    testBroadcastingPerformance();
    //testTopicTree();

}
