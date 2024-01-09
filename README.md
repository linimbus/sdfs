# sdfs v0.1
Simple distributed file system </br>
Dependent dev-fuse 2.9.2 & portmap service

## v0.1 Explain

### Features

- Support the standard POSIX interface
- Support point to point remote file operations

### Scheme
- Client takes the Posix interface requst by fuse, send requst packages on RPC ways through UDP protocol to the server terminal;
- Server handler the RPC request and execute the local POSIX interface; The execution result is answered through the UDP protocol to the client .

### Programme
- Server cluster scheme (master slave replication, file partitioning, service monitoring)

## Development guidance

### build binary file
make

### run server 
./sdfs_server [--debug]

### run client
./sdfs_client -o fsname=demo /mnt/test [-d]

### check mount dir
cd /mnt/test

### run some bench mark test
./benchmarktest.sh

### some bench mark test result

10000000 bytes (10 MB, 9.5 MiB) copied, 2.65307 s, 3.8 MB/s

4000000 bytes (4.0 MB, 3.8 MiB) copied, 0.431267 s, 9.3 MB/s

100000000 bytes (100 MB, 95 MiB) copied, 5.92279 s, 16.9 MB/s

40000000 bytes (40 MB, 38 MiB) copied, 2.48194 s, 16.1 MB/s

10000000 bytes (10 MB, 9.5 MiB) copied, 1.07627 s, 9.3 MB/s

10000000 bytes (10 MB, 9.5 MiB) copied, 0.34185 s, 29.3 MB/s

4000000 bytes (4.0 MB, 3.8 MiB) copied, 0.20391 s, 19.6 MB/s

100000000 bytes (100 MB, 95 MiB) copied, 5.34444 s, 18.7 MB/s

40000000 bytes (40 MB, 38 MiB) copied, 2.1679 s, 18.5 MB/s

10000000 bytes (10 MB, 9.5 MiB) copied, 0.610137 s, 16.4 MB/s
