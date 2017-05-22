default:
	g++ -std=c++17 -O3 -s standalone.cpp Broker.cpp uSockets/Berkeley.cpp uSockets/Epoll.cpp -o µTT
	g++ -std=c++17 -O3 -s benchmark.cpp Broker.cpp uSockets/Berkeley.cpp uSockets/Epoll.cpp -o benchmark
