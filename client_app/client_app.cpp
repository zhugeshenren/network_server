// client_app.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "winsock2.h"  
#include <ws2tcpip.h> 
# include <stdio.h>
#include <iostream>
#include <cstring>
#include <fstream>

#include <iostream>  
#include <ctime>
#pragma comment(lib, "ws2_32.lib")

clock_t p_start, p_end;


using namespace std;

int char_to_int(char* num) {
	return unsigned char(num[0]) * 16777216 +
		unsigned char(num[1]) * 65536 +
		unsigned char(num[2]) * 256 +
		unsigned char(num[3]);
}

void RecvFile(SOCKET sHost);

int main(int argc, char* argv[])
{
	const int BUF_SIZE = 64;
	WSADATA wsd; //WSADATA Variable  
	SOCKET sHost; //Server socket
	SOCKADDR_IN servAddr; //Server Address  
	char buf[BUF_SIZE]; //Receiving data buffer area 
	char bufRecv[BUF_SIZE];
	int retVal; //return value  
	//Initial socket dynamic lib
	if (WSAStartup(MAKEWORD(2, 2), &wsd) != 0)
	{
		cout << "WSAStartup failed!" << endl;
		return -1;
	}
	//Create socket  
	sHost = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (INVALID_SOCKET == sHost)
	{
		cout << "socket failed!" << endl;
		WSACleanup();//Release socket resource  
		return  -1;
	}

	//Set server address 
	servAddr.sin_family = AF_INET;

	// servAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

	InetPton(AF_INET, TEXT("127.0.0.1"), &servAddr.sin_addr.s_addr);


	servAddr.sin_port = htons((short)4999);
	int nServAddlen = sizeof(servAddr);

	//Connect Server 
	retVal = connect(sHost, (LPSOCKADDR)& servAddr, sizeof(servAddr));
	if (SOCKET_ERROR == retVal)
	{
		cout << "connect failed!" << endl;
		closesocket(sHost); //close socket 
		WSACleanup(); //Release socket resource   
		return -1;
	}
	while (true)
	{
		//Sending data to the Server 
		ZeroMemory(buf, BUF_SIZE);
		cout << " Sending data to the Server:  ";

		buf[0] = 1;
		cin >> (buf + 1);

		retVal = send(sHost, buf, strlen(buf), 0);
		if (SOCKET_ERROR == retVal)
		{
			cout << "send failed!" << endl;
			closesocket(sHost);
			WSACleanup();
			return -1;
		}
		//RecvLine(sHost, bufRecv);  
		ZeroMemory(bufRecv, BUF_SIZE);

		RecvFile(sHost);

		// 从服务器接受数据
		recv(sHost, bufRecv, BUF_SIZE, 0); //  
		cout << endl << "Accepting data from Server: " << bufRecv;
		cout << endl;
	}
	// 退出阶段 , 关闭
	closesocket(sHost);
	WSACleanup();
	return 0;
}


void RecvFile(SOCKET sHost) {
	cout << "start recv!" << endl;
	const int bufferSize = 1024;
	char buffer[bufferSize] = { 0 };
	string file_name = "";
	int file_len = 0;

	int readLen = 0;
	string desFileName = "C:/Users/R/Desktop/net_homework/recv_file/";
	string user_name = "";

	// 读取头数据
	readLen = recv(sHost, buffer, bufferSize, 0);
	{
		int offset = 0;

		if (buffer[offset++] != 2)
			return;

		if (buffer[offset++] != 1)
			return;

		int user_name_size = buffer[offset++];

		for (size_t i = 0; i < user_name_size; i++)
		{
			user_name += buffer[offset + i];
		}

		offset += user_name_size;

		int file_name_size = buffer[offset++];
		for (size_t i = 0; i < file_name_size; i++)
		{
			file_name += buffer[offset + i];
		}

		offset += file_name_size;

		file_len = char_to_int(buffer + offset);

		offset += sizeof(int);

		if (offset != readLen) {
			return;
		}

		ZeroMemory(buffer, bufferSize);
	}

	desFileName += file_name;
	cout << user_name << '\n';

	{
		buffer[0] = 2;
		buffer[1] = 2;

		int retVal = send(sHost, buffer, 2, 0);
		if (SOCKET_ERROR == retVal)
		{
			cout << "send failed!" << endl;
			closesocket(sHost);
			WSACleanup();
			return;
		}
	}



	ofstream desFile;
	desFile.open(desFileName.c_str(), ios::binary);
	if (!desFile)
	{
		return;
	}

	p_start = clock();
	do
	{
		readLen = recv(sHost, buffer, bufferSize, 0);
		if (readLen == 0 || buffer[1] != 10)
		{
			break;
		}
		else
		{
			desFile.write(buffer + 2, (std::streamsize)readLen - 2);
		}
	} while (true);

	p_end = clock();
	double endtime = (double)p_end - (double)p_start;

	cout << "传输时间 :" << endtime << endl;

	buffer[0] = 1;
	buffer[1] = 'a';
	buffer[2] = 'c';
	buffer[3] = 'k';
	buffer[4] = 0;

	std::cout << "send ack\n";

	std::streamsize retVal = send(sHost, buffer, strlen(buffer), 0);
	if (SOCKET_ERROR == retVal)
	{
		cout << "send failed!" << endl;
		closesocket(sHost);
		WSACleanup();
		return;
	}

	std::cout << "recv over\n";
	desFile.close();
}