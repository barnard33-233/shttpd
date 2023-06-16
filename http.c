#include "conf.h"
#include "common.h"
#include "http.h"

#include <strings.h>
#include <fcntl.h>

const char * err_html = "<!DOCTYPE HTML><html><body><h>%s</h><p>%s</p></body></html>"ENDL;
const char * server_info = "Server: shttpd/0.1"ENDL;
const char * server_info_val = "shttpd/0.1";

int readline(int fd, char * buffer, int size){
    char this = 0, last = 0;
    for(int i = 0; i < size - 1; i++){
        read(fd, &this, 1);
        if(this == '\n'){
            // end
            if(last != '\r'){
                error_handler(ERR_HTTPREQ);
            }
            else{
                buffer[i - 1] = '\0';
                if(i - 1 > 0){
                    return 0;
                }
                else{
                    return -1; // empty line
                }
            }
        }
        last = this;
        buffer[i] = this;
    }
    buffer[size - 1] = '\0';
    error_handler(ERR_HTTPREQ);
}

int set_nonblock(int fd){
    int flag = 0;
    flag = fcntl(fd, F_GETFL);
    if(flag == -1){
        return -1;
    }
    
    flag |= O_NONBLOCK;

    if(fcntl(fd, F_SETFL, flag) == -1){
        return -1;
    }

    return 0;
}

int set_block(int fd){
    int flag = 0;
    flag = fcntl(fd, F_GETFL);
    if(flag == -1){
        return -1;
    }
    
    flag &= ~O_NONBLOCK;

    if(fcntl(fd, F_SETFL, flag) == -1){
        return -1;
    }

    return 0;
}

/* implementation of each response type*/

void err_400_bad_request(int sock){
    char buffer[ERR_BUF_SIZE];

    // response line
    sprintf(buffer, "HTTP/1.1 400 Bad Request"ENDL);
    write(sock, buffer, strlen(buffer));
    
    // response header
    sprintf(buffer, "%s", server_info);
    write(sock, buffer, strlen(buffer));
    sprintf(buffer, "Content-Type: text/html"ENDL);
    write(sock, buffer, strlen(buffer));
    sprintf(buffer, ENDL);
    write(sock, buffer, strlen(buffer));

    // entity    
    // sprintf(buffer, err_html, "404", "Bad Request");
    // write(sock, buffer, strlen(buffer));
}

void err_404_not_found(int sock){
    char buffer[ERR_BUF_SIZE];

    // response line
    sprintf(buffer, "HTTP/1.1 404 Not Found"ENDL);
    write(sock, buffer, strlen(buffer));
    
    // response header
    sprintf(buffer, "%s", server_info);
    write(sock, buffer, strlen(buffer));
    sprintf(buffer, "Content-Type: text/html"ENDL);
    write(sock, buffer, strlen(buffer));
    sprintf(buffer, ENDL);
    write(sock, buffer, strlen(buffer));

    // entity    
    sprintf(buffer, err_html, "404", "Not Found");
    write(sock, buffer, strlen(buffer));
}

void err_500_internal_server_error(int sock){
    char buffer[ERR_BUF_SIZE];

    // response line
    sprintf(buffer, "HTTP/1.1 500 Internal Server Error"ENDL);
    write(sock, buffer, strlen(buffer));
    
    // response header
    sprintf(buffer, "%s", server_info);
    write(sock, buffer, strlen(buffer));
    sprintf(buffer, "Content-Type: text/html"ENDL);
    write(sock, buffer, strlen(buffer));
    sprintf(buffer, ENDL);
    write(sock, buffer, strlen(buffer));

    // entity
    sprintf(buffer, err_html, "500", "Internal Server Error");
    write(sock, buffer, strlen(buffer));
}

void err_501_not_implemented(int sock){
    char buffer[ERR_BUF_SIZE];

    // response line
    sprintf(buffer, "HTTP/1.1 501 Not Implemented"ENDL);
    write(sock, buffer, strlen(buffer));
    
    // response header
    sprintf(buffer, "%s", server_info);
    write(sock, buffer, strlen(buffer));
    sprintf(buffer, "Content-Type: text/html"ENDL);
    write(sock, buffer, strlen(buffer));
    sprintf(buffer, ENDL);
    write(sock, buffer, strlen(buffer));

    // entity    
    sprintf(buffer, err_html, "501", "Not Implemented");
    write(sock, buffer, strlen(buffer));
}

