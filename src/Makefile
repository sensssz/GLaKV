CXXFLAGS = -std=c++11 -c -O3 -g
OBJS = DB.o glakv_client.o glakv_server.o exponential_distribution.o

all: exec

test: exp_dist_test

exec: $(OBJS)
	$(CC) DB.o glakv_server.o -o glakv_server
	$(CC) DB.o glakv_client.o exponential_distribution.o -o glakv_client

glakv_client.o: glakv_client.cc config.h
	$(CC) $(CXXFLAGS) glakv_client.cc -o glakv_client.o

glakv_server.o: glakv_server.cc config.h
	$(CC) $(CXXFLAGS) glakv_server.cc -o glakv_server.o

exp_dist_test: exp_dist_test.o exponential_distribution.o
	$(CC) $(LDFLAGS) exp_dist_test.o exponential_distribution.o -o exp_dist_test

exp_dist_test.o: exp_dist_test.cc
	$(CC) $(CXXFLAGS) exp_dist_test.cc -o exp_dist_test.o

exponential_distribution.o: exponential_distribution.cc exponential_distribution.h
	$(CC) $(CXXFLAGS) exponential_distribution.cc -o exponential_distribution.o

DB.o: DB.cc DB.h config.h
	$(CC) $(CXXFLAGS) DB.cc -o DB.o

clean:
	rm $(OBJS)
