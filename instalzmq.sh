#!/bin/bash
#sh scrip
wget http://download.zeromq.org/zeromq-4.0.5.tar.gz
tar xzvf zeromq-4.0.5.tar.gz
mkdir zmq
cd zeromq-4.0.5
./configure --prefix=/home/danielfms/zmq
make -j4 
make install
echo export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/home/danielfms/zmq/lib >> /home/danielfms/.bashrc 
source /home/danielfms/.bashrc
cd ..
wget http://download.zeromq.org/czmq-3.0.0-rc1.tar.gz
tar xzvf czmq-3.0.0-rc1.tar.gz
cd czmq-3.0.0
./configure --with-libzmq=/home/danielfms/zmq --prefix=/home/danielfms/zmq/
make install
