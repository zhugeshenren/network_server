#include "stdaxf.h"
#include "server_tools.h"
#include "threadpool.h"
#include "tools.h"



Server::Server()
{
	this->stop = false;
	this->runing_client = std::map<int, ClientConn*>();
	this->pool = new ThreadPool();

	this->pool->init(200);
}

Server::~Server()
{
	listen_thread.join();
}

void Server::start()
{
	listen_thread = std::thread(&Server::listen_conn, this);

	
	// ���ϵ�ѭ�� �Դӿ���̨��ȡ����
	std::string cmd;
	std::cout << "����������: \n";

	while (true)
	{
		char buff[128];
		int id;
		int bia;

		
		std::cin >> bia >> id >> buff;


		std::map<int, ClientConn*>::iterator iter;
		iter = runing_client.find(id);

		// ��������
		if (bia == 0) {
			iter->second->send_data(strlen(buff), buff, 0);
		}
		// �����ļ�  (Ӧ�ü�һ���� ���ҿ�һ���߳�)
		else if (bia == 1) {

			iter->second->send_file(buff);
		}
	}
	
}


void Server::listen_conn()
{
	int client_count = 0;
	int retVal = 0;

	if (WSAStartup(MAKEWORD(2, 2), &this->wsd) != 0)
	{
		std::cout << "WSAStartup failed!" << std::endl;
		return;
	}

	//����socket
	this->socket_server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (INVALID_SOCKET == this->socket_server)
	{
		std::cout << "socket failed!" << std::endl;

		WSACleanup();//�ͷ� dll
		return;
	}

	//���õ�ַ  
	this->addrServ.sin_family = AF_INET;
	this->addrServ.sin_port = htons(4999);
	this->addrServ.sin_addr.s_addr = INADDR_ANY;

	//��socket  
	retVal = bind(this->socket_server, (LPSOCKADDR)& addrServ, sizeof(SOCKADDR_IN));
	if (SOCKET_ERROR == retVal)
	{
		std::cout << "bind failed!" << std::endl;
		closesocket(this->socket_server);
		WSACleanup();

		return;
	}

	//�������� , �����������20������
	retVal = listen(this->socket_server, 20);
	if (SOCKET_ERROR == retVal)
	{
		std::cout << "listen failed!" << std::endl;
		closesocket(this->socket_server);
		WSACleanup();

		return;
	}

	
	// ���ܿͻ�������
	sockaddr_in addrClient;
	int addrClientlen = sizeof(addrClient);

	while (true)
	{
		// accept ���������߳�
		SOCKET sClient = accept(this->socket_server, (sockaddr FAR*) & addrClient, &addrClientlen);

		if (INVALID_SOCKET == sClient)
		{
			std::cout << "accept failed!" << std::endl;
			closesocket(this->socket_server);
			WSACleanup();
		}

		ClientConn* cc = new ClientConn(64, sClient,0);

		//cc_v.emplace_back(64, sClient);

		int th_id = pool->add(std::bind(&ClientConn::start, cc));
		cc->client_id = th_id;
		std::pair<int, ClientConn*> value(th_id, cc);
		runing_client.insert(value);

		std::cout << client_count << "��,��������\n";
		std::cout << "�ȴ���һ������....\n";

	}
	
}



/*
	�ͻ������ӵĹ��캯��
*/
ClientConn::ClientConn(int buff_len, SOCKET socket_client, int client_id)
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

	this->client_id = client_id;
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
		//��buf ���Ϊ0
		ZeroMemory(this->buf, this->buf_len);

		retVal = recv(this->socket_client, this->buf, this->buf_len, 0);

		// ������ʧ��֮�� , �رյ�ǰ���� , ���˳�ѭ������
		if (SOCKET_ERROR == retVal)
		{
			std::cout << "recv failed! The remote client disconnects actively" << std::endl;
			closesocket(this->socket_client);

			// ǧ����dll�ͷ���
			// WSACleanup(); 
			break;
		}

		// ��ʼ�ֽ� buf ����

		int sign_1 = buf[0];

		if (sign_1 == '0')
			break;

		// 1 ���� message ����
		// 2 ���� file �ļ�����
		
		char* new_str = Tools::copy_datas(buf, 1, retVal);
		pack* p = new pack(retVal-1,new_str);


		{
			std::lock_guard<std::mutex> lock(this->mutex_recv_pool);
			(*this->recv_pool)[sign_1 - 1].push(p);

			// �����Ժ���Ա�ɰ��������ѵ� , ���ڽ�Э�鴦�������ͻ�������뿪��
			switch (sign_1)
			{
			case 1:
				this->condition_message.notify_one();
				break;
			case 2:
				this->condition_file.notify_one();
				break;
			default:
				break;
			}
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

			// һֱ�Ѷ������ݷ�����
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
		����һ���ļ�, ��Ҫ���ݾ����Э���ʽ
	*/
	//std::cout << "file\n";
}

