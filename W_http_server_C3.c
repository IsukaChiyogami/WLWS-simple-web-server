/*
*  all question please send to 35118767065@qq.com (or tell me directly in QQ)
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <Winsock2.h>
#pragma comment(lib,"Ws2_32.lib")
#pragma warning(disable:4996)

//#define RECORD_IN_FILE 0 //宏定义后启用记录文件，否则仅显示于控制台 

#define DEFAULT_PORT 80u //监听端口
#define MAX_CONNECT_NUMBER 10u //最大连接数
#define RECORD_FILE_NAME "HTTP_record.txt" //默认记录文件
#define HTTP_VERSION "HTTP/1.1" //默认HTTP版本 :  1.0=原始版 1.1=保持TCP连接，多HTTP复用TCP  
#define ROOT_PATH "E:/winc/web_sinse2022/w_desktop/" //根目录
#define IP_PROTOCOL_IPv4_6 AF_INET //默认未定义既IPv4，定义生效既IPv6

#define REQUEST_BUF_SIZE_UNIT 4096u //make request缓冲区单元大小(初始化不能小于一单元大小，每次扩容一单元)
#define REQUEST_RECV_BUF_SIZE_UNIT 4096u //request接收缓冲区最小单位容量 (初始化也必须大于改大小,否则被强制扩容至该大小)
#define REQUEST_BUF_BASELINE_SIZE 512u //make request检测时小于这个大小就马上扩容
#define REQUEST_HTTP_VERSION " HTTP/1.0\r\n" //请求http版本 必须以 " HTTP/x.x\r\n"形式
#define REQUEST_RECV_SIZE_PER_TIME 1024u //request每次接收大小

#define RECV_BUFFER_SIZE 4096u //TCP首次接收缓冲区大小
#define RECV_MAX_CONTENT_LENGTH 2097152u //接收正文最大大小 2MB (2*1024*1024)
#define MAXLEN_HTTP_METHOD 7u //最大方法长度 6，+1
#define MAXLEN_HTTP_URL 2049u //最大URL长度 1024，+1
#define MAXLEN_HTTP_VERSION 9u //最大方法长度 8，+1 如 -HTTP/1.1-

#define RESPOND_CHUNK_ENCODING_THRESHOLD 102400u //返回内容超过此阈值则采用chunk编码以节省内存空间(实测回应时间会延长很多) 单位Byte

#define MAX_HTTP_HEADER_PAGE_NUM 3u //最多 3*10 个HTTP标头
#define MAX_COOKIE_PAGE 3u // 最多cookie数量：3*10 个
#define MAX_UNIT_PER_PAGE_IN_CHAIN 10u //链表每页10个

#define MAX_THREAD_LOG_ID 30000 //到达这个数后归零线程id
#define record_down(content) if(record_file){fwrite(content, sizeof(char), strlen(content), record_file);}else{printf("文件已关闭，记录失败:|%s|\n",content);} //记录到文件
#ifdef RECORD_IN_FILE
#define print_and_record(content) printf(content);record_down(content) //打印并记录
#else
#define print_and_record(content) printf(content) //打印并记录
#endif

//unsingned int typedef
typedef unsigned short u2;//2byte
typedef unsigned int u4;//4byte
typedef unsigned long long u8;//8byte

//传参用句柄与套接字组合结构体
struct param_u2_and_socket
{
	u2 thread_id;
	SOCKET client_socket;
};

//字典单元 结构体
struct dict_ele
{
	char* name;
	char* value;
};

 //10单位 字典 链表 定义变量后一定要初始化len与next置零
struct dict_chain_10u
{
	struct dict_ele eles[MAX_UNIT_PER_PAGE_IN_CHAIN];
	u2 len;
	struct dict_chain_10u* next;
};


u2 thread_total_num = 0;//HTTP连接处理线程总数
u8 thread_id_cycle = 0;//循环id池
HANDLE server_loop_handle = 0;//核心服务线程句柄
FILE* record_file = 0;// 记录与主页 文件指针
u2 cleanup = 0;//是否需要清除(WSA clean up)
struct dict_chain_10u request_headers,response_headers;//通用请求与回答标头
u2 root_path_len = 0;//根目录字符串长度
u8 index_len = 0;//index.html文件长
char* index_content = 0;//index.html文件内容


/*
* @brief 16转10进制
* @param c 要转换的字符（数字）
* @return 转换后的值，-1则为错误
*/
int hex2dec(char c)
{
	if ('0' <= c && c <= '9')
	{
		return c - '0';
	}
	else if ('a' <= c && c <= 'f')
	{
		return c - 'a' + 10;
	}
	else if ('A' <= c && c <= 'F')
	{
		return c - 'A' + 10;
	}
	else
	{
		return -1;
	}
}


/*
* @brief UTF8转ANSI
* @param str 输入的字符串
* @param result 返回解码后的字符串
*/
UTF8toANSI(const char* str, char* result)
{
	int textlen;
	wchar_t* turn;
	textlen = MultiByteToWideChar(CP_UTF8, 0, str, -1, NULL, 0);
	turn = (wchar_t*)calloc((textlen + 1), sizeof(wchar_t));
	MultiByteToWideChar(CP_UTF8, 0, str, -1, turn, textlen);

	textlen = WideCharToMultiByte(CP_ACP, 0, turn, -1, NULL, 0, NULL, NULL);
	WideCharToMultiByte(CP_ACP, 0, turn, -1, result, textlen, NULL, NULL);
	free(turn);
	return ;
}

/*
* @brief ANSI转UTF8
* @param str 输入的字符串
* @param result 返回解码后的字符串
*/
 ANSItoUTF8(const char* str,char* result)
{
	int textlen;
	wchar_t* turn;
	textlen = MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0);
	turn = (wchar_t*)calloc((textlen + 1) , sizeof(wchar_t));
	MultiByteToWideChar(CP_ACP, 0, str, -1, turn, textlen);

	textlen = WideCharToMultiByte(CP_UTF8, 0, turn, -1, NULL, 0, NULL, NULL);
	WideCharToMultiByte(CP_UTF8, 0, turn, -1, result, textlen, NULL, NULL);
	free(turn);
	return ;
}



/*
* @brief 在记录文件中记录当前时间(没有换行)
* @ param scription 记录的内容
*/
void inline print_and_record_with_time(const char* scription)
{
	time_t now_t = time(0);
	struct tm* now_tm = localtime(&now_t);
	char now_str[32];
	memset(now_str, 0, 32);
	strftime(now_str, 32, "%Y-%m-%d %H:%M:%S:\t", now_tm);
	u4 time_str_len = strlen(now_str);
	u8 scription_len = strlen(scription);
	char* output_cont = (char*)malloc(time_str_len + scription_len + 1);
	memcpy(output_cont, now_str, time_str_len);
	memcpy(output_cont + time_str_len, scription, scription_len + 1);
	print_and_record(output_cont);
	free(output_cont);
}

