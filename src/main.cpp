#include "Broker.h"
#include <iostream>

int main() {
    std::cout << "µTT - v0.0.0" << std::endl;

    uTT::Node broker;
    broker.listen();
    broker.run();
}
