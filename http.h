#ifndef __HTTP_H
#define __HTTP_H

// method
#define HTTP_OPTIONS 1
#define HTTP_HEAD    2
#define HTTP_GET     3
#define HTTP_POST    4
#define HTTP_PUT     5
#define HTTP_DELETE  6
#define HTTP_TRACE   7
#define HTTP_CONNECT 8
#define HTTP_UNKOWN  -1

#define HREQ_MEMBER_SIZE 128
#define HRES_MEMBER_SIZE 128
#define HTTPHDR_KEY_SIZE 128
#define HTTPHDR_VAL_SIZE 1024
#define ENTITY_SIZE 8192

#define ERR_BUF_SIZE 4096
#define MAX_METHOD_SIZE 8

#define ENDL "\r\n"

struct header_item{
    char hi_key[HTTPHDR_KEY_SIZE];
    char hi_val[HTTPHDR_VAL_SIZE];
    struct header_item* hi_next;
};

struct http_req{
    // request line
    char hreq_method[MAX_METHOD_SIZE]; // TODO
    char hreq_uri[HREQ_MEMBER_SIZE];
    char hreq_version[HREQ_MEMBER_SIZE];
    int(*hreq_method_fn)(struct http_req*, int);

    // header
    struct header_item* hreq_items;

    // entity
    char hreq_entity[ENTITY_SIZE];
};

struct http_res{
    // response line
    // char hres_version[HRES_MEMBER_SIZE];
    // int hres_status;
    // char hres_status_desc[HRES_MEMBER_SIZE];
    // actually we don't need that.

    // header
    struct header_item* hres_items;

    // entity
    char hres_entity[ENTITY_SIZE];
    int hres_entity_length; // including '\0'
};


// char reqline_format* = "%s %s %s"HTTP_ENDL;
// char resline_format* = "%s %d %s"HTTP_ENDL;
// char item_format* = "%s:%s"HTTP_ENDL;


void * handle_request(void* args);
void hi_free(struct header_item *item);
void hreq_free(struct http_req * hreq);
void hres_free(struct http_res * hres);


void err_500_internal_server_error(int sock);

#endif