/*
* @brief HTTP状态码转名称
* @param state_code 状态码
* @return 返回对应名称
*/
const char* state_code_2_name(u2 state_code)
{
	switch (state_code)
	{
	case 100:
		return "Continue";
		break;
	case 101:
		return "Switching Protocols";
		break;
	case 200:
		return "OK";
		break;
	case 201:
		return "Created";
		break;
	case 202:
		return "Accepted";
		break;
	case 203:
		return "Non-Authoritative Information";
		break;
	case 204:
		return "No Content";
		break;
	case 205:
		return "Reset Content";
		break;
	case 206:
		return "Partial Content";
		break;
	case 300:
		return "Multiple Choices";
		break;
	case 301:
		return "Moved Permanently";
		break;
	case 302:
		return "Found";
		break;
	case 303:
		return "See Other";
		break;
	case 304:
		return "Not Modified";
		break;
	case 305:
		return "Use Proxy";
		break;
	case 307:
		return "Temporary Redirect";
		break;
	case 400:
		return "Bad Request";
		break;
	case 401:
		return "Unauthorized";
		break;
	case 402:
		return "Payment Required";
		break;
	case 403:
		return "Forbidden";
		break;
	case 404:
		return "Not Found";
		break;
	case 405:
		return "Method Not Allowed";
		break;
	case 406:
		return "Not Acceptable";
		break;
	case 407:
		return "Proxy Authentication Required";
		break;
	case 408:
		return "Request Time-out";
		break;
	case 409:
		return "Conflict";
		break;
	case 410:
		return "Gone";
		break;
	case 411:
		return "Length Required";
		break;
	case 412:
		return "Precondition Failed";
		break;
	case 413:
		return "Request Entity Too Large";
		break;
	case 414:
		return "Request-URI Too Large";
		break;
	case 415:
		return "Unsupported Media Type";
		break;
	case 416:
		return "Requested range not satisfiable";
		break;
	case 417:
		return "Expectation Failed";
		break;
	case 500:
		return "Internal Server Error";
		break;
	case 501:
		return "Not Implemented";
		break;
	case 502:
		return "Bad Gateway";
		break;
	case 503:
		return "Service Unavailable";
		break;
	case 504:
		return "Gateway Time-out";
		break;
	case 505:
		return "HTTP Version not supported";
		break;
	default:
		break;
	}
}

/*
* @brief 文件后缀名转为HTTP协议Content-type(MIME)
* @param suffix 文件后缀名(不带.)
* @return 返回MIME
*/
const char* file_suffix_2_MIME(const char* suffix)
{
	if (suffix == 0)
	{
		return "text/plain";
	}
	else if (memcmp(suffix, "html", 4) == 0)
	{
		return "text/html";
	}
	else if (memcmp(suffix, "icon", 4) == 0)
	{
		return "image/x-icon";
	}
	else if (memcmp(suffix, "docx", 4) == 0)
	{
		return "application/vnd.openxmlformats-officedocument.wordprocessingml.document";
	}
	else if (memcmp(suffix, "xlsx", 4) == 0)
	{
		return "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet";
	}
	else if (memcmp(suffix, "pptx", 4) == 0)
	{
		return "application/vnd.openxmlformats-officedocument.presentationml.presentation";
	}
	else if (memcmp(suffix, "zip", 3) == 0 || memcmp(suffix, "7z", 2) == 0)
	{
		return "application/zip";
	}
	else if (memcmp(suffix, "rar", 3) == 0)
	{
		return "application/rar";
	}
	else if (memcmp(suffix, "pdf", 3) == 0)
	{
		return "application/pdf";
	}
	else if (memcmp(suffix, "gif", 3) == 0)
	{
		return "image/gif";
	}
	else if (memcmp(suffix, "jpg", 3) == 0 || memcmp(suffix, "jpeg", 4) == 0)
	{
		return "image/jpeg";
	}
	else if (memcmp(suffix, "png", 3) == 0)
	{
		return "image/png";
	}
	else if (memcmp(suffix, "bmp", 3) == 0)
	{
		return "image/bmp";
	}
	else if (memcmp(suffix, "js", 2) == 0)
	{
		if (memcmp(suffix, "json", 4) == 0)
		{
			return "application/json";
		}
		else
		{
			return "text/javascript";
		}
	}
	else if (memcmp(suffix, "css", 3) == 0)
	{
		return "text/css";
	}
	else if (memcmp(suffix, "mp3", 3) == 0)
	{
		return "audio/mpeg";
	}
	else if (memcmp(suffix, "mp4", 3) == 0)
	{
		return "video/mp4";
	}
	else
	{
		return "text/plain";
	}
	return "text/plain";
}


/*
* @brief 在字典链表中增加一个元素(表满则顺延到下一表或创建下一表)
* @param target 目标表
* @param ele_name 创建的元素名称
* @param ele_value 创建的元素值
*/
void dict_chain_apeend_ele(struct dict_chain_10u* target, const char* ele_name, const char* ele_value)
{
	struct dict_chain_10u* handling = target;
	while (handling->len == MAX_UNIT_PER_PAGE_IN_CHAIN)
	{
		if (handling->next==0)
			handling->next = (struct dict_chain_10u*)calloc(1u, sizeof(struct dict_chain_10u));
		handling = handling->next;
	}
	u8 len_buf = strlen(ele_name)+1;
	handling->eles[handling->len].name = (char*)malloc(len_buf * sizeof(char));
	memcpy(handling->eles[handling->len].name, ele_name, len_buf);
	len_buf = strlen(ele_value) + 1;
	handling->eles[handling->len].value = (char*)malloc(len_buf * sizeof(char));
	memcpy(handling->eles[handling->len].value, ele_value, len_buf);
	handling->len++;
}

/*
* @brief 释放字典链表结构体全链(链首只清空不释放)
* @param on_free 需要释放的字典链表 链头 指针
* 
*/
void dict_chain_free(struct dict_chain_10u* on_free)
{
	struct dict_chain_10u* freeing = on_free;
	u2 chead = 1;
	while (freeing->len--)
	{
		free(freeing->eles[freeing->len].name);
		free(freeing->eles[freeing->len].value);
		if (freeing->len==0)
		{
			if (freeing->next)
			{
				if (chead)
				{
					freeing = freeing->next;
					chead = 0;
				}
				else
				{
					struct dict_chain_10u* fbuf = freeing;
					freeing = freeing->next;
					free(fbuf);
				}
			}
		}
	}
	if (chead == 0)
	{
		free(freeing);
	}
}