/*
	��recv_pool ȡ����Ϣ
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
				pack* p = que->front();

				
				std::cout << this->client_id << "�ţ��ظ� :";
				// �����Ϣ
				for (size_t i = 0; i < p->len; i++)
				{
					std::cout << p->data[i];
				}
				
				que->pop();
				std::cout << '\n';

				delete[] p->data;
				delete p;
			}
		}

	}

	std::cout << "message over\n";
}


/*
	��������, �⽫��Ϊchar* ��ָ�����ݴ���һ���µ��ڴ�ռ�,
	����ڳػ���˼�� ,���Դ���һ���ڴ��, ��������Ƶ����new �� delete , �д�ʵ��
*/
void ClientConn::send_data(std::streamsize len, char* str, int index)
{
	char * str2 = Tools::copy_chars(str,0,len);
	pack* p = new pack(len,str2);

	{
		std::unique_lock<std::mutex> lock_send(this->mutex_send_pool); 
		(*this->send_pool)[index].push(p);
		this->condition_send_pool.notify_one();
	}
}


/*
	����Ϳ���ʵ��Э�� , Լ��ÿһ�����ݰ�
	2 �����ļ�ͷ��Ϣ(��ʱ�����ļ���hashУ��)
	10 ���������ļ�
	11 �����������
*/
bool ClientConn::send_file(std::string file_path)
{
	int haveSend = 0;
	const int bufferSize = 1024;
	char buffer[bufferSize] = { 0 };

	std::string user_name = "admin";
	std::string file_name = Tools::get_file_name(file_path);
	int offset = 0;
	{
		
		buffer[offset++ ] = FILE_DATA;

		buffer[offset++] = FILE_DATA_HEAD;

		buffer[offset++] = user_name.size();	// �û����������127λ ���ᱻ�ض�

		offset += Tools::copy_datas(buffer+offset,"admin");

		buffer[offset++] = file_name.size();
		offset += Tools::copy_datas(buffer + offset, file_name);
		
		// д���ļ���С, ��λ��B
		int file_size = Tools::get_size(file_path);
		Tools::int_to_char(buffer+offset,file_size);
		offset += sizeof(int);
	}

	this->send_data(offset,buffer,1);
	
	pack* p;
	{
		std::unique_lock<std::mutex> lock_recv(this->mutex_recv_pool);
		this->condition_file.wait(lock_recv);

		std::queue<pack*>* que = &(*this->recv_pool)[1];
		p = que->front();
		que->pop();
	}


	if (p->len != 1 || p->data[0] != FILE_DATA_BEGIN_SEND) {
		std::cout << "client deny \n";

		delete[] p->data;
		delete p;

		return false;
	}

	delete[] p->data;
	delete p;

	ZeroMemory(buffer, bufferSize);
	// ��ʼ���ͳ���
	std::streamsize readLen = 0;
	std::ifstream srcFile;
	srcFile.open(file_path.c_str(), std::ios::binary);
	if (!srcFile) {
		return false;
	}
	while (!srcFile.eof()) {
		buffer[0] = 2;
		buffer[1] = 10;
		srcFile.read(buffer + 2, bufferSize - 2);
		readLen = srcFile.gcount();

		this->send_data(readLen + 2, buffer, 1);

		haveSend += readLen;
	}
	srcFile.close();

	ZeroMemory(buffer, bufferSize);
	buffer[0] = 2;
	buffer[1] = 11;
	this->send_data(2, buffer,0);


	std::cout << "send: " << haveSend << "B" << std::endl;

	return true;
}