#ifndef LST_TIMER
#define LST_TIMER

#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/socket.h>

#include "../log/log.h"
#include <time.h>

class util_timer;

// 连接资源结构体
struct client_data
{
	sockaddr_in address;
	int sockfd;
	util_timer *timer;
};

class util_timer
{
public:
	util_timer() : prev(NULL), next(NULL) {}

public:
	// 超时回调函数
	void (* cb_func)(client_data *);
	// 超时时间
	time_t expire;
	// 连接资源
	client_data *user_data;
	
	// 双向链表
	util_timer *prev;
	util_timer *next;
};

class  sort_timer_lst
{
public:
	sort_timer_lst();
	~sort_timer_lst();

	void add_timer(util_timer *timer);
	void adjust_timer(util_timer *timer);
	void del_timer(util_timer *timer);
	void tick();

private:
	void add_timer(util_timer *timer, util_timer *lst_head);
	util_timer *head;
	util_timer *tail;
};

class Utils
{
public:
	Utils() {}
	~Utils() {}

	void init(int timeslot);
	// 对文件设置非阻塞
	int setnonblocking(int fd);
	
	// 内核事件表注册读事件，ET模式，选择开启EPOLLONESHOT
	void addfd(int epollfd, int fd, bool one_shot, int TRIGMode);

	// 信号处理函数
	static void sig_handler(int sig);

	// 设置信号函数
	void addsig(int sig, void(handler)(int), bool reset = true);

	// 定时处理任务，重新定时以不断触发SIGALARM信号
	void timer_handler();

	void show_error(int connfd, const char *info);
	
public:
	static int *u_pipefd;
	sort_timer_lst m_timer_lst;
	static int u_epollfd;
	int m_TIMESLOT;
};

void cb_func(client_data *user_data);

#endif