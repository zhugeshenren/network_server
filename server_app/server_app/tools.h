#pragma once

/*
	增加一个通用的工具类
*/

#define FILE_DATA 2
#define MESSAGE_DATA 1

#define FILE_DATA_HEAD 1

#define FILE_DATA_DATA 10
#define FILE_DATA_END 11

#define FILE_DATA_BEGIN_SEND 2


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
		带new的
	*/
	static char* copy_datas(char* str1, int begin_index, std::streamsize size) {
		char* str2 = new char[size - begin_index];
		for (size_t i = begin_index; i < size; i++)
		{
			str2[i - begin_index] = str1[i];
		}
		return str2;
	}

	static int copy_datas(char * str1, std::string str2) {
		for (int i = 0; i < str2.size(); i++) {
			str1[i] = str2[i];
		}

		return str2.size();
	};

	// 获取文件大小
	static int get_size(std::string path) {
		std::ifstream mySource;
		mySource.open(path, std::ios_base::binary);
		mySource.seekg(0, std::ios_base::end);
		int size = mySource.tellg();
		mySource.close();
		return size;
	}

	static std::string get_file_name(std::string path) {
		int i = path.size() - 1;
		for (; i >= 0; i--) {
			if (path[i] == '\\' || path[i] == '/')
				break;
		}
		return path.substr(i+1,path.size()-(i+1));
	}



private:

};

Tools::Tools()
{
}

Tools::~Tools()
{
}