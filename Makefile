default: build

build:
	g++ client.cpp -o client
	g++ server.cpp -o server
