## COMP-8567: Client Server Project

---

### Introduction

Client can can request a file or set of files from server. The server searches for the files in its ***file directory rooted at its ~*** and returns temp.tar.gz of the files requested tot the client, Multiple clients can connect to the server from different machines and can request files as per following commands.

***The server, the mirror and the client processes must run on different machines and
must communicate using sockets only***

## Compiling the project
Run the following code to compile the project
```
 gcc -o server server.c
 gcc -o client client.c
```
This will output executable binary files ```server and client.```

 ### Running the project
 You can run the project by ```./server <port>``` eg

```
./server 9001

```
For client ```./server <IP Address> <port>```
```
./client localhost 9001
```
For multiple machines  Find IP adress of your WLAN and put on the example, 
```
./client 172.17.0.1 9001
```
### Functions

```int main(int argc, char* argv[])``` Initializes socket,waits and accepts connections

```void serviceClient(int sd)``` Handles messaging between client and server

```int processRequest(char msg[], int sd)``` Processes and tokenizes messages from client into command

```void compress_files()``` Compresses files requested by the client
```send_file()``` Sends file to the client

This two functions handles the acknowledgment messages between the server and client
```
bool getAck(int sd)
void sendAck(int sd)
```

### functions that handles client Commands as explained in the document
```
void findFile()
void sgetFiles()
void dgetFiles()
void getFiles()
void getTargz()
```
>```findfile filename``` 
If the file filename is found in its file directory tree rooted at ~, the server must return the ```filename, size(in bytes)```, and date created to the client and the client prints the received information on its terminal. Example ```findfile sample.txt```

>```sgetfiles size1 size2 <-u>```
The server must return to the client temp.tar.gz that contains all the files in the directory tree rooted at its ~ whose file-size in bytes is ```>=size1 and <=size2  size1 < = size2 (size1>= 0 and size2>=0)```  and ```-u unzip temp.tar.gz``` in the pwd of the client. Example ```sgetfiles 0 4096 -u```

>```dgetfiles date1 date2 <-u>```
The server must return to the client temp.tar.gz that contains all the files in the directory tree rooted at ~ whose date of creation is >=date1 and <=date2 (date1<=date2) ```-u unzip temp.tar.gz``` in the pwd of the client Example: ```dgetfiles 2023-01-16 2023-04-16 -u```

>```getfiles file1 file2 file3 file4 file5 file6 <-u >```
The server must search the files (file 1 ..up to file6) in its directory tree rooted at ~ and return temp.tar.gz that contains at least one (or more of the listed files) if they are present.If none of the files are present, the server sends “No file found” to the client (which is then printed on the client terminal by the client) ```-u unzip temp.tar.gz``` in the pwd of the client Example: ```getfiles new.txt ex1.c ex4.pdf```

>```gettargz <extension list> <-u>``` //up to 6 different file types
The server must return temp.tar.gz that contains all the files in its directory tree rooted at ***~ belonging to the file type/s listed*** in the extension list, else the server sends the message “No file found” to the client (which is printed on the client terminal by the client) ```-u unzip temp.tar.gz``` in the pwd of client The extension list must have at least one file type and can have up to six different file types Example: ```gettargz c txt pdf>```

>```quit ```
The command is transferred to the server and the client process is terminated


### How unzip functions are handled in the program

The server recieves the command and tokenizes if it has ```-u``` token it set flag. This flag will be used to send unzip message after files are send to client

```
if (unzip == true) {
                write(sd, unzipAction, sizeof(unzipAction));
                printf("Unzip the files\n");
                getAck(sd);
            }
```

## Finding IP Adddress for your machine
---
### Linux
Run the following command  to list network devices
```
ip address
```
This will ip addresses of network devices on your machine. For WiFi it will be wlan0 or wlp3s0. Choose network device that is connected to LAN where the server runs. You will put this ip address when running client program.