void res_200_ok(int sock, struct http_res * response){
    //TODO
    char buffer[ERR_BUF_SIZE];

    // response line
    sprintf(buffer, "HTTP/1.1 200 OK"ENDL);
    write(sock, buffer, sizeof(buffer));

    // response header
    for(struct header_item* item = response->hres_items; item != NULL; item = item->hi_next){
        // DEBUG("item = %p", item);
        sprintf(buffer, "%s: %s"ENDL, item->hi_key, item->hi_val);
        write(sock, buffer, strlen(buffer));
    }
    sprintf(buffer, ENDL);
    write(sock, buffer, strlen(buffer));

    // entity
    write(sock, response->hres_entity, response->hres_entity_length);
}

/*
 * CGI protocol
 */

// TODO

/*
 * response a file.
 */

int response_file(int sock, char* path_ptr){
    char path[BUFSIZ] = {0};
    DEBUG("path=%s, path_ptr=%s\n", path, path_ptr);
    struct stat path_stat;
    struct http_res response;

    // FIXME: this is not safe...
    strcpy(path, global_opts->DocumentRoot);
    strncat(path, path_ptr, strlen(path_ptr) + 1);
    if(stat(path, &path_stat) != 0){
        // error_handler(ERR_OTHER);// TODO
        DEBUG("404, strerror: %s, path=%s\n", strerror(errno), path);
        err_404_not_found(sock);
        return 404;
    }
    if(S_ISDIR(path_stat.st_mode)){
        strcat(path, "/index.html");
        if(stat(path, &path_stat) != 0){
            // error_handler(ERR_OTHER); // TODO
            DEBUG("404, strerror: %s, path=%s\n", strerror(errno), path);
            err_404_not_found(sock);
            return 404;
        }
    }

    FILE * file = fopen(path, "r");
    size_t file_size = 0;
    file_size = fread(response.hres_entity, 1, ENTITY_SIZE - 1, file);
    response.hres_entity_length = file_size;

    // recognize file type and fullfill header
    // TODO
    struct header_item * this_item = NULL;
    struct header_item * first_item = NULL;

    // Server
    this_item = malloc(sizeof(struct header_item));
    memset(this_item, 0, sizeof(struct header_item));
    strncpy(this_item->hi_key, "Server", 7);
    strncpy(this_item->hi_val, server_info_val, 20);
    this_item->hi_next = malloc(sizeof(struct header_item));
    first_item = this_item;

    // Content-type
    this_item = this_item->hi_next;
    memset(this_item, 0, sizeof(struct header_item));
    strncpy(this_item->hi_key, "Content-type", 13);
    if(!strcmp(strrchr(path, '.'), ".html")){
        strncpy(this_item->hi_val, "text/html", 10);
    }
    else if(!strcmp(strrchr(path, '.'), ".txt")){
        strncpy(this_item->hi_val, "text/plain", 11);
    }
    else if(!strcmp(strrchr(path, '.'), ".json")){
        strncpy(this_item->hi_val, "application/json", 17);
    }
    else if(!strcmp(strrchr(path, '.'), ".png")){
        strncpy(this_item->hi_val, "image/png", 10);
    }
    else{
        strncpy(this_item->hi_val, "application/octet-stream", 20);
    }

    response.hres_items = first_item;
    res_200_ok(sock, &response);
    return 200;
}

/*implementations for each method*/

int method_options(struct http_req * request, int sock){
    DEBUG("`method_options` start\n");
    err_501_not_implemented(sock);
    return 501;
}

int method_head(struct http_req * request, int sock){
    DEBUG("`method_head` start\n");
    err_501_not_implemented(sock);
    return 501;
}

int method_get(struct http_req * request, int sock){
    DEBUG("`method_get` start\n");
    char* path;
    char* args;
    path = strtok(request->hreq_uri, "?");
    args = strtok(NULL, "?");
    if(args == NULL){
        // response the file
        DEBUG("GET response file\n");
        return response_file(sock, path);
        // return 200;
    }
    else{
        // start cgi
        DEBUG("GET CGI\n");
        err_501_not_implemented(sock);
        return 501;
    }
    return 0;
}

int method_post(struct http_req * request, int sock){
    DEBUG("`method_post` start\n");
    err_501_not_implemented(sock);
    return 501;
}

int method_put(struct http_req * request, int sock){
    DEBUG("`method_put` start\n");
    err_501_not_implemented(sock);
    return 501;
}

int method_delete(struct http_req * request, int sock){
    DEBUG("`method_delete` start\n");
    err_501_not_implemented(sock);
    return 501;
}

