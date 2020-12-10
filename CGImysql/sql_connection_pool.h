#ifndef SQL_CONNECTION_POOL
#define SQL_CONNECTION_POOL

#include <list>
#include <mysql/mysql.h>
#include <iostream>
#include <error.h>
#include <stdio.h>
#include "../lock/locker.h"
#include "../log/log.h"
#include <string>
#include <string.h>

using std::string;

class connection_pool
{
public:
	MYSQL *GetConnection();			// 获取数据库连接
	bool ReleaseConnection(MYSQL *conn);	// 释放连接
	int GetFreeConn();			// 获取连接数
	void DestroyPool();			// 销毁所有连接

	// 单例模式
	static connection_pool *GetInstance();

	void init(string url, string User, 
		  string Password, string DatabaseName, 
		  int Port, int MaxConn, int close_log);

private:
	connection_pool();
	~connection_pool();

	int m_Maxconn;	// 最大连接数
	int m_CurConn;	// 当前已使用的连接数
	int m_FreeConn;	// 当前空闲的连接数
	locker lock;
	list<MYSQL *> connList;	// 连接池
	sem reserve;

public:
	string m_url;
	string m_Port;
	string m_User;
	string m_Password;
	string m_DatabaseName;
	int m_close_log;
};


class connectionRAII{

public:
	connectionRAII(MYSQL **con, connection_pool *connPool);
	~connectionRAII();
	
private:
	MYSQL *conRAII;
	connection_pool *poolRAII;
};

#endif