/*
* @brief 制作请求包
* @param[in,out] buf_pointer 指向结果字符串的指针的指针
* @param[in] buf_init_size 小于REQUEST_BUF_SIZE_UNIT自动补为该值
* @param[in] method 请求方法
* @param[in] url 统一资源定位符
* @param[in] url_len url长度
* @param[in] headers 自定义标头
* @param[in] content 报文内容
* @param[in] cont_len 内容长度
* @param[in] content_type 内容类型
* @return 返回buf_pointer 指向的指针的内容的长度
*/
u8 HTTP_make_request(char** buf_pointer, const u8 buf_init_size, const char* const method, const char* const url, const u8 url_len, struct dict_chain_10u* headers, const char* const content, const u8 cont_len, const char* content_type)
{
	//init

	u8 mpos = 0, add_size = 0, rlen = buf_init_size > REQUEST_BUF_SIZE_UNIT ? buf_init_size : REQUEST_BUF_SIZE_UNIT;
	char* mbuf = (char*)malloc(rlen);
	if (!mbuf)
	{
		printf("make_request 崩溃: malloc 无法分配要求的内存 request size:%d", rlen);
	}
	*buf_pointer = mbuf;

	// method

	if (memcmp(method, "GET", 3) == 0)
	{
		memcpy(mbuf, "GET /", 5);
		mpos = 5;
	}
	else if (memcmp(method, "POST", 4) == 0)
	{
		memcpy(mbuf, "POST /", 6);
		mpos = 6;
	}
	else
	{
		printf("make request unsopported HTTP method:%s", method);
		return 0u;
	}

	// url

	memcpy(mbuf + mpos, url, url_len);
	mpos += url_len;

	// version

	memcpy(mbuf + mpos, REQUEST_HTTP_VERSION, 11);
	mpos += 11;

	if (rlen - mpos < REQUEST_BUF_BASELINE_SIZE)
	{
		rlen += REQUEST_BUF_SIZE_UNIT;
		mbuf = (char*)realloc(mbuf, rlen);
	}

	//通用标头 public header
	struct dict_chain_10u* on_make = &request_headers;
	for (u2 num0 = 0; num0 < MAX_UNIT_PER_PAGE_IN_CHAIN * MAX_HTTP_HEADER_PAGE_NUM; num0++)
	{
		u2 opt = num0 % MAX_UNIT_PER_PAGE_IN_CHAIN;
		if (opt == 0)
		{
			if (num0)
			{
				if (on_make->next == 0)
					break;
				else
					on_make = on_make->next;
			}
		}
		if (opt == on_make->len)
			break;

		add_size = strlen(on_make->eles[num0].name);
		memcpy(mbuf + mpos, on_make->eles[num0].name, add_size);
		mpos += add_size;

		memcpy(mbuf + mpos, ": ", 2);
		mpos += 2;

		add_size = strlen(on_make->eles[num0].value);
		memcpy(mbuf + mpos, on_make->eles[num0].value, add_size);
		mpos += add_size;

		memcpy(mbuf + mpos, "\r\n", 2);
		mpos += 2;
	}

	if (rlen - mpos < REQUEST_BUF_BASELINE_SIZE)
	{
		rlen += REQUEST_BUF_SIZE_UNIT;
		mbuf = (char*)realloc(mbuf, rlen);
	}

	//指定标头 private header
	if (headers)
	{
		on_make = headers;
		for (u2 num0 = 0; num0 < MAX_UNIT_PER_PAGE_IN_CHAIN * MAX_HTTP_HEADER_PAGE_NUM; num0++)
		{
			u2 opt = num0 % MAX_UNIT_PER_PAGE_IN_CHAIN;
			if (opt == 0)
			{
				if (num0)
				{
					if (on_make->next == 0)
						break;
					else
						on_make = on_make->next;
				}
			}
			if (opt == on_make->len)
				break;

			add_size = strlen(on_make->eles[num0].name);
			memcpy(mbuf + mpos, on_make->eles[num0].name, add_size);
			mpos += add_size;

			memcpy(mbuf + mpos, ": ", 2);
			mpos += 2;

			add_size = strlen(on_make->eles[num0].value);
			memcpy(mbuf + mpos, on_make->eles[num0].value, add_size);
			mpos += add_size;

			memcpy(mbuf + mpos, "\r\n", 2);
			mpos += 2;
		}
	}

	if (rlen - mpos < REQUEST_BUF_BASELINE_SIZE)
	{
		rlen += REQUEST_BUF_SIZE_UNIT;
		mbuf = (char*)realloc(mbuf, rlen);
	}

	//内容
	if (content && cont_len && content_type)
	{
		//正文类型header
		memcpy(mbuf + mpos, "Content-Type: ", 14);
		mpos += 14;

		add_size = strlen(content_type);
		memcpy(mbuf + mpos, content_type, add_size);
		mpos += add_size;

		memcpy(mbuf + mpos, "\r\n", 2);
		mpos += 2;

		//正文长header
		char len_str[32];
		memset(len_str, 0, 32);
		itoa(cont_len, len_str, 10);

		memcpy(mbuf + mpos, "Content-Length: ", 16);
		mpos += 16;

		add_size = strlen(len_str);
		memcpy(mbuf + mpos, len_str, add_size);
		mpos += add_size;

		memcpy(mbuf + mpos, "\r\n\r\n", 4);
		mpos += 4;

		//正文体
		if (rlen < mpos + cont_len)
		{
			mbuf = (char*)realloc(mbuf, mpos + cont_len + 32u);
		}
		memcpy(mbuf + mpos, content, cont_len);
		mpos += cont_len;
	}
	else
	{
		memcpy(mbuf + mpos, "\r\n", 2);
		mpos += 2;
	}
	return mpos;

}



/*
* @brief 请求单个资源
* @param[in] rhost 主机 www.baidu.com / 123.456.789.1
* @param[in] rmessage 发送的报文(含方法标头内容从头到尾)
* @param[in] rmessage_len rmessage长度
* @param[out] rrecv_buf_pointer 接收缓冲区的指针的指针(指向一个未分配内存的char指针)
* @param[in] rrecv_init_buf_len 首次接收缓冲区大小
* @return 共接收的长度
*/
u8 HTTP_request(const char* const rhost, const char* const rmessage, const u8 rmessage_len, char** rrecv_buf_pointer, const u8 rrecv_init_buf_len)
{

	//----------------------
	// Declare and initialize variables.
	int iResult;
	u8 total_len = 0;
	//----------------------
	// The sockaddr_in structure specifies the address family,
	// IP address, and port of the server to be connected to.

	//-----------------------
	//获取ip地址

	struct addrinfo server_setting, * result;

	memset(&server_setting, 0, sizeof(server_setting));
	server_setting.ai_family = AF_INET;
	server_setting.ai_socktype = SOCK_STREAM;

	iResult = getaddrinfo(rhost, "80", &server_setting, &result);
	if (iResult)
	{
		printf("get addr info err : getaddrinfo return code : %d  err code = %d\n", iResult, WSAGetLastError());

		return 0;
	}

	SOCKET ConnectSocket = INVALID_SOCKET;

	//----------------------
	// Create a SOCKET for connecting to server
	ConnectSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (ConnectSocket == INVALID_SOCKET) {
		printf("socket failed with error: %ld\n", WSAGetLastError());
		return 0;
	}

	//----------------------
	// Connect to server.
	iResult = connect(ConnectSocket, result->ai_addr, result->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		printf("connect failed with error: %d\n", WSAGetLastError());
		closesocket(ConnectSocket);

		return 0;
	}

	//----------------------
	// Send an initial buffer
	iResult = send(ConnectSocket, rmessage, rmessage_len, 0);
	if (iResult == SOCKET_ERROR) {
		printf("send failed with error: %d\n", WSAGetLastError());
		closesocket(ConnectSocket);

		return 0;
	}


	// shutdown the connection since no more data will be sent
	//iResult = shutdown(ConnectSocket, SD_SEND);
	//if (iResult == SOCKET_ERROR) {
	//    printf("shutdown failed with error: %d\n", WSAGetLastError());
	//    closesocket(ConnectSocket);
	//    
	//    return 0;
	//}

	u8 recv_buf_len = rrecv_init_buf_len > REQUEST_RECV_BUF_SIZE_UNIT ? rrecv_init_buf_len : REQUEST_RECV_BUF_SIZE_UNIT;
	char* recv_buf = (char*)malloc(recv_buf_len);
	// Receive until the peer closes the connection
	int recv_time_out = 5000;
	setsockopt(ConnectSocket, SOL_SOCKET, SO_RCVTIMEO, (char*)&recv_time_out, sizeof(int));
	u8 recv_ret = 1;
	while (recv_ret)
	{
		recv_ret = recv(ConnectSocket, recv_buf + total_len, REQUEST_RECV_SIZE_PER_TIME, 0);
		if (recv_ret == SOCKET_ERROR)
		{
			printf("recv接收错误,错误码:%d ,rhost:|%s|\n", WSAGetLastError(), rhost);
			closesocket(ConnectSocket);

			return 0;
		}
		total_len += recv_ret;
		if (total_len + REQUEST_RECV_SIZE_PER_TIME > recv_buf_len)
		{
			recv_buf_len += REQUEST_RECV_BUF_SIZE_UNIT;
			recv_buf = (char*)realloc(recv_buf, recv_buf_len);
			if (recv_buf == 0)
			{
				printf("realloc failed !");
				return 0;
			}
		}
	}
	*rrecv_buf_pointer = recv_buf;

	// close the socket
	iResult = closesocket(ConnectSocket);
	if (iResult == SOCKET_ERROR) {
		printf("close failed with error: %d\n", WSAGetLastError());
		return 0;
	}


	return total_len;
}


