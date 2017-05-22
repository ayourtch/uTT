#include "Broker.h"
#include <chrono>

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

    testTopicTree();
}
