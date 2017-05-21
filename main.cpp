#include "Broker.h"
#include <chrono>

void testBroadcastingPerformance(int subscribers, int publishers, bool native) {
    uTT::Node broker;
    std::vector<uTT::Connection *> clients;
    std::chrono::high_resolution_clock::time_point startPoint;

    int lapse = 0;
    double totalDelay = 0;
    int totalSamples = 0;

    std::cout << "Running benchmark of " << subscribers << " subscribers, " << publishers << " publishers" << std::endl;

    broker.onConnected([](uTT::Connection *connection) {
        connection->subscribe("hallå/#");
    });

    broker.onSubscribed([&](uTT::Connection *connection) {
        clients.push_back(connection);
        if (clients.size() == subscribers) {
            startPoint = std::chrono::high_resolution_clock::now();
            for (int i = 0; i < publishers; i++) {
                clients[i]->publish("hallå/some topic", "hallåja");
            }
        } else {
            broker.connect("localhost");
        }
    });

    broker.onMessage([&](uTT::Connection *connection, std::string_view topic, std::string_view message) {
        static int received = 0;
        if (++received == publishers * subscribers) {
            int delay = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - startPoint).count();
            totalDelay += delay;
            totalSamples += 1;
            if (totalSamples % 100 == 0) {
                lapse = 0;
                std::cout << "\tAverage delay at sample " << totalSamples << ": " << (totalDelay / totalSamples / 1000) << "ms" << std::endl;
            }
            received = 0;
            if (totalSamples / 100 < 10) {
                startPoint = std::chrono::high_resolution_clock::now();
                for (int i = 0; i < publishers; i++) {
                    clients[i]->publish("hallå/some topic", "hallåja");
                }
            } else {
                std::cout << std::endl;
                for (auto *client : clients) {
                    client->close();
                }
                broker.close();
            }
        }
    });

    if (native) {
        broker.listen();
    }
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

    while (true) {
        std::cout << "Enter broker name: ";
        std::string brokerName;
        std::cin >> brokerName;
        bool native = brokerName == "µtt";
        testBroadcastingPerformance(25, 1, native);
        testBroadcastingPerformance(25, 10, native);
        testBroadcastingPerformance(25, 25, native);

        testBroadcastingPerformance(50, 25, native);
        testBroadcastingPerformance(100, 25, native);
        testBroadcastingPerformance(500, 25, native);
    }

    //testTopicTree();

}