/*
* @brief 发送HTTP回答，函数正常完成内部不做closesocket操作
* @param client 客户端套接字
* @param state_code HTTP状态码
* @param headers 在通用header (全局变量 response_headers) 上增加，(0则为空)
* @param content 报文正文内容 (长度与内容任一为0则此俩参数无效，不发送正文体与正文长header)
* @param content_len 报文正文内容长度
* @param content_type 报文正文内容类型(0则为空)
*/
void HTTP_respond(const SOCKET client, const u2 state_code, struct dict_chain_10u * headers, const char* const content, const u8 content_len, const char* const content_type)
{
	//头信息栏
	char send_buf[128],trans_buf[16];
	memset(send_buf, 0, 128);
	memset(trans_buf, 0, 16);
	memcpy(send_buf, HTTP_VERSION, 8);
	send_buf[8] = ' ';
	itoa(state_code, trans_buf, 10);
	memcpy(send_buf + 9, trans_buf, 4);
	send_buf[12] = ' ';
	const char* state_name = state_code_2_name(state_code);
	u4 state_len = strlen(state_name);
	memcpy(send_buf + 13, state_name, state_len);
	memcpy(send_buf + state_len + 13, "\r\n", 2);
	send(client, send_buf, state_len + 15, 0);

	//通用标头
	struct dict_chain_10u* on_send = &response_headers;
	for (u2 num0 = 0; num0 < MAX_UNIT_PER_PAGE_IN_CHAIN * MAX_HTTP_HEADER_PAGE_NUM; num0++)
	{
		u2 opt = num0 % MAX_UNIT_PER_PAGE_IN_CHAIN;
		if (opt == 0)
		{
			if (num0)
			{
				if (on_send->next == 0)
					break;
				else
					on_send = on_send->next;
			}
		}
		if (opt == on_send->len)
			break;
		send(client, on_send->eles[num0].name, strlen(on_send->eles[num0].name), 0);
		send(client, ": ", 2, 0);
		send(client, on_send->eles[num0].value, strlen(on_send->eles[num0].value), 0);
		send(client, "\r\n", 2, 0);
	}

	//指定标头
	if (headers)
	{
		on_send = headers;
		for (u2 num0 = 0; num0 < MAX_UNIT_PER_PAGE_IN_CHAIN * MAX_HTTP_HEADER_PAGE_NUM; num0++)
		{
			u2 opt = num0 % MAX_UNIT_PER_PAGE_IN_CHAIN;
			if (opt == 0)
			{
				if (num0)
				{
					if (on_send->next == 0)
						break;
					else
						on_send = on_send->next;
				}
			}
			if (opt == on_send->len)
				break;
			send(client, on_send->eles[num0].name, strlen(on_send->eles[num0].name), 0);
			send(client, ": ", 2, 0);
			send(client, on_send->eles[num0].value, strlen(on_send->eles[num0].value), 0);
			send(client, "\r\n", 2, 0);
		}
	}

	//正文类型header
	if (content_type)
	{
		send(client, "Content-Type: ", 14, 0);
		send(client, content_type, strlen(content_type), 0);
		send(client, "\r\n", 2, 0);
	}

	//内容
	if (content && content_len)
	{

		//正文长header
		char len_str[32];
		memset(len_str, 0, 32);
		itoa(content_len, len_str, 10);
		send(client, "Content-Length: ", 16, 0);
		send(client, len_str, strlen(len_str), 0);
		send(client, "\r\n\r\n", 4, 0);

		//正文体
		send(client, content, content_len, 0);
	}
	else
	{
		send(client, "\r\n", 2, 0);
	}

}

