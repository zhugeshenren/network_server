// server_homework.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "stdaxf.h"
#include "server_tools.h"




using namespace std;



int main(void)
{
	Server myServer;
	myServer.start();

	return 0;
}




//char add[64] = "C:/Users/R/Desktop/net_homework/send_file/calculus.pdf";

void get_size(char add[64]) {
	ifstream mySource;
	mySource.open(add, ios_base::binary);
	mySource.seekg(0, ios_base::end);
	int size = mySource.tellg();
	mySource.close();
	cout << size;
}