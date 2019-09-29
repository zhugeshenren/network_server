#pragma once
#include "threadpool.h"

/*
	创建一个客户端连接的类
*/

struct pack {
	std::streamsize len;
	char* data;

	pack(std::streamsize len ,char * data) {
		this->len = len;
		this->data = data;
	}
};


class ClientConn
{
public:
	ClientConn(int, SOCKET,int);
	~ClientConn();

	void start();


	char* buf;  //Receiving data buffer area   
	char* sendBuf;//Return data to client

	int buf_len = 0;

	std::streamsize retVal = 0;         //Return value 

	SOCKET socket_client;

	std::thread* file_thread;
	std::thread* message_thread;
	std::condition_variable condition_file;
	std::condition_variable condition_message;


	std::thread* send_thread;
	std::thread* recv_thread;

	std::vector<std::queue<pack*>>* recv_pool;
	std::vector<std::queue<pack*>>* send_pool;

	std::mutex mutex_recv_pool;
	std::mutex mutex_send_pool;
	std::condition_variable condition_send_pool;

	void recv_fun();
	void send_fun();

	void file_fun();
	void message_fun();


	void send_data(std::streamsize,char *,int);

	bool send_file(std::string name);


	int client_id;

private:

};

class Server {
public:
	Server();
	~Server();
	void start();

private:


	WSADATA         wsd;            //WSADATA Variable   

	SOCKADDR_IN     addrServ;;      //Serevr address
	SOCKET socket_server;

	void listen_conn();
	std::thread listen_thread;

	bool stop;

	std::map<int, ClientConn*> runing_client;

	ThreadPool * pool;
};


