#define _GNU_SOURCE

#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h> // read(), write(), close()
#include <signal.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <syslog.h>
#include <fcntl.h>
#include <string.h>

#define MAX_READ_SIZE 1000
#define PORT "9000"
#define BACKLOG 5
#define SA struct sockaddr
#define TEMP_FILE "/var/tmp/aesdsocketdata"


struct sockaddr client_addr;
char* globalBuffer = NULL;
unsigned int globalBufferSize = 0;
int sockFd = 0;
int connFd = 0;
int fileFd = -1;

void handle_shutdown(int exit_code) {
    if(sockFd > 0){
        shutdown(connFd, SHUT_RDWR);
        close(sockFd);
    }
    if (connFd > 0){
        shutdown(connFd, SHUT_RDWR);
        close(connFd);
    }
    if(globalBuffer != NULL){
        free(globalBuffer);
        globalBuffer = NULL;
    }
    unlink(TEMP_FILE);

    exit(exit_code);
}

void sendDataToClient() {

    //read msgs from file and send to client
    int bytes_read;

    if(globalBuffer != NULL){
        free(globalBuffer);
        globalBuffer = NULL;
    }

    globalBuffer = (char*) malloc(MAX_READ_SIZE);
    if(globalBuffer == NULL){
        perror("malloc");
        handle_shutdown(EXIT_FAILURE);
    }
    bzero(globalBuffer, MAX_READ_SIZE);
    globalBufferSize = MAX_READ_SIZE;



    if (lseek(fileFd, 0, SEEK_SET)) {
        perror("lseek");
        handle_shutdown(EXIT_FAILURE);
    }
    while ((bytes_read = read(fileFd, globalBuffer, globalBufferSize))) {
        if (-1 == bytes_read) {
            perror("read");
            handle_shutdown(EXIT_FAILURE);
        }
        printf("bytes_read = %d {%.*s}\n",bytes_read, bytes_read, globalBuffer);
        // Ignore partial writes.  Shouldn't happen...
        if (write(connFd, globalBuffer, bytes_read) != bytes_read) {
            perror("write");
            handle_shutdown(EXIT_FAILURE);
        }
    }

    if(globalBuffer != NULL){
        free(globalBuffer);
        globalBuffer = NULL;
    }
}

void handleClientMsgs() {

    if(globalBuffer != NULL){
        free(globalBuffer);
        globalBuffer = NULL;
    }

    globalBuffer = (char*) malloc(MAX_READ_SIZE);
    if(globalBuffer == NULL){
        perror("malloc");
        handle_shutdown(EXIT_FAILURE);
    }
    bzero(globalBuffer, MAX_READ_SIZE);
    globalBufferSize = MAX_READ_SIZE;



    for (;;) {

        int receivedBytes = 0;

        // read the message from client and copy it in buffer
        printf("connFd: %d\n", connFd);
        receivedBytes = (int) recv(connFd, globalBuffer + globalBufferSize - MAX_READ_SIZE, MAX_READ_SIZE, 0);
        if (receivedBytes == -1) {
            perror("recv");
            handle_shutdown(EXIT_FAILURE);
        } else if (0 == receivedBytes) {
            //connection closed by client
            printf("Connection closed from client\n");
            shutdown(connFd, SHUT_RDWR);
            close(connFd);
            connFd = 0;

            free(globalBuffer);
            globalBuffer = NULL;

            struct sockaddr_in* client = (struct sockaddr_in*)&client_addr;
            syslog(LOG_DEBUG, "Closed connection from %s",
                   inet_ntoa(client->sin_addr));
            break;
        } else if(receivedBytes == MAX_READ_SIZE) {
            //Process steam of bytes to and store msgs
            globalBufferSize = globalBufferSize + MAX_READ_SIZE;
            globalBuffer = realloc(globalBuffer, globalBufferSize);
            if(globalBuffer == NULL){
                perror("realloc");
                handle_shutdown(EXIT_FAILURE);
            }

        } else {
            //No more data available on stream
            globalBufferSize = globalBufferSize - MAX_READ_SIZE + receivedBytes;
            globalBuffer[globalBufferSize] = 0;

            if (-1 == write(fileFd, globalBuffer, globalBufferSize)) {
                perror("write");
                handle_shutdown(EXIT_FAILURE);
            }

            sendDataToClient();
        }
    }
}



void sig_handler(int signum){

    //Return type of the handler function should be void
    printf("\nSignal Caught: %d\n", signum);
    printf("\nShutting Down\n");
    syslog(LOG_DEBUG,"Caught signal, exiting");

    handle_shutdown(EXIT_SUCCESS);
}


// Driver function
int main(int argc, char** argv)
{

    struct addrinfo hints, *servinfo;

    int rv;
    socklen_t client_addr_len;
    int enableDaemon = 0;
    int sock_opts;
    pid_t process_id = 0;

    fileFd= open(TEMP_FILE, O_CREAT | O_RDWR | O_APPEND);

    if(fileFd == -1){
        perror("open");
        handle_shutdown(EXIT_FAILURE);
    }

    sockFd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockFd == -1) {
        printf("socket creation failed...\n");
        return -1;
    }
    else {
        printf("Socket successfully created..\n");
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return -1;
    }

    sock_opts = 1;
    if (-1 == setsockopt(sockFd, SOL_SOCKET, SO_REUSEADDR, &sock_opts,
                         sizeof(sock_opts))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    // Binding socket
    if ((bind(sockFd, servinfo->ai_addr, servinfo->ai_addrlen)) != 0) {
        printf("socket bind failed...\n");
        return -1;
    }
    else {
        printf("socket bind successful..\n");
    }
    freeaddrinfo(servinfo);

    // Start listening
    if ((listen(sockFd, BACKLOG)) != 0) {
        printf("Listen failed...\n");
        exit(1);
    }
    else {
        printf("Server listening..\n");
    }

    //register signal
    signal(SIGINT,sig_handler);
    signal(SIGTERM,sig_handler);

    if(argc ==2){
        if(0 == strncmp(argv[1], "-d", strlen(argv[1]))){
            enableDaemon = 1;
        }
    }
    if(enableDaemon == 1){
        // Create child process
        process_id = fork();

        if (process_id < 0)
        {
            printf("fork failed!\n");
            exit(1);
        }

        if (process_id > 0)
        {
            //This parent process
            printf("process_id of child process %d \n", process_id);
            exit(0);
        }
    }


    while (1){
        // Accept connection from client
        client_addr_len = sizeof(client_addr);
        connFd = accept(sockFd, &client_addr, &client_addr_len);
        if (connFd < 0) {
            printf("server accept failed...\n");
            handle_shutdown(EXIT_FAILURE);
        }
        else {
            struct sockaddr_in* client = (struct sockaddr_in*)&client_addr;
            printf("Accepted connection from %s\n", inet_ntoa(client->sin_addr));
            syslog(LOG_DEBUG,"Accepted connection from %s", inet_ntoa(client->sin_addr));
        }
        // Function to handle Client Messages
        handleClientMsgs();
    }


    return 0;
}

