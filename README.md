# tcp-client-server
Multithreading TCP client/server for file sharing

## Build

```
git clone git@github.com:kuksag/tcp-client-server.git
cd tcp-client-server
mkdir build
cd build 
cmake ..
make
```

## Run

### Client 

```
./client [-a address] [-p port] [-n result_name] filename
```
Where `address` and `port` specifies server, `result_name` name to be saved on server side, `filename` file to send

### Server 

```
./server [-p port] [-f folder]
```
Where `port` specifies which port is to listen, `folder` specifies where to save incoming files
