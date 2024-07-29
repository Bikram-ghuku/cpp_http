#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <thread>
#include <sstream>
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

int serve(int client, std::string dirL){

	std::string client_message(1024, '\0');

	ssize_t brecvd = recv(client, (void *)&client_message[0], client_message.max_size(), 0);

	std::string message;
	if(client_message.starts_with("GET /echo/")){
		int i;
		std::string x;

		for(i = 0; client_message[i+10] != ' '; i++){
			x += client_message[i+10];
		}

		message = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: "+std::to_string(x.length())+"\r\n\r\n"+x;
	}else if(client_message.starts_with("GET /files/")){

		int i = client_message.find("/files/");
		std::string x;
		for(int j = 7; client_message[i + j] != ' '; j++){
			x += client_message[i + j];
		}

		std::string filename = dirL + x;
		std::ifstream ifs(filename);
		if(ifs.good()){
			std::stringstream content;
        	content << ifs.rdbuf();
			message = "HTTP/1.1 200 OK\r\nContent-Type: application/octet-stream\r\nContent-Length: " + std::to_string(content.str().length()) + "\r\n\r\n" + content.str() + "\r\n";
		}else{
			message = "HTTP/1.1 404 Not Found\r\n\r\n";
		}

	}else if(client_message.starts_with("POST /files/")){

		int i = client_message.find("/files/");
		std::string x;
		for(int j = 7; client_message[i + j] != ' '; j++){
			x += client_message[i + j];
		}

		std::string filename = dirL + x;
		std::ofstream ofs(filename);

		i = client_message.find("\r\n\r\n");
		std::string con = "";

		for(int j = 4; client_message[i + j] != '\0' || i + j < client_message.size(); j++){
			con += client_message[i + j];
		}

		if (!ofs) {
			return 1;
		}
		
		ofs << con;
		ofs.close();

		message = "HTTP/1.1 201 Created\r\n\r\n";
		
	}else if(client_message.starts_with("GET /user-agent")){

		int i = client_message.find("User-Agent: ");
		std::string x;
		for(int j = 12; client_message[i + j] != '\r'; j++){
			x += client_message[i + j];
		}

		message = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: "+std::to_string(x.length())+"\r\n\r\n"+x;

	}else if(client_message.starts_with("GET / HTTP/1.1\r\n")){

		message =  "HTTP/1.1 200 OK\r\n\r\n";

	}else{

		message = "HTTP/1.1 404 Not Found\r\n\r\n";
	}

	send(client, message.c_str(), message.length(), 0);
	std::cout << "Client connected\n";


	
	return 0;
}

int main(int argc, char **argv) {

	std::cout << std::unitbuf;
	std::cerr << std::unitbuf;
	
	std::cout << "Logs from your program will appear here!\n";

	
	int server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd < 0) {
		std::cerr << "Failed to create server socket\n";
		return 1;
	}

	int reuse = 1;
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
		std::cerr << "setsockopt failed\n";
		return 1;
	}

	struct sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(4221);

	if (bind(server_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) != 0) {
		std::cerr << "Failed to bind to port 4221\n";
		return 1;
	}

	int connection_backlog = 5;
	if (listen(server_fd, connection_backlog) != 0) {
		std::cerr << "listen failed\n";
		return 1;
	}	

	struct sockaddr_in client_addr;
	int client_addr_len = sizeof(client_addr);
	
	std::cout << "Waiting for a client to connect...\n";
	int client;

	std::string dir;
	if(argc == 3 && strcmp(argv[1], "--directory") == 0){
		dir = argv[2];
	}
	while (1)
	{
		client = accept(server_fd, (struct sockaddr *) &client_addr, (socklen_t *) &client_addr_len);
		std::cout << "Client connected\n";
		std::thread th(serve, client, dir);
		th.detach();
	}

	close(server_fd);

	return 0;
}
