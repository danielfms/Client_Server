CC=g++ -std=c++11
#ZMQ=/home/utp/cs/zmq
ZMQ=/usr/local
ZMQ_LIBS=$(ZMQ)/lib
ZMQ_HDRS=$(ZMQ)/include


all: nodo tracker

nodo: nodo.cc
	$(CC) -I$(ZMQ_HDRS) -c nodo.cc
	$(CC) -L$(ZMQ_LIBS) -o nodo nodo.o -lzmq -lczmq -lpthread

tracker: tracker.cc
	$(CC) -I$(ZMQ_HDRS) -c tracker.cc
	$(CC) -L$(ZMQ_LIBS) -o tracker tracker.o -lzmq -lczmq -lpthread

clean:
	rm -rf nodo nodo.o tracker tracker.o  *~


#export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/home/utp/cs/zmq/lib
#export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/home/utp/zmq/lib
# LD_LIBRARY_PATH=/usr/local/lib
# export LD_LIBRARY_PATH=$LD_LIBRARY_PATH=/usr/local/lib
#killall -9 *
