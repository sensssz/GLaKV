UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
    CUSTOM_USR = /home/jiamin/usr
    LIBS = -lboost_thread
endif
ifeq ($(UNAME_S),Darwin)
    CUSTOM_USR = /usr/local/Cellar/boost/1.60.0_2
    LIBS = -lboost_thread-mt -lboost_system
endif

SRC = .
INCLUDE = -I$(CUSTOM_USR)/include -I$(SRC)
LDFLAGS = -L$(CUSTOM_USR)/lib $(LIBS)
CXXFLAGS = -Wall -Werror -std=c++11 -c $(INCLUDE)
SERVEROBJS = DBImpl.o fakeDB.o lrucache.o worker_thread.o thread_pool.o glakv_server.o
CLIENTOBJS = glakv_client.o exponential_distribution.o
OBJS = $(SERVEROBJS) $(CLIENTOBJS)
RM = rm -rf

all: release

release: CXXFLAGS += -O3 -DNDEBUG
release: exec

debug: CXXFLAGS += -g
debug: exec

profile: CXXFLAGS += -pg
profile: LDFLAGS += -pg
profile: exec

test: exp_dist_test
	./exp_dist_test

exec: $(OBJS)
	$(CXX) $(LDFLAGS) $(SERVEROBJS) -o glakv_server
	$(CXX) $(LDFLAGS) $(CLIENTOBJS) -o glakv_client

glakv_client.o: glakv_client.cc config.h
	$(CXX) $(CXXFLAGS) glakv_client.cc -o glakv_client.o

glakv_server.o: glakv_server.cc config.h
	$(CXX) $(CXXFLAGS) glakv_server.cc -o glakv_server.o

exponential_distribution.o: exponential_distribution.cc exponential_distribution.h
	$(CXX) $(CXXFLAGS) exponential_distribution.cc -o exponential_distribution.o

DBImpl.o: DBImpl.cc DBImpl.h config.h
	$(CXX) $(CXXFLAGS) DBImpl.cc -o DBImpl.o

fakeDB.o: fakeDB.cc fakeDB.h config.h
	$(CXX) $(CXXFLAGS) fakeDB.cc -o fakeDB.o

lrucache.o: lrucache.cc lrucache.h config.h
	$(CXX) $(CXXFLAGS) lrucache.cc -o lrucache.o

worker_thread.o: worker_thread.cc worker_thread.h mpsc_queue.h
	$(CXX) $(CXXFLAGS) worker_thread.cc -o worker_thread.o

thread_pool.o: thread_pool.cc thread_pool.h
	$(CXX) $(CXXFLAGS) thread_pool.cc -o thread_pool.o

exp_dist_test: exp_dist_test.o exponential_distribution.o
	$(CXX) $(LDFLAGS) exp_dist_test.o exponential_distribution.o -o exp_dist_test

exp_dist_test.o: exp_dist_test.cc
	$(CXX) $(CXXFLAGS) exp_dist_test.cc -o exp_dist_test.o

clean:
	$(RM) $(OBJS)
	$(RM) gmon.out gprof_analysis
