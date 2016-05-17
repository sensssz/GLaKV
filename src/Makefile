CUSTOM_USR = /home/jiamin/usr
SRC = .
INCLUDE = -I$(CUSTOM_USR)/include -I$(SRC)
LIBS = -lboost_thread
LDFLAGS = -pg -L$(CUSTOM_USR)/lib $(LIBS)
CXXFLAGS = -Wall -Werror -std=c++11 -c -O3 $(INCLUDE)
OBJS = DB.o glakv_client.o glakv_server.o exponential_distribution.o worker_thread.o thread_pool.o

all: exec

debug: CXXFLAGS += -pg
debug: exec

test: exp_dist_test

exec: $(OBJS)
	$(CC) $(LDFLAGS) DB.o glakv_server.o worker_thread.o thread_pool.o -o glakv_server
	$(CC) $(LDFLAGS) DB.o glakv_client.o exponential_distribution.o -o glakv_client

glakv_client.o: glakv_client.cc config.h
	$(CC) $(CXXFLAGS) glakv_client.cc -o glakv_client.o

glakv_server.o: glakv_server.cc config.h
	$(CC) $(CXXFLAGS) glakv_server.cc -o glakv_server.o

exponential_distribution.o: exponential_distribution.cc exponential_distribution.h
	$(CC) $(CXXFLAGS) exponential_distribution.cc -o exponential_distribution.o

DB.o: DB.cc DB.h config.h
	$(CC) $(CXXFLAGS) DB.cc -o DB.o

worker_thread.o: worker_thread.cc worker_thread.h
	$(CC) $(CXXFLAGS) worker_thread.cc -o worker_thread.o

thread_pool.o: thread_pool.cc thread_pool.h
	$(CC) $(CXXFLAGS) thread_pool.cc -o thread_pool.o

exp_dist_test: exp_dist_test.o exponential_distribution.o
	$(CC) $(LDFLAGS) exp_dist_test.o exponential_distribution.o -o exp_dist_test

exp_dist_test.o: exp_dist_test.cc
	$(CC) $(CXXFLAGS) exp_dist_test.cc -o exp_dist_test.o

clean:
	rm $(OBJS)
