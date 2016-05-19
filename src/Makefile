CUSTOM_USR = /home/jiamin/usr
SRC = .
INCLUDE = -I$(CUSTOM_USR)/include -I$(SRC)
LIBS = -lboost_thread
LDFLAGS = -L$(CUSTOM_USR)/lib $(LIBS)
CXXFLAGS = -Wall -Werror -std=c++11 -c -O3 $(INCLUDE)
SERVEROBJS = DBImpl.o fakeDB.o lrucache.o worker_thread.o thread_pool.o glakv_server.o
CLIENTOBJS = glakv_client.o exponential_distribution.o
OBJS = $(SERVEROBJS) $(CLIENTOBJS)

all: exec

debug: CXXFLAGS += -g
debug: LDFLAGS += -g
debug: exec

profile: CXXFLAGS += -pg
profile: LDFLAGS += -pg
profile: exec

test: exp_dist_test

exec: $(OBJS)
	$(CC) $(LDFLAGS) $(SERVEROBJS) -o glakv_server
	$(CC) $(LDFLAGS) $(CLIENTOBJS) -o glakv_client

glakv_client.o: glakv_client.cc config.h
	$(CC) $(CXXFLAGS) glakv_client.cc -o glakv_client.o

glakv_server.o: glakv_server.cc config.h
	$(CC) $(CXXFLAGS) glakv_server.cc -o glakv_server.o

exponential_distribution.o: exponential_distribution.cc exponential_distribution.h
	$(CC) $(CXXFLAGS) exponential_distribution.cc -o exponential_distribution.o

DBImpl.o: DBImpl.cc DBImpl.h config.h
	$(CC) $(CXXFLAGS) DBImpl.cc -o DBImpl.o

fakeDB.o: fakeDB.cc fakeDB.h config.h
	$(CC) $(CXXFLAGS) fakeDB.cc -o fakeDB.o

lrucache.o: lrucache.cc lrucache.h config.h
	$(CC) $(CXXFLAGS) lrucache.cc -o lrucache.o

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