u2 CRA_state = 0;//CRA页面所需值
/*
* @brief 主要的自定义区域，提供请求的解析数据，要求做出返回
* @param client 用户端套接字
* @param method 请求方法
* @param url 请求统一资源定位符
* @param path 请求路径
* @param query 表单
* @param fragment 定位符
* @param version HTTP版本
* @param headers 标头
* @param content 正文内容
* @param cont_len 正文内容长度
* @return 返回赋值 keep_alive，决定是否保持连接 1:保持 0: 关闭
*/
u2 reply_client(SOCKET client, const char* method, const char* url, const char* path, const char* query, const char* fragment,const char* version, const struct dict_chain_10u* headers, const char* content, u8 cont_len)
{
	u2 keep_alive = 1;
	//回应客户端
	if (memcmp(method, "GET", 3) == 0)
	{
		if (memcmp(path, "index.html", 10) == 0)
		{
			HTTP_respond(client, 200, 0, index_content, index_len, "text/html");
		}
		else if (memcmp(path, "CRA/change_alerm_state.opt", 26) == 0)
		{
			if (query[0]=='s'&&query[1]=='='&&query[2]>='0'&&query[2]<='9')
			{
				CRA_state = query[2] - '0';
				HTTP_respond(client, 200, 0, "finish", 6, "text/plain");
			}
			else
			{
				HTTP_respond(client, 200, 0, "syntax error", 12, "text/plain");
			}
		}
		else if (memcmp(path, "CRA/change_alerm_state.cdt", 26) == 0)
		{
			char state_buf = '0' + CRA_state;
			HTTP_respond(client, 200, 0, &state_buf, 1, "text/plain");
		}
		else if (memcmp(path, "winc/transmit/get.rkcc", 22) == 0)//stock info transmit
		{
			char* hostr = 0;
			if (memcmp(query, "t=tx", 4) == 0)
			{
				hostr = "qt.gtimg.cn";
			}
			else if (memcmp(query, "t=ne", 4) == 0)
			{
				hostr = "img1.money.126.net";
			}
			else
			{
				HTTP_respond(client, 403, 0, 0, 0, 0);
			}

			char* make_buf = 0, * trans_buf = 0;
			struct dict_chain_10u dheader={.next=0,.len=0};
			dict_chain_apeend_ele(&dheader, "Host", hostr);
			dict_chain_apeend_ele(&dheader, "Accept-Encoding", "gzip");
			dict_chain_apeend_ele(&dheader, "Accept-Language", "zh-CN,zh;q=0.9,en;q=0.8,en-GB;q=0.7,en-US;q=0.6");
			dict_chain_apeend_ele(&dheader, "Accept", "text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.9");
			u8 rlen = HTTP_make_request(&make_buf, REQUEST_BUF_SIZE_UNIT, "GET", query + 5, strlen(query + 5), &dheader, 0, 0, 0);
			dict_chain_free(&dheader);
			rlen = HTTP_request(hostr, make_buf, rlen, &trans_buf, 8192u);
			free(make_buf);
			if (trans_buf)
			{
				u8 rrlen = rlen;
				char* bpos = memchr(trans_buf, 'C', rlen);
				while (bpos)
				{
					rrlen = rlen - (bpos - trans_buf);
					if (memcmp(bpos, "Content-Encoding:", 17) == 0)
					{
						bpos += 17;
						while (bpos[0] == ' ')
							bpos++;
						char* fpos = memchr(bpos, '\r', 32);
						if (fpos)
						{
							rrlen = fpos - bpos;

							struct dict_chain_10u rheader = { .len = 1,.next = 0 };
							rheader.eles[0].name = (char*)malloc(18);
							memcpy(rheader.eles[0].name, "Content-Encoding", 18);
							rheader.eles[0].value = (char*)malloc(rrlen + 1);
							memcpy(rheader.eles[0].value, bpos, rrlen);
							rheader.eles[0].value[rrlen] = 0;
							
							rrlen = rlen - (bpos - trans_buf);
							bpos = memchr(bpos, '\n', rrlen);
							while (bpos)
							{
								if (bpos[2] == '\n')
								{
									bpos += 3;
									rrlen = rlen - (bpos - trans_buf);
									HTTP_respond(client, 200, &rheader, bpos, rrlen, "text/html");
									break;
								}
								rrlen = rlen - (bpos - trans_buf);
								bpos = memchr(bpos, '\n', rrlen);
							}
							dict_chain_free(&rheader);
						}
						else
						{
							HTTP_respond(client, 502, 0, 0, 0, 0);
						}
						break;
					}
					bpos = memchr(bpos + 1, 'C', rrlen);
				}
				free(trans_buf);
			}
			else
			{
				HTTP_respond(client, 502, 0, 0, 0, 0);
			}
		}
		else//通用情况
		{
			u4 path_len = strlen(path);
			char* request_path = (char*)malloc(root_path_len + path_len + 1, sizeof(char));
			if (!request_path)
			{
				printf("reply_client get commnon condition : malloc failed");
			}
			memcpy(request_path, ROOT_PATH, root_path_len);
			memcpy(request_path + root_path_len, path, path_len + 1);
			request_path[root_path_len + path_len] = 0;
			FILE* request_file = 0;
			errno_t fo_code = fopen_s(&request_file, request_path, "rb");
			free(request_path);
			if (fo_code)
			{
				printf("文件访问错误: fopen_s path: |%s| error code: |%d| \n", path, fo_code);
				HTTP_respond(client, 404, 0, 0, 0, 0);
			}
			else
			{
				fseek(request_file, 0, SEEK_END);
				u8 req_flen = ftell(request_file);
				fseek(request_file, 0, SEEK_SET);

				if (req_flen)
				{

					u4 point_pos = 0;
					char* file_suffix = 0;
					for (u4 num = 0; num < path_len; num++)
						if (path[num] == '.')
							point_pos = num;
					if (point_pos < path_len)
					{
						u2 path_rest_len = path_len - point_pos + 1;
						file_suffix = (char*)malloc(path_rest_len);
						memcpy(file_suffix, path + point_pos + 1, path_rest_len);
					}
					//内容类型content-type
					char* cont_type = file_suffix_2_MIME(file_suffix);
					free(file_suffix);
					//自定义header
					struct dict_chain_10u temporary_header = { .len = 0, .next = 0 };
					struct dict_chain_10u* header_m = 0;
					if (memcmp(path, "favicon.ico", 11) == 0 || memcmp(path, "maoyuna.png", 11) == 0 || memcmp(path, "dbp.png", 7) == 0)
					{
						header_m = &temporary_header;
						dict_chain_apeend_ele(header_m, "Cache-Control", "max-age=2592000");
					}
					//选择传输方式
					if (req_flen > RESPOND_CHUNK_ENCODING_THRESHOLD)
					{
						char chunk_buf[RESPOND_CHUNK_ENCODING_THRESHOLD+2];
						header_m = &temporary_header;
						dict_chain_apeend_ele(header_m, "Transfer-Encoding", "chunked");
						HTTP_respond(client, 200, header_m, 0, 0, cont_type);//内容必须置零，否则会制作并发送内容长header
						char chunk_size[64];
						while (feof(request_file) == 0)
						{
							//循环发送直到末尾发0
							u8 read_len = fread(chunk_buf, sizeof(char), RESPOND_CHUNK_ENCODING_THRESHOLD, request_file);
							memset(chunk_size, 0, 64);
							itoa(read_len, chunk_size, 16);
							u2 size_len = strlen(chunk_size);
							memcpy(chunk_size + size_len, "\r\n", 2);
							memcpy(chunk_buf + read_len, "\r\n", 2);
							send(client, chunk_size, size_len + 2, 0);
							send(client, chunk_buf, read_len + 2, 0);
						}
						send(client, "0\r\n\r\n", 5, 0);
					}
					else
					{
						char* req_cont = (char*)malloc(req_flen * sizeof(char));
						fread(req_cont, sizeof(char), req_flen, request_file);
						HTTP_respond(client, 200, header_m, req_cont, req_flen, cont_type);
						free(req_cont);
					}

					if (header_m)
						dict_chain_free(header_m);
				}
				else
				{
					HTTP_respond(client, 204, 0, 0, 0, 0);
				}
				fclose(request_file);
			}
		}
	}
	else if (memcmp(method, "POST", 4) == 0)
	{
		
		if (memcmp(path, "winc/upload/tasks_data.json",27) == 0)
		{
			if (content)
			{
				FILE* tasks_file;
				char* td_fn = (char*)malloc(root_path_len + 16);
				memcpy(td_fn, ROOT_PATH, root_path_len);
				memcpy(td_fn + root_path_len, "tasks_data.json", 16);
				errno_t fresult = fopen_s(&tasks_file, td_fn, "wb");
				free(td_fn);
				if (fresult)
				{
					printf("tasks_data.json打开失败:%d", fresult);
					HTTP_respond(client, 500, 0, 0, 0, 0);
				}
				else
				{
					fwrite(content, 1, cont_len, tasks_file);
					fclose(tasks_file);
					HTTP_respond(client, 200, 0, content, cont_len, "application/json");
				}
			}
			else
			{
				HTTP_respond(client, 403, 0, 0, 0, 0);
			}
		}
		else if (memcmp(path, "winc/cmd/run_d.slience", 22) == 0)
		{
			if (content)
			{
				char* cmp_cmd = (char*)calloc(cont_len, 1);
				UTF8toANSI(content, cmp_cmd);
				if (memcmp(cmp_cmd, "查看公开库", 18) == 0)
				{
					system("start https://gitee.com/");
				}
				else if (memcmp(cmp_cmd, "打开照片库", 10) == 0)
				{
					system("start E:\\winc\\picture\\majsoul");
				}
				free(cmp_cmd);
			}
			HTTP_respond(client, 204, 0, 0, 0, 0);
		}
		else//通用情况
		{
			HTTP_respond(client, 404, 0, 0, 0, 0);
		}
	}
	else
	{
		HTTP_respond(client, 501, 0, 0, 0, 0);
	}
	return keep_alive;
}


