#include "common.h"
#include "conf.h"
#include "http.h"

#include <pthread.h>

struct conf_opts * global_opts;
int global_sock = 0;

void handle_exit(int num){
    // release resources when exit
    printf("[I] Exit");
    close(global_sock);
    exit(0);
}

// DEBUG:
#ifndef NDEBUG
void handle_sigsegv(int num){
    printf("[E] SIGSEGV");
    close(global_sock);
    exit(-1);
}
#endif

/*
 * handel various errors.
 */
void error_handler(int num){
    switch(num){
        case ERR_SOCKET:
            printf("[E] `socket` fail\n");
            close(global_sock);
            exit(-1);

        case ERR_BIND:
            printf("[E]: `bind` fail\n");
            close(global_sock);
            exit(-1);
        case ERR_LISTEN:
            printf("[E]: `listen` fail\n");
            close(global_sock);
            exit(-1);
        case ERR_THREAD:
            printf("[E] fail to create a new thread\n");
            break;
        default:
            printf("[E] unknown fail\n");
            close(global_sock);
            exit(-1);
    }
}

/*
 * create a socket, start linstening.
*/
int setup_server(struct conf_opts * opts){
    int http_sock;
    struct sockaddr_in name = {0};
    int reuse = 0;

    http_sock = socket(AF_INET, SOCK_STREAM, 0);
    if(http_sock < 0){
        error_handler(ERR_SOCKET);
    }
    setsockopt(http_sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    name.sin_family = AF_INET;
    name.sin_port = htons(opts->ListenPort);
    name.sin_addr.s_addr = htonl(INADDR_ANY);
    if(bind(http_sock, (struct sockaddr *)&name, sizeof(struct sockaddr)) < 0){
        error_handler(ERR_BIND);
    }

    if (listen(http_sock, opts->MaxClient) < 0){
        error_handler(ERR_LISTEN);
    }
    return http_sock;
}

int main_loop(int http_sock, struct conf_opts * opts){
    int client_sock;
    struct sockaddr_in client_name = {0};
    socklen_t socklen = sizeof(struct sockaddr);
    pthread_t thread;
    pthread_attr_t attr;
    while(1){
        client_sock = accept(http_sock, (struct sockaddr *)&client_name, &socklen);
        DEBUG("main_loop start accept\n");
        if(client_sock < 0){
            error_handler(ERR_ACCEPT);
        }
        
        // create thread
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

        if(pthread_create(&thread, &attr, handle_request, (void*)&client_sock)){
            err_500_internal_server_error(client_sock);
            close(client_sock);
            error_handler(ERR_THREAD);
        }
    }
}

int main(int argc, char ** argv){
    struct conf_opts opts;
    int http_sock = 0;

    printf("[I] Server Startup\n");
    parse_conf(&opts, argc, argv);
    if(opts.help == 1){
        printf("%s", help_info);
    }
    print_conf(&opts);
    global_opts = &opts;
    signal(SIGINT, handle_exit);
    signal(SIGQUIT, handle_exit);
    signal(SIGKILL, handle_exit);
#ifndef NDEBUG
    signal(SIGSEGV, handle_sigsegv);
#endif

    // setup
    http_sock = setup_server(&opts);
    global_sock = http_sock;
    // mainloop
    main_loop(http_sock, &opts);


    return 0;
}