int method_trace(struct http_req * request, int sock){
    DEBUG("`method_trace` start\n");
    err_501_not_implemented(sock);
    return 501;
}

int method_connect(struct http_req * request, int sock){
    DEBUG("`method_connect` start\n");
    err_501_not_implemented(sock);
    return 501;
}


/*
 * get method code
 */
int switch_method(char* method_str){
    if(!strcasecmp(method_str, "OPTIONS")){
        return HTTP_OPTIONS;
    }
    else if(!strcasecmp(method_str, "HEAD")){
        return HTTP_HEAD;
    }
    else if(!strcasecmp(method_str, "GET")){
        return HTTP_GET;
    }
    else if(!strcasecmp(method_str, "POST")){
        return HTTP_POST;
    }
    else if(!strcasecmp(method_str, "PUT")){
        return HTTP_PUT;
    }
    else if(!strcasecmp(method_str, "DELETE")){
        return HTTP_DELETE;
    }
    else if(!strcasecmp(method_str, "TRACE")){
        return HTTP_TRACE;
    }
    else if(!strcasecmp(method_str, "CONNECT")){
        return HTTP_CONNECT;
    }
    else{
        return HTTP_UNKOWN;
    }
}

/*
 * handel an incoming http request
 * the new process
 */
void * handle_request(void * args){
    // TODO: timeout
    int client_sock = *((int*)args);
    struct http_req request = {0};

    // parse 'request line'
    char reqline_buf[3 * HREQ_MEMBER_SIZE + 3];
    char method[HREQ_MEMBER_SIZE];
    int method_code = -1;

    DEBUG("request line\n");
    readline(client_sock, reqline_buf, sizeof(reqline_buf));
    sscanf(reqline_buf, "%s %s %s", method, request.hreq_uri, request.hreq_version);
    if((method_code = switch_method(method)) == HTTP_UNKOWN){
        error_handler(ERR_HTTPMETHOD);
    }

    request.hreq_method = method_code;

    if(strncmp(request.hreq_version, "HTTP/1.1", 8)){
        DEBUG("version: %s\n", request.hreq_version);
        error_handler(ERR_HTTPVER);
    }

    // set socket to non-block
    set_nonblock(client_sock);

    // parse 'http header'
    // TODO free before exit
    char reqheader_buf[HTTPHDR_KEY_SIZE + HTTPHDR_VAL_SIZE + 2];
    struct header_item * this_item = NULL;
    struct header_item * last_item = NULL;
    struct header_item * first_item = NULL;

    DEBUG("request header\n");
    while(readline(client_sock, reqheader_buf, sizeof(reqheader_buf)) != -1){
        this_item = malloc(sizeof(struct header_item));
        // TODO check if malloc is failed
        sscanf(reqheader_buf, "%s:%s", this_item->hi_key, this_item->hi_val);
        if(first_item == NULL){
            first_item = this_item;
        }
        else{
            last_item->hi_next = this_item;
        }
        last_item = this_item;
    }

    request.hreq_items = first_item;

    // get entity
    DEBUG("start reading entity\n");
    ssize_t t = read(client_sock, request.hreq_entity, ENTITY_SIZE); // could be 0
    request.hreq_entity[ENTITY_SIZE - 1] = '\0';

    set_block(client_sock);

    DEBUG("request line %s %s %s\n", method, request.hreq_uri, request.hreq_version);
    DEBUG("entity length = %d;return = %ld\n", (int)strlen(request.hreq_entity), t);

    // handle request
    int status = 0;
    switch(request.hreq_method){
        case HTTP_OPTIONS:
            status = method_options(&request, client_sock);
            break;
        case HTTP_HEAD:
            status = method_head(&request, client_sock);
            break;
        case HTTP_GET:
            status = method_get(&request, client_sock);
            break;
        case HTTP_POST:
            status = method_post(&request, client_sock);
            break;
        case HTTP_PUT:
            status = method_put(&request, client_sock);
            break;
        case HTTP_DELETE:
            status = method_delete(&request, client_sock);
            break;
        case HTTP_TRACE:
            status = method_trace(&request, client_sock);
            break;
        case HTTP_CONNECT:
            status = method_connect(&request, client_sock);
            break;
    }
    // DEBUG("finish execute:%s %s %s\n", method, request.hreq_uri, request.hreq_version);
    printf("[I] %s %s %s %d\n", method, request.hreq_uri, request.hreq_version, status);
exit:
    close(client_sock);
    // TODO: release
    return NULL;
}