/*
* @brief HTTP连接处理函数 函数内不做closesocket操作
* @param client 客户套接字
* @param tid 线程id
* @return 如果是HTTP/1.1返回1 否则0
*/
u2 HTTP_connection_handle(const SOCKET client, const HANDLE tid)
{
	u2 keep_alive = 0;

	char recv_buf[RECV_BUFFER_SIZE + 1];
	memset(recv_buf, '\000', RECV_BUFFER_SIZE + 1);
	//int recv_time_out = 8000;
	//setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, (char*)&recv_time_out, sizeof(int));
	int recv_ret = recv(client, recv_buf, RECV_BUFFER_SIZE, 0);
	if (recv_ret == 0)
	{
		char rec[64];
		sprintf(rec, "线程%d:\t对方关闭连接，正在清理并退出\r\n", tid);
		print_and_record_with_time(rec);
		return 0;
	}
	if (recv_ret == SOCKET_ERROR)
	{
		char rec[64];
		sprintf(rec,"线程%d:\tHTTP接收错误: WSA_err_code %d \r\n", tid, WSAGetLastError());
		print_and_record_with_time(rec);
		return 0;
	}
	char* strpos = recv_buf;
	u4 paclen = 0, rest_len = recv_ret;

	//提取method

	char method[MAXLEN_HTTP_METHOD];
	for (paclen = 0; paclen < MAXLEN_HTTP_METHOD; paclen++)
	{
		if (strpos[paclen] == ' ')
		{
			memcpy(method, strpos, paclen);
			method[paclen] = '\0';
			break;
		}
	}
	if (paclen == MAXLEN_HTTP_METHOD)
	{
		char rec[64];
		sprintf(rec, "线程%d:\tHTTP 请求方法错误，已终结该连接\r\n", tid);
		print_and_record_with_time(rec);
		return 0;
	}
	strpos += paclen + 1;
	rest_len -= paclen + 1;

	//提取url

	char url[MAXLEN_HTTP_URL];
	u2 url_len=0, overload = 1;
	for (paclen = 0; paclen < MAXLEN_HTTP_URL; paclen++)
	{
		if (strpos[paclen] == ' ')
		{
			url[url_len] = '\0';
			overload = 0;
			break;
		}
		if (strpos[paclen] == '%')
		{
			url[url_len++] = hex2dec(strpos[paclen + 1]) * 16 + hex2dec(strpos[paclen + 2]);
			paclen += 2;
		}
		else
		{
			url[url_len++] = strpos[paclen];
		}
	}
	if (overload)
	{
		char rec[64];
		sprintf(rec, "线程%d:\tHTTP URL　长度超过阈值，已终结该连接\r\n", tid);
		print_and_record_with_time(rec);
		//414 url too large
		HTTP_respond(client, 414, 0, 0, 0, 0);

		return 0;
	}
	strpos += paclen + 1;
	rest_len -= paclen + 1;
	UTF8toANSI(url, url);
	//分析 url 提取 path query fragment
	char* path = 0, * query = 0, * fragment = 0;
	u2 stage = 0, clip_pos = 0;
	if (url_len == 1)
	{
		path = (char*)malloc(11 * sizeof(char));
		memcpy(path, "index.html", 11);
	}
	else
	{
		for (u2 num0 = 0;; num0++)
		{
			if (num0 == url_len)
			{
				if (stage)
				{
					u2 clip_len = url_len - clip_pos + 1;
					query = (char*)malloc(clip_len * sizeof(char));
					memcpy(query, url + clip_pos, clip_len);
				}
				else
				{
					path = (char*)malloc((url_len) * sizeof(char));
					memcpy(path, url + 1, url_len);
				}
				break;
			}
			if (url[num0] == '?')
			{
				path = (char*)malloc((num0) * sizeof(char));
				memcpy(path, url + 1, num0 - 1);
				path[num0 - 1] = 0;
				stage = 1;
				clip_pos = num0 + 1;
			}
			if (url[num0] == '#')
			{
				if (stage)
				{
					u2 clip_len = num0 - clip_pos;
					query = (char*)malloc((clip_len + 1) * sizeof(char));
					memcpy(query, url + clip_pos, clip_len);
					query[clip_len] = 0;
					clip_len = url_len - num0 + 1;
					fragment = (char*)malloc(clip_len * sizeof(char));
					memcpy(fragment, url + num0 + 1, clip_len);
					break;
				}
				else
				{
					path = (char*)malloc((num0) * sizeof(char));
					memcpy(path, url + 1, num0 - 1);
					path[num0 - 1] = 0;
					clip_pos = url_len - num0 + 2;
					fragment = (char*)malloc(clip_pos * sizeof(char));
					memcpy(fragment, url + num0 + 1, clip_pos);
					break;
				}
			}
		}
	}

	//提取 version

	char version[MAXLEN_HTTP_VERSION] = { 0 };
	for (paclen = 0; paclen < MAXLEN_HTTP_VERSION; paclen++)
	{
		if (strpos[paclen] == '\r')
		{
			memcpy(version, strpos, paclen);
			version[paclen] = '\0';
			break;
		}
	}

	if (memcmp(version, "HTTP/1.1", 8)==0)
	{
		keep_alive = 1;
	}
	else if (memcmp(version, "HTTP/1.0", 8))
	{
		char rec[64] = { 0 };
		sprintf(rec, "线程%d:\tHTTP version: |%s|　版本超前，无法支持，已关闭连接\r\n", tid, version);
		print_and_record_with_time(rec);

		if (path)
			free(path);
		if (query)
			free(query);
		if (fragment)
			free(fragment);

		return 0;
	}

	if (paclen == MAXLEN_HTTP_VERSION)
	{
		char rec[64];
		sprintf(rec, "线程%d:\tHTTP version　长度超过阈值，已终结该连接\r\n", tid);
		print_and_record_with_time(rec);
		HTTP_respond(client, 505, 0, 0, 0, 0);

		if (path)
			free(path);
		if (query)
			free(query);
		if (fragment)
			free(fragment);

		return 0;
	}
	strpos += paclen + 2;
	rest_len -= paclen + 2;
	
	//解析headers
	struct dict_chain_10u header_chain = { .len = 0, .next = 0 };
	struct dict_chain_10u* header_chain_handling = &header_chain;
	u2 header_page = 0, header_end = 0;
	for (;; header_page++)//每一页header
	{
		for (header_chain_handling->len = 0, header_chain_handling->next = 0; header_chain_handling->len < MAX_UNIT_PER_PAGE_IN_CHAIN; header_chain_handling->len++)//每个header
		{
			if (strpos[0] == '\r')//判断是否没有header
			{
				strpos += 2;
				rest_len -= 2;
				header_end = 1;
				break;
			}

			struct dict_ele* header_handling = &header_chain_handling->eles[header_chain_handling->len];//处理中的header


			for (paclen = 0; paclen < rest_len; paclen++)//取出header头部字名
			{
				if (strpos[paclen] == ':')
					break;
			}

			//分配并储存
			header_handling->name = (char*)malloc(paclen + 1);
			memcpy(header_handling->name, strpos, paclen);
			header_handling->name[paclen] = '\0';

			//更新参数
			strpos += paclen + 1;
			rest_len -= paclen + 1;

			//排除值空格
			if (strpos[0] == ' ')
			{
				strpos++;
				rest_len--;
			}

			for (paclen = 0; paclen < rest_len; paclen++)//取出header值
			{
				if (strpos[paclen] == '\r')
					break;
			}

			//分配并储存
			header_handling->value = (char*)malloc(paclen + 1);
			memcpy(header_handling->value, strpos, paclen);
			header_handling->value[paclen] = '\0';

			//更新参数
			strpos += paclen + 2;
			rest_len -= paclen + 2;

		}

		if (header_end || header_page == MAX_HTTP_HEADER_PAGE_NUM)
			break;

		header_chain_handling->next = (struct dict_chain_10u*)calloc(1,sizeof(struct dict_chain_10u));
		header_chain_handling = header_chain_handling->next;
	}


	//根据请求获取 正文内容
	u8 cont_len = 0;
	char* content = 0;
	if (rest_len)
	{
		//遍历获取内容长
		struct dict_chain_10u* header_page_handling = &header_chain;
		for (u2 num0 = 0; num0 < MAX_UNIT_PER_PAGE_IN_CHAIN * MAX_HTTP_HEADER_PAGE_NUM; num0++)//获取 conten length
		{
			u2 opt_num = num0 % MAX_UNIT_PER_PAGE_IN_CHAIN;
			if (opt_num == 0)
			{
				if (num0)
				{
					if (header_page_handling->next == 0)
						break;
					else
						header_page_handling = header_page_handling->next;
				}
			}
			if (opt_num == header_page_handling->len)
				break;
			if (memcmp(header_page_handling->eles[opt_num].name, "Content-Length", 14) == 0)
			{
				cont_len = atoi(header_page_handling->eles[opt_num].value);
			}
		}

		if (cont_len)
		{
			content = (char*)calloc(cont_len + 1, sizeof(char));
			memcpy(content, strpos, rest_len);
			if (cont_len <= RECV_MAX_CONTENT_LENGTH)
			{
				if (cont_len > rest_len)
				{
					u4 sec_r_len = cont_len - rest_len;
					char* rest_content = (char*)calloc(sec_r_len + 1, sizeof(char));
					memset(rest_content, 0, sec_r_len + 1);
					int sec_r = recv(client, rest_content, sec_r_len, 0);
					if (sec_r == 0)
					{
						char rec[64];
						sprintf(rec, "线程%d:\t对方关闭连接，正在清理并退出\r\n", tid);
						print_and_record_with_time(rec);
						if (path)
							free(path);
						if (query)
							free(query);
						if (fragment)
							free(fragment);
						if (content)
							free(content);
						dict_chain_free(&header_chain);

						return 0;
					}
					if (sec_r == SOCKET_ERROR)
					{
						char rec[64];
						sprintf(rec,"线程%d:\tHTTP接收错误: WSA_err_code %d \r\n", tid, WSAGetLastError());
						print_and_record_with_time(rec);

						if (path)
							free(path);
						if (query)
							free(query);
						if (fragment)
							free(fragment);
						if (content)
							free(content);
						dict_chain_free(&header_chain);

						return 0;
					}
					memcpy(content+ rest_len, rest_content, sec_r_len);
					free(rest_content);
				}
			}
			else
			{
				// 413 	Request Entity Too Large
				HTTP_respond(client, 413, 0, 0, 0, 0);

				if (path)
					free(path);
				if (query)
					free(query);
				if (fragment)
					free(fragment);
				if (content)
					free(content);
				dict_chain_free(&header_chain);

				return 0;
			}
		}
		else
		{
			// 411 Length Required
			HTTP_respond(client, 411, 0, 0, 0, 0);

			if (path)
				free(path);
			if (query)
				free(query);
			if (fragment)
				free(fragment);
			if (content)
				free(content);
			dict_chain_free(&header_chain);

			return keep_alive;
		}

	}

	char rec[64 + MAXLEN_HTTP_URL];
	sprintf(rec, "线程%d:\t方法:|%s|\turl:|%s|\r\n", tid, method, url);
	print_and_record_with_time(rec);


	//回应客户端
	keep_alive = reply_client(client, method, url, path, query, fragment, version, &header_chain, content, cont_len);



	//清理
	if (path)
		free(path);
	if (query)
		free(query);
	if (fragment)
		free(fragment);
	if (content)
		free(content);
	dict_chain_free(&header_chain);


	return keep_alive;//HTTP/1.1 默认保持tcp连接
}


