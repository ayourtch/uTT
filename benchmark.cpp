#include "Broker.h"
#include <chrono>
#include <fstream>

std::ofstream log("benchmark_results");

void testBroadcastingPerformance(int subscribers, int publishers) {
    uTT::Node broker;
    std::vector<uTT::Connection *> clients, pubs;
    std::chrono::high_resolution_clock::time_point startPoint;

    int lapse = 0;
    double totalDelay = 0;
    int totalSamples = 0;

    std::cout << "Running benchmark of " << subscribers << " subscribers, " << publishers << " publishers" << std::endl;
    log << "Running benchmark of " << subscribers << " subscribers, " << publishers << " publishers" << std::endl;

    broker.onConnected([&](uTT::Connection *connection) {

        if (clients.size() == subscribers) {
            pubs.push_back(connection);
            if (pubs.size() == publishers) {
                startPoint = std::chrono::high_resolution_clock::now();
                for (int i = 0; i < publishers; i++) {
                    pubs[i]->publish("hallå/some topic", "hallåja");
                }
            } else {
                broker.connect("localhost");
            }

        } else {
            connection->subscribe("hallå/#");
        }
    });

    broker.onSubscribed([&](uTT::Connection *connection) {
        clients.push_back(connection);
        if (clients.size() == subscribers) {

            if (REDIS) {
                // now connect the separate publishers
                broker.connect("localhost");
            } else {
                startPoint = std::chrono::high_resolution_clock::now();
                for (int i = 0; i < publishers; i++) {
                    clients[i]->publish("hallå/some topic", "hallåja");
                }
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
            if (totalSamples % 10 == 0) {
                lapse = 0;
                std::cout << "\tAverage delay at sample " << totalSamples << ": " << (totalDelay / totalSamples / 1000) << "ms" << std::endl;
            }
            received = 0;
            if (totalSamples / 10 < 10) {

                if (REDIS) {
                    startPoint = std::chrono::high_resolution_clock::now();
                    for (int i = 0; i < publishers; i++) {
                        pubs[i]->publish("hallå/some topic", "hallåja");
                    }
                } else {
                    startPoint = std::chrono::high_resolution_clock::now();
                    for (int i = 0; i < publishers; i++) {
                        clients[i]->publish("hallå/some topic", "hallåja");
                    }
                }




            } else {
                log << "\tAverage delay at sample " << totalSamples << ": " << (totalDelay / totalSamples / 1000) << "ms" << std::endl;
                std::cout << std::endl;
                log << std::endl;
                for (auto *client : clients) {
                    client->close();
                }
                for (auto *client : pubs) {
                    client->close();
                }
                broker.close();
            }
        }
    });

    broker.connect("localhost");
    broker.run();
    log.flush();
}

int main() {
    srand(time(0));

    while (true) {
        std::cout << "Enter broker name: ";
        std::string brokerName;
        std::cin >> brokerName;
        if (brokerName == "redis") {
            REDIS = true;
            log << "Broker name is: " << brokerName << " (running as Redis client)" << std::endl;
        } else {
            REDIS = false;
            log << "Broker name is: " << brokerName << " (running as MQTT client)" << std::endl;
        }

        testBroadcastingPerformance(25, 1);
        testBroadcastingPerformance(25, 10);
        testBroadcastingPerformance(25, 25);
        testBroadcastingPerformance(50, 25);
        testBroadcastingPerformance(100, 25);
        testBroadcastingPerformance(500, 25);
        testBroadcastingPerformance(500, 100);
    }
}

