#ifndef __HTTP_COMMON_H
#define __HTTP_COMMON_H
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/signal.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <errno.h>

#define ERR_SOCKET  1
#define ERR_BIND    2
#define ERR_LISTEN  3
#define ERR_ACCEPT  4
#define ERR_HTTPREQ 5
#define ERR_HTTPMETHOD 6
#define ERR_HTTPVER 7
#define ERR_OTHER   -1

#ifndef NDEBUG
#define DEBUG(...) do{fprintf(stderr, "[D] %s: [%d]: ", __FILE__, __LINE__);fprintf(stderr, __VA_ARGS__);}while(0)
#else
#define DEBUG(...)
#endif

extern struct conf_opts * global_opts;

void error_handler(int num);

#endif