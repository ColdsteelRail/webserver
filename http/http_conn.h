#ifndef HTTPCONNECTION_H
#define HTTPCONNECTION_H
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/uio.h>
#include <map>

#include "../lock/locker.h"
#include "../CGImysql/sql_connection_pool.h"
#include "../timer/lst_timer.h"
#include "../log/log.h"

class http_conn
{
public:
	static const int FILENAME_LEN = 200;
	static const int READ_BUFFER_SIZE = 2048;
	static const int WRITE_BUFFER_SIZE = 1024;

	// HTTP报文请求方法
	enum METHOD {
		GET = 0,
		POST,
		HEAD,
		PUT,
		DELETE,
		TRACE,
		OPTIONS,
		CONNECT,
		PATH
	};
	// 主状态机的状态
	enum CHECK_STATE {
		CHECK_STATE_REQUESTIONLINE = 0,
		CHECK_STATE_HEADER,
		CHECK_STATE_CONTENT
	};
	// 从状态机的状态
	enum LINE_STATUS {
		LINE_OK = 0,
		LINE_BAD,
		LINE_OPEN
	};
	// 报文解析结果
	enum HTTP_CODE {
		NO_REQUEST,
		GET_REQUEST,
		BAD_REQUEST,
		NO_RESOURCE,
		FORBIDDEN_REQUEST,
		FILE_REQUEST,
		INTERNAL_REQUEST,
		CLOSED_CONNECTION
	};
public:
	http_conn();
	~http_conn();

public:
	void init(int sockfd, ocnst sockaddr_in &addr, char *, int, int, string user, string passwd, string sqlname);
        void close_conn(bool real_close = true);
        void process();
        // 读取浏览器发来的全部数据
        bool read_once();
        // 相应报文写入函数
        bool write();
        sockadddr_in *get_address()
        {
                return &m_address;
        }
        // 同步线程初始化数据库读取表
        void initmysql_result(connection_pool *connPool);
        int timer_flag;
        int improv;

private:
        void init();
        // 从m_read_buf读取，并处理请求报文
        HTTP_CODE process_read();
        // 向m_write_buf写入响应报文数据
        bool process_write(HTTP_CODE ret);
        // 主状态机解析报文中的请求行
        HTTP_CODE parse_request_line(char *text);
        // 主状态机解析报文中的请求数据
        HTTP_CODE parse_headers(char *text);
        // 主状态机解析报文中的请求内容
        HTTP_CODE parse_content(char *text);
        // 生成响应报文
        HTTP_CODE do_request();

        // m_start_line是已解析的字符数
        // get_line用于将指针向后便宜，指向未处理的字符
        char *get_line() { return m_read_buf + m_start_line; };
        
        // 从状态机读取一行，分析是请求报文的那一部分
        LINE_STATUS parse_line();

        void unmap();
        
        // 根据响应报文格式，生成8个部分，由do_request调用
        bool add_response(const char *format, ...);
        bool add_content(const char *content);
        bool add_status_line(int status, const char *title);
        bool add_headers(int content_length);
        bool add_content_type();
        bool add_content_length(int content_length);
        bool add_linger();
        bool add_blank_line();

// 类公有数据
public:
	static int m_epollfd; 	// 内核文件描述符集合
	static int m_user_count;
	MYSQL *mmysql;	// 数据库连接
	int m_state; 	// 读为0， 写为1

// 私有数据
private:
	int m_sockfd;
	sockaddr_in m_address;
	// 存储读取的请求报文数据
	char m_read_buf[READ_BUFFER_SIZE];
	// m_read_buf中数据的最后一个字节的下一位置
	int m_read_idx;
	// m_read_buf中读取的位置
	int checked_idx;
	// m_read_buf中已经解析的字符个数
	int m_start_line;

	// 存储发出的相应报文
	char m_write_buf[WRITE_BUFFER_SIZE];
	// m_write_buf中的长度
	int m_write_idx;

	// 主状态机的状态
	CHECK_STATE m_check_state;
	// 请求方法
	METHOD m_method;

	// 以下为请求报文中的6个变量
	char m_real_file[FILENAME_LEN];
	char *m_url;
	char *m_version;
	char *host;
	int m_content_length;
	bool m_linger;
	char *m_file_address;

    char *m_file_address;       // 读取服务器上的文件地址
    struct stat m_file_stat;
    struct iovec m_iv[2];       // io向量机制iovec
    int m_iv_count;
    int cgi;                    // 是否启用的POST
    char *m_string;             // 存储请求头数据
    int byte_to_send;           // 剩余发送字节数
    int bytes_have_send;        // 以发送的字节数
};

#endif