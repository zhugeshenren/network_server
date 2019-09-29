#include "stdaxf.h"
#include "server_tools.h"
#include "threadpool.h"
#include "tools.h"



Server::Server()
{
	this->stop = false;
	this->runing_client = std::map<int, ClientConn*>();
	this->pool = new ThreadPool();

	this->pool->init(20);
}

Server::~Server()
{
	listen_thread.join();
}

void Server::start()
{
	listen_thread = std::thread(&Server::listen_conn, this);

	
	// 不断地循环 以从控制台读取数据
	std::string cmd;
	while (true)
	{
		char buff[128];
		int id;
		int bia;

		std::cin >> bia >> id >> buff;


		std::map<int, ClientConn*>::iterator iter;
		iter = runing_client.find(id);

		// 传送数据
		if (bia == 0) {
			iter->second->send_data(strlen(buff), buff);
		}
		// 传送文件  (应该加一个锁 并且开一个线程)
		else if (bia == 1) {
			iter->second->send_file("C:/Users/R/Desktop/net_homework/send_file/calculus.pdf");
		}
	}
	
}


void Server::listen_conn()
{
	int retVal = 0;

	if (WSAStartup(MAKEWORD(2, 2), &this->wsd) != 0)
	{
		std::cout << "WSAStartup failed!" << std::endl;
		return;
	}

	//创建socket
	this->socket_server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (INVALID_SOCKET == this->socket_server)
	{
		std::cout << "socket failed!" << std::endl;

		WSACleanup();//释放 dll
		return;
	}

	//设置地址  
	this->addrServ.sin_family = AF_INET;
	this->addrServ.sin_port = htons(4999);
	this->addrServ.sin_addr.s_addr = INADDR_ANY;

	//绑定socket  
	retVal = bind(this->socket_server, (LPSOCKADDR)& addrServ, sizeof(SOCKADDR_IN));
	if (SOCKET_ERROR == retVal)
	{
		std::cout << "bind failed!" << std::endl;
		closesocket(this->socket_server);
		WSACleanup();

		return;
	}

	//启动监听 , 并且最大允许20个连接
	retVal = listen(this->socket_server, 20);
	if (SOCKET_ERROR == retVal)
	{
		std::cout << "listen failed!" << std::endl;
		closesocket(this->socket_server);
		WSACleanup();

		return;
	}

	
	// 接受客户端请求
	sockaddr_in addrClient;
	int addrClientlen = sizeof(addrClient);

	while (true)
	{
		// accept 阻塞了主线程
		SOCKET sClient = accept(this->socket_server, (sockaddr FAR*) & addrClient, &addrClientlen);

		if (INVALID_SOCKET == sClient)
		{
			std::cout << "accept failed!" << std::endl;
			closesocket(this->socket_server);
			WSACleanup();
		}

		ClientConn* cc = new ClientConn(64, sClient);

		//cc_v.emplace_back(64, sClient);

		int th_id = pool->add(std::bind(&ClientConn::start, cc));
		
		std::pair<int, ClientConn*> value(th_id, cc);
		runing_client.insert(value);

		std::cout << "\n Listen for the next connection... \n";

	}
	
}



/*
	客户端连接的构造函数
*/
ClientConn::ClientConn(int buff_len, SOCKET socket_client)
{
	this->buf = new char[buff_len];
	this->sendBuf = new char[buff_len];
	this->socket_client = socket_client;
	this->buf_len = buff_len;

	this->file_thread = nullptr;
	this->message_thread = nullptr;
	this->recv_thread = nullptr;
	this->send_thread = nullptr;

	this->recv_pool = new std::vector<std::queue<pack *>>(2);
	this->send_pool = new std::vector<std::queue<pack *>>(2);
}

ClientConn::~ClientConn()
{
	delete this->recv_thread;
	delete this->send_thread;
	delete this->file_thread;
	delete this->message_thread;
}


void ClientConn::start()
{
	this->recv_thread = new std::thread([this] {
		this->recv_fun();
		});

	this->send_thread = new std::thread([this] {
		this->send_fun();
		});

	this->file_thread = new std::thread([this] {
		this->file_fun();
		});
	this->message_thread = new  std::thread([this] {
		this->message_fun();
		});

	this->recv_thread->join();
	this->send_thread->join();
	this->file_thread->join();
	this->message_thread->join();
}