DWORD WINAPI HTTP_connection_handle_thread(const LPVOID lparam)
{
	thread_total_num++;
	u2 hold_link = 1;
	char rec[64];
	struct param_u2_and_socket* lpparam = lparam;
	sprintf(rec, "线程%u:\t客户id:%u\t开始\r\n", lpparam->thread_id, lpparam->client_socket);
	print_and_record_with_time(rec);
	while (hold_link)
	{
		hold_link = HTTP_connection_handle(lpparam->client_socket, lpparam->thread_id);
	}
	closesocket(lpparam->client_socket);
	memset(rec, 0, 64);
	sprintf(rec, "线程%u:\t客户id:%u\t结束\r\n", lpparam->thread_id, lpparam->client_socket);
	print_and_record_with_time(rec);
	free(lpparam);
	thread_total_num--;
}

void HTTP_server_loop()
{
	//HWND hwnd1 = GetActiveWindow();
	//ShowWindow(hwnd1, SW_SHOW);
	WSADATA wsaData;
	SOCKET sListen, sAccept;         //服务器监听套接字，连接套接字
	u2 serverport = DEFAULT_PORT;   //服务器端口号
	//struct addrinfo server_, client_;
	struct sockaddr_in ser, cli;     //服务器地址，客户端地址
	int iLen;

	printf("HTTP_server_loop:\t核心服务初始化开始\n");


	//第一步：加载协议栈
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		printf("协议栈加载失败\n");
		return;
	}

	//第二步：创建监听套接字，用于监听客户请求
	sListen = socket(IP_PROTOCOL_IPv4_6, SOCK_STREAM, 0);
	if (sListen == INVALID_SOCKET)
	{
		printf("失败:socket() Failed:%d\n", WSAGetLastError());
		return;
	}

	//创建服务器地址：IP+端口号
	ser.sin_family = IP_PROTOCOL_IPv4_6;
	ser.sin_port = htons(serverport);               //服务器端口号
	ser.sin_addr.s_addr = htonl(INADDR_ANY);   //服务器IP地址，默认使用本机IP

	//第三步：绑定监听套接字和服务器地址
	if (bind(sListen, (LPSOCKADDR)&ser, sizeof(ser)) == SOCKET_ERROR)
	{
		printf("失败:blind() Failed:%d\n", WSAGetLastError());
		return;
	}

	//第五步：通过监听套接字进行监听
	if (listen(sListen, 5) == SOCKET_ERROR)
	{
		printf("失败:listen() Failed:%d\n", WSAGetLastError());
		return;
	}
	printf("HTTP_server_loop:\t准备完成,等待连接\n");

	cleanup = 1;


	while (1)   //循环等待客户的请求
	{
		//第六步：接受客户端的连接请求，返回与该客户建立的连接套接字
		iLen = sizeof(cli);
		sAccept = accept(sListen, (struct sockaddr*)&cli, &iLen);
		if (sAccept == INVALID_SOCKET)
		{
			printf("失败:accept() Failed:%d\n", WSAGetLastError());
			break;
		}
		else if (sAccept == WSAEINTR)
		{
			printf("accept函数被打断,核心 HTTP server 正在退出");
			break;
		}
		if (thread_total_num < MAX_CONNECT_NUMBER)
		{
			//第七步，创建线程接受浏览器请求
			struct param_u2_and_socket* lpparam = (struct param_u2_and_socket*)malloc(sizeof(struct param_u2_and_socket));
			lpparam->client_socket = sAccept;
			lpparam->thread_id = thread_id_cycle++;
			if (thread_id_cycle == MAX_THREAD_LOG_ID)
				thread_id_cycle = 0;
			CreateThread(0, 0, HTTP_connection_handle_thread, lpparam, 0, 0);
		}
		else
		{
			//如果已有连接超过规定数则返回503过载保护
			HTTP_respond(sAccept, 503, 0, 0, 0, 0);
			closesocket(sAccept);
		}
	}
	closesocket(sListen);
	WSACleanup();
	printf("HTTP_server_loop:\t核心服务函数退出成功\n");
	return 0;
}


