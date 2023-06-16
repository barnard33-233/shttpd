#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/signal.h>

char ip[] = "127.0.0.1";
int port = 7889;

const char msg[32] = "Hi, it's nice to see you.";

int serv_sock;

// use these vars after fork:
FILE* file = NULL;
int clnt_sock_for_this_connection = 0;

void SIGINT_handler(int num){
    printf("exit...\n");
    // kill all child proc
    kill(0, SIGINT);
    // release
    shutdown(serv_sock, SHUT_RDWR);
    exit(0);
}

void connection_SIG_handler(int num){
    printf("close client sock %d...\n", clnt_sock_for_this_connection);
    fclose(file);
    close(clnt_sock_for_this_connection);
    exit(0);
}

void new_connection(int clnt_sock){
    clnt_sock_for_this_connection = clnt_sock;
    signal(SIGPIPE, connection_SIG_handler);
    signal(SIGINT, connection_SIG_handler);
    int msg_size = 0;
    char buffer[1024];

    file = fopen("./log.txt", "a+");

    while(1){
        msg_size =  read(clnt_sock, buffer, 1024);
        printf("%s, %d\n", buffer, msg_size);
        write(clnt_sock, &msg_size, sizeof(msg_size)); 
        fwrite(buffer, sizeof(char), msg_size, file);
    }
    // nerver reach here.
}

int main(){
    signal(SIGINT, SIGINT_handler);
    struct sockaddr_in serv_addr;

    serv_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(ip);
    serv_addr.sin_port = htons(port);

    if(bind(serv_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0){
        printf("bind err\n");
        return -1;
    }

    if(listen(serv_sock, 4) < 0){
        printf("listen err");
        return -1;
    }
    for(;;){
        int clnt_sock;
        struct sockaddr_in clnt_addr;
        socklen_t clnt_addr_size = sizeof(clnt_addr);
        clnt_sock = accept(serv_sock, (struct sockaddr*)& clnt_addr, &clnt_addr_size);
        pid_t pid = fork();
        if (pid == 0){
            new_connection(clnt_sock);
        }
    }
    return 0;
}