void ClientConn::recv_fun()
{
	while (true)
	{
		//将buf 填充为0
		ZeroMemory(this->buf, this->buf_len);

		retVal = recv(this->socket_client, this->buf, this->buf_len, 0);

		// 当接受失败之后 , 关闭当前连接 , 并退出循环即可
		if (SOCKET_ERROR == retVal)
		{
			std::cout << "recv failed! The remote client disconnects actively" << std::endl;
			closesocket(this->socket_client);

			// 千万别把dll释放了
			// WSACleanup(); 
			break;
		}

		// 开始分解 buf 数据

		int sign_1 = buf[0];

		if (sign_1 == '0')
			break;

		// 1 代表 message 传输
		// 2 代表 file 文件传输
		
		char* new_str = Tools::copy_chars(buf, 1,retVal);
		pack* p = new pack(10,new_str);

		{
			std::lock_guard<std::mutex> lock(this->mutex_recv_pool);
			(*this->recv_pool)[sign_1 - 1].push(p);
			this->condition_message.notify_one();
		}
	}

}

void ClientConn::send_fun()
{
	while (true)
	{
		std::unique_lock<std::mutex> lock(this->mutex_send_pool);
		this->condition_send_pool.wait(lock);

		for (int i = 0; i < 2; i++) {
			std::queue<pack*>* que = &(*this->send_pool)[i];

			// 一直把队列数据发送完
			while (!que->empty())
			{
				std::streamsize retVal = send(this->socket_client, que->front()->data,
					que->front()->len, 0);

				if (SOCKET_ERROR == retVal)
				{
					std::cout << "bind failed!" << std::endl;
					closesocket(this->socket_client);
					WSACleanup();

					return;
				}

				delete[] que->front()->data;
				delete que->front();
				que->pop();
			}
		}
	}
}

void ClientConn::file_fun()
{
	/*
		接收一个文件, 这要根据具体的协议格式
	*/
	std::cout << "file\n";
}

/*
	从recv_pool 取出消息
*/
void ClientConn::message_fun()
{
	while (true) {

		{
			std::unique_lock<std::mutex> lock_recv(this->mutex_recv_pool);
			this->condition_message.wait(lock_recv);

			std::queue<pack*>* que = &(*this->recv_pool)[0];
			while (!que->empty())
			{
				char* str = que->front()->data;
				pack* p = que->front();
				que->pop();

				std::cout << str << '\n';

				delete[] str;
				delete p;
			}
		}

	}

	std::cout << "message over\n";
}


/*
	发送数据, 这将会为char* 所指的数据创建一块新的内存空间,
	类比于池化的思想 ,可以创建一块内存池, 这样不用频繁的new 和 delete , 尚待实现
*/
void ClientConn::send_data(std::streamsize len, char* str)
{
	char * str2 = Tools::copy_chars(str,0,len);
	pack* p = new pack(len,str2);

	{
		std::unique_lock<std::mutex> lock_send(this->mutex_send_pool); 
		(*this->send_pool)[1].push(p);
		this->condition_send_pool.notify_one();
	}
}


/*
	这里就可以实现协议
*/
bool ClientConn::send_file(std::string file_name)
{
	int haveSend = 0;
	const int bufferSize = 1024;
	char buffer[bufferSize] = { 0 };
	std::streamsize readLen = 0;
	std::ifstream srcFile;
	srcFile.open(file_name.c_str(), std::ios::binary);
	if (!srcFile) {
		return false;
	}
	while (!srcFile.eof()) {
		buffer[0] = 10;
		srcFile.read(buffer + 1, bufferSize - 1);
		readLen = srcFile.gcount();

		this->send_data(readLen + 1, buffer);

		haveSend += readLen;
	}
	srcFile.close();


	buffer[0] = 11;
	send(this->socket_client, buffer, 1, 0);

	std::cout << "send: " << haveSend << "B" << std::endl;

	return true;
}