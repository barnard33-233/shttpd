#include "common.h"
#include "conf.h"
#include "http.h"

#include <strings.h>
#include <fcntl.h>

const char * err_html = "<!DOCTYPE HTML><html><body><h>%s</h><p>%s</p></body></html>"ENDL;
const char * server_info = "Server: shttpd/0.1"ENDL;
const char * server_info_val = "shttpd/0.1";

int parse_header_line(char* buffer, char* key, char* val){
    char* ptr;
    for(ptr = buffer; *ptr != ':'; ptr++){
        if(*ptr == '\0'){
            return -1;
        }
    }
    *ptr = '\0';
    strcpy(key, buffer);
    strcpy(val, ptr + 1);
    return 0;
}

int readline(int fd, char * buffer, int size){
    char this = 0, last = 0;
    for(int i = 0; i < size - 1; i++){
        read(fd, &this, 1);
        if(this == '\n'){
            // end
            if(last != '\r'){
                return 1; // syntax error 400
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
    return 2; // buffer overflow 500
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

void err_505_version_not_supported(int sock){
    char buffer[ERR_BUF_SIZE];

    // response line
    sprintf(buffer, "HTTP/1.1 505 Version Not Supported"ENDL);
    write(sock, buffer, strlen(buffer));
    
    // response header
    sprintf(buffer, "%s", server_info);
    write(sock, buffer, strlen(buffer));
    sprintf(buffer, "Content-Type: text/html"ENDL);
    write(sock, buffer, strlen(buffer));
    sprintf(buffer, ENDL);
    write(sock, buffer, strlen(buffer));

    // entity    
    sprintf(buffer, err_html, "505", "Version Not Supported");
    write(sock, buffer, strlen(buffer));
}

void res_200_ok(int sock, struct http_res * response){
    char buffer[ERR_BUF_SIZE];

    // response line
    sprintf(buffer, "HTTP/1.1 200 OK"ENDL);
    write(sock, buffer, strlen(buffer));

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

int exec_cgi(struct http_req* hreq, int sock, char* query){
    char path[BUFSIZ] = {0};
    char query_copy[BUFSIZ] = {0};
    char buffer[BUFSIZ] = {0};
    int cgi_output[2];
    int cgi_input[2];
    int status = 0;
    pid_t pid;
    struct http_res response;

    // copy
    strcpy(query_copy, query);

    // set path
    strcpy(path, global_opts->CGIRoot);
    strcat(path, "/cgi-bin");
    DEBUG("cgi script path=%s\n", path);

    // pipe
    if(pipe(cgi_input) < 0){
        DEBUG("fail to create cgi_input\n");
        err_500_internal_server_error(sock);
        status = 500;
        goto exec_cgi_exit;
    }

    if(pipe(cgi_output) < 0){
        DEBUG("fail to create cgi_output\n");
        err_500_internal_server_error(sock);
        status = 500;
        goto exec_cgi_exit;
    }

    pid = fork();

    // child
    if(pid == 0){
        // set env
        setenv("REQUEST_METHOD", hreq->hreq_method, 1);

        if(strncasecmp(hreq->hreq_method, "GET", 3) == 0){
            // GET
            setenv("QUERY_STRING", query_copy, 1);
        }
        else{
            err_501_not_implemented(sock);
            status = 501;
            goto exec_cgi_exit_child;
        }

        close(cgi_output[0]);
        close(cgi_input[1]);
        dup2(cgi_output[1], 1);
        dup2(cgi_input[0], 0);

        // execl
        execl(path, path, NULL);
        DEBUG("`execl` fail\n");
        err_500_internal_server_error(sock);
        status = 500;
        goto exec_cgi_exit_child;
    }
    else if(pid < 0){
        DEBUG("fail to fork CGI\n");
        err_500_internal_server_error(sock);
        status = 500;
        goto exec_cgi_exit;
    }

    // parent:
    close(cgi_output[1]);
    close(cgi_input[0]);
    
    // read:
    int ret = 0;
    int status_temp = 0;
    int flag_header_or_entity = 0;//0 for header, 1 for entity
    struct header_item * first_item = NULL;
    struct header_item * item;

    set_block(cgi_output[0]);
    set_block(cgi_input[1]);
    while(1){
        if(flag_header_or_entity == 0){
            //read
            ret = readline(cgi_output[0], buffer, BUFSIZ);
            if(ret == -1){
                if(flag_header_or_entity == 0){
                    flag_header_or_entity = 1;
                    response.hres_items = first_item;
                    continue;
                }
            }
            if(ret == 2){ // too long
                DEBUG("CGI return too long\n");
                err_500_internal_server_error(sock);
                status = 500;
                goto exec_cgi_exit;
            }

            // analyze
            if(first_item == NULL){
                item = malloc(sizeof(struct header_item));
                first_item = item;
                memset(item, 0, sizeof(struct header_item));
            }
            else{
                item->hi_next = malloc(sizeof(struct header_item));
                item = item->hi_next;
                memset(item, 0, sizeof(struct header_item));
            }
            // sscanf(buffer, "%s: %s", item->hi_key, item->hi_val);
            parse_header_line(buffer, item->hi_key, item->hi_val); // TODO handle error
        }
        else{
            size_t size;
            size = read(cgi_output[0], response.hres_entity, ENTITY_SIZE);
            response.hres_entity_length = (int)size;
            break;
        }
    }

    if((ret = wait(&status_temp)) > 0){
        DEBUG("CGI exit: %d\n", status_temp);
        if(status_temp != 0){
            status = status_temp;
        }
        else{
            res_200_ok(sock, &response);
            status = 200;
        }
    }


exec_cgi_exit:
    hi_free(response.hres_items);
    close(cgi_output[0]);
    close(cgi_input[1]);
    return status;
exec_cgi_exit_child:
    close(cgi_output[1]);
    close(cgi_input[0]);
    exit(status);
}

/*
 * response a file.
 */

int response_file(int sock, char* path_ptr){
    DEBUG("`response_file`\n");
    char path[BUFSIZ] = {0};
    // DEBUG("path=%s, path_ptr=%s\n", path, path_ptr);
    struct stat path_stat;
    struct http_res response;
    int status;

    // FIXME: this is not safe...
    strcpy(path, global_opts->DocumentRoot);
    strcat(path, path_ptr);
    if(stat(path, &path_stat) != 0){
        DEBUG("404, strerror: %s, path=%s\n", strerror(errno), path);
        err_404_not_found(sock);
        status = 404;
        goto response_file_exit;
    }
    if(S_ISDIR(path_stat.st_mode)){
        strcat(path, "/");
        strcat(path, global_opts->DefaultFile);
        if(stat(path, &path_stat) != 0){
            DEBUG("404, strerror: %s, path=%s\n", strerror(errno), path);
            err_404_not_found(sock);
            status = 404;
            goto response_file_exit;
        }
    }

    FILE * file = fopen(path, "r");
    size_t file_size = 0;
    file_size = fread(response.hres_entity, 1, ENTITY_SIZE - 1, file);
    response.hres_entity_length = file_size;

    // recognize file type and fullfill header
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
        strncpy(this_item->hi_val, "application/octet-stream", 24);
    }

    response.hres_items = first_item;
    res_200_ok(sock, &response);
    status = 200;
response_file_exit:
    hi_free(response.hres_items);
    return status;
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
        DEBUG("GET: response file\n");
        return response_file(sock, path);
    }
    else{
        // start cgi
        DEBUG("GET: CGI\n");
        return exec_cgi(request, sock, args);
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
int switch_method(struct http_req* hreq){
    if(!strncasecmp(hreq->hreq_method, "OPTIONS", 7)){
        hreq->hreq_method_fn = method_options;
    }
    else if(!strncasecmp(hreq->hreq_method, "HEAD", 4)){
        hreq->hreq_method_fn = method_head;
    }
    else if(!strncasecmp(hreq->hreq_method, "GET", 3)){
        hreq->hreq_method_fn = method_get;
    }
    else if(!strncasecmp(hreq->hreq_method, "POST", 4)){
        hreq->hreq_method_fn = method_post;
    }
    else if(!strncasecmp(hreq->hreq_method, "PUT", 3)){
        hreq->hreq_method_fn = method_put;
    }
    else if(!strncasecmp(hreq->hreq_method, "DELETE", 6)){
        hreq->hreq_method_fn = method_delete;
    }
    else if(!strncasecmp(hreq->hreq_method, "TRACE", 5)){
        hreq->hreq_method_fn = method_trace;
    }
    else if(!strncasecmp(hreq->hreq_method, "CONNECT", 7)){
        hreq->hreq_method_fn = method_connect;
    }
    else{
        return -1;
    }
    return 0;
}

/*
 * handel an incoming http request
 * the new process
 */
void * handle_request(void * args){
    // TODO: timeout
    int client_sock = *((int*)args);
    struct http_req request = {0};
    int status = 0;
    int ret;

    // parse 'request line'
    char reqline_buf[3 * HREQ_MEMBER_SIZE + 3];
    // char method[HREQ_MEMBER_SIZE];

    DEBUG("start: parse request line\n");
    ret = readline(client_sock, reqline_buf, sizeof(reqline_buf));
    if(ret == 1 || ret == -1){
        err_400_bad_request(client_sock);
        status = 400;
        goto handle_request_exit;
    }
    else if(ret == 2){
        err_500_internal_server_error(client_sock);
        status = 500;
        goto handle_request_exit;
    }
    sscanf(reqline_buf, "%s %s %s", request.hreq_method, request.hreq_uri, request.hreq_version);
    if((switch_method(&request)) == -1){
        DEBUG("bad method\n");
        err_400_bad_request(client_sock);
        status = 400;
        goto handle_request_exit;
    }

    if(strncmp(request.hreq_version, "HTTP/1.1", 8)){
        DEBUG("version not supported. version: %s\n", request.hreq_version);
        err_505_version_not_supported(client_sock);
        status = 505;
        goto handle_request_exit;
    }

    // set socket to non-block
    set_nonblock(client_sock);

    // parse 'http header'
    char reqheader_buf[HTTPHDR_KEY_SIZE + HTTPHDR_VAL_SIZE + 2];
    struct header_item * this_item = NULL;
    struct header_item * last_item = NULL;
    struct header_item * first_item = NULL;

    DEBUG("start: parse request header\n");
    while(1){
        ret = readline(client_sock, reqheader_buf, sizeof(reqheader_buf));
        if (ret == -1){
            break;
        }
        else if(ret == 1){
            err_400_bad_request(client_sock);
            status = 400;
            goto handle_request_exit;
        }
        else if (ret == 2){
            err_500_internal_server_error(client_sock);
            status = 500;
            goto handle_request_exit;
        }

        if((this_item = malloc(sizeof(struct header_item))) == NULL){
            err_500_internal_server_error(client_sock);
            status = 500;
            goto handle_request_exit;
        }
        parse_header_line(reqheader_buf, this_item->hi_key, this_item->hi_val);
        this_item->hi_next = NULL;
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
    DEBUG("start: parse entity\n");
    read(client_sock, request.hreq_entity, ENTITY_SIZE); // could be 0
    request.hreq_entity[ENTITY_SIZE - 1] = '\0';

    set_block(client_sock);

    DEBUG("finish parsing request. INFO: %s %s %s\n", request.hreq_method, request.hreq_uri, request.hreq_version);

    // handle request
    status = request.hreq_method_fn(&request, client_sock);

    // DEBUG("finish execute:%s %s %s\n", method, request.hreq_uri, request.hreq_version);
handle_request_exit:
    printf("[I] %s %s %s %d\n", request.hreq_method, request.hreq_uri, request.hreq_version, status);
    close(client_sock);
    hi_free(request.hreq_items);
    return NULL;
}


void hi_free(struct header_item *item){
    for(struct header_item * this_item = item, * next_item = item; next_item != NULL; this_item = next_item){
        next_item = this_item->hi_next;
        free(this_item);
    }
}

/*
void hreq_free(struct http_req * hreq){
    hi_free(hreq->hreq_items);
    free(hreq);
}

void hres_free(struct http_res * hres){
    hi_free(hres->hres_items);
    free(hres);
}
*/
