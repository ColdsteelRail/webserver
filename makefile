CXX ?= g++

DEBUG ?= 1

ifeq ($(DEBUG), 1)
    CXXFLAGS += -g
else
    CXXFLAGS += -O2

endif

server: ./webserver/main.cpp ./webserver/library/lst_timer.cpp ./webserver/http_conn.cpp ./webserver/library/log.cpp ./webserver/library/sql_connection_pool.cpp ./webserver/webserver.cpp ./webserver/config.cpp
	$(CXX) -o server $^ $(CXXFLAGS) -lpthread -lmysqlclient

clean:
		rm -r server
