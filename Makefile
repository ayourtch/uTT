default:
	g++ -std=c++17 -O3 -s -I. src/*.cpp uSockets/Berkeley.cpp uSockets/Epoll.cpp -o µTT