void inline check_clean()
{
	WSACancelBlockingCall();
	if (server_loop_handle)
	{
		CloseHandle(server_loop_handle);
		server_loop_handle = 0;
	}
	if (cleanup)
	{
		WSACleanup();
		cleanup = 0;
	}

	dict_chain_free(&response_headers);
	dict_chain_free(&request_headers);

	if (index_content)
	{
		free(index_content);
		index_content = 0;
	}

	print_and_record_with_time("结束\r\n\r\n");
	Sleep(1000);//记录文件时常过早关闭导致部分线程来不及记录而报错
	if (record_file)
	{
		fclose(record_file);
		record_file = 0;
	}
}


BOOL WINAPI cshand(DWORD dwCtrlType)
{
	// 控制台将要被关闭
	if (CTRL_CLOSE_EVENT == dwCtrlType)
	{
		check_clean();
		printf("退出操作完成");
	}
	return FALSE;
}

void inline init_func()//只需且只能执行一次
{
	root_path_len = strlen(ROOT_PATH);
	SetConsoleCtrlHandler(cshand, 1);//设置关闭回调
	srand(time(0));//设置随机数种子

}

void inline start_func()
{
	//设置通用标头
	response_headers.next = 0;
	response_headers.eles[0].name = (char*)malloc(7);
	memcpy(response_headers.eles[0].name, "Server", 7);
	response_headers.eles[0].value = (char*)malloc(17);
	memcpy(response_headers.eles[0].value, "WJH's C-- server", 17);
	response_headers.eles[1].name = (char*)malloc(11);
	memcpy(response_headers.eles[1].name, "Connection", 11);
	response_headers.eles[1].value = (char*)malloc(11);
	memcpy(response_headers.eles[1].value, "keep-alive", 11);
	response_headers.len = 2;

	request_headers.next = 0;
	request_headers.eles[0].name = (char*)malloc(11);
	memcpy(request_headers.eles[0].name, "User-Agent", 11);
	request_headers.eles[0].value = (char*)malloc(130);
	memcpy(request_headers.eles[0].value, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/105.0.0.0 Safari/537.36 Edg/105.0.1343.27", 130);
	request_headers.eles[1].name = (char*)malloc(11);
	memcpy(request_headers.eles[1].name, "Connection", 11);
	request_headers.eles[1].value = (char*)malloc(6);
	memcpy(request_headers.eles[1].value, "close", 6);
	request_headers.eles[2].name = (char*)malloc(4);
	memcpy(request_headers.eles[2].name, "DNT", 4);
	request_headers.eles[2].value = (char*)malloc(3);
	memcpy(request_headers.eles[2].value, " 1", 3);
	request_headers.len = 3;

#ifdef RECORD_IN_FILE
	//打开记录文件
	int f_err = fopen_s(&record_file, RECORD_FILE_NAME, "ab");
	if (f_err)
	{
		printf("错误: record记录文件打开错误 : err_code: %d \n", f_err);
		return;
	}
#else
	record_file = 0;
	int f_err = 0;
#endif
	//记录时间
	print_and_record_with_time("开始\r\n");


	//预存index.html
	FILE* home_page_file;
	char* index_path = (char*)malloc(root_path_len + 11);
	memcpy(index_path, ROOT_PATH, root_path_len);
	memcpy(index_path + root_path_len, "index.html", 11);
	f_err = fopen_s(&home_page_file,index_path, "rb");
	if (f_err)
	{
		printf("错误: index.html主页文件打开错误 : err_code: %d \n", f_err);
		return;
	}
	free(index_path);

	fseek(home_page_file, 0, SEEK_END);
	index_len = ftell(home_page_file);
	fseek(home_page_file, 0, SEEK_SET);

	if (index_content)
	{
		free(index_content);
		index_content = 0;
	}

	index_content = (char*)malloc(index_len* sizeof(char));
	fread(index_content, sizeof(char), index_len, home_page_file);

	fclose(home_page_file);

	server_loop_handle = CreateThread(NULL, NULL, HTTP_server_loop, NULL, NULL, NULL);//开始核心服务函数
}

void reload_home_page()
{
	FILE* home_page_file;
	char* index_path = (char*)malloc(root_path_len + 11);
	memcpy(index_path, ROOT_PATH, root_path_len);
	memcpy(index_path + root_path_len, "index.html", 11);
	int f_err = fopen_s(&home_page_file, index_path, "rb");
	if (f_err)
	{
		printf("错误: index.html主页文件打开错误 : err_code: %d \n", f_err);
		return;
	}
	free(index_path);

	fseek(home_page_file, 0, SEEK_END);
	index_len = ftell(home_page_file);
	fseek(home_page_file, 0, SEEK_SET);

	if (index_content)
	{
		free(index_content);
		index_content = 0;
	}

	index_content = (char*)malloc(index_len * sizeof(char));
	fread(index_content, sizeof(char), index_len, home_page_file);

	fclose(home_page_file);
}

u2 handle_cmd(const char* cmd_text)
{
	if (memcmp(cmd_text, "quit", 4) == 0|| memcmp(cmd_text, "exit", 4) == 0)
	{
		check_clean();
		printf("退出操作完成\n");
		return 0;
	}
	else if (memcmp(cmd_text, "stop", 4) == 0)
	{
		printf("暂停操作完成\n");
		check_clean();
	}
	else if (memcmp(cmd_text, "continue", 8) == 0)
	{
		if (server_loop_handle == 0)
		{
			printf("继续操作完成");
			start_func();
		}
		else
		{
			printf("错误：核心服务线程句柄未正常清理");
		}
	}
	else if (memcmp(cmd_text, "reload", 6) == 0)
	{
		reload_home_page();
		printf("重载首页完成\n");
	}
	else
	{
		printf("没有匹配命令，支持的命令有: quit stop continue echo（最长59位）\n");
	}
	return 1;
}

int main()
{
	printf("\thello world \t __main__ line 1\n");
	init_func();
	printf("\n\t 初始化完成 \n\n");
	start_func();
	char cmd[64];
	u2 status = 1;
	while (status)
	{
		memset(cmd, '\000', 64);
		scanf("%s",cmd);
		status = handle_cmd(cmd);
	}
	if (cleanup)
	{
		WSACleanup();
		cleanup = 0;
	}
	check_clean();
	printf("main end");
	return 0;
}

/*
* debug version 19.48.329 | release version 3.2
*/