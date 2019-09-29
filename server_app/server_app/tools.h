#pragma once

/*
	增加一个通用的工具类
*/

class Tools
{
public:
	Tools();
	~Tools();

	static int char_to_int(char *num) {
		return unsigned char(num[0]) * 16777216 +
			unsigned char(num[1]) * 65536 +
			unsigned char(num[2]) * 256 +
			unsigned char(num[3]);
	}

	static void int_to_char(char *num, int n) {
		num[0] = n >> 24;
		num[1] = n >> 16;
		num[2] = n >> 8;
		num[3] = n >> 0;
	}

	/*
		使用了 new
	*/
	static char* copy_chars(char *str1,int begin_index, std::streamsize size) {

		char * str2 = new char[size - begin_index + 1];
		for (size_t i = begin_index; i < size; i++)
		{
			str2[i - begin_index] = str1[i];
		}
		str2[size - begin_index] = 0;
		return str2;
	}

	/*
	
	*/
	static char* copy_datas(char* str1, int begin_index, std::streamsize size) {
		char* str2 = new char[size - begin_index];
		for (size_t i = begin_index; i < size; i++)
		{
			str2[i - begin_index] = str1[i];
		}
		return str2;
	}



private:

};

Tools::Tools()
{
}

Tools::~Tools()
{
}