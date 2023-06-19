#define _GNU_SOURCE
#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include "conf.h"
#include "common.h"
#include <assert.h>

int is_set(int map, int mask){
    return (map & mask) == mask;
}

// delete extra spaces of a string
inline char* strtrim(char * str){
    assert(str != NULL);
    char * start = str;
    while(*start == ' '){
        start++;
    }
    char * end = str + strlen(str) - 1;
    while(*end == ' ' || *end == '\n'){
        end --;
    }
    *(end + 1) = '\0';
    return start;
}

// parse a single line in the .conf file. separate key and value from the whole line
static int parse_line(char * buffer, char ** value){
    assert(buffer != NULL);

    char buffer_copy[16];
    strncpy(buffer_copy, buffer, 15);
    buffer_copy[15] = '\0';

    while(*buffer == ' '){
        buffer ++;
    }

    // ignore blank lines and comments
    if(buffer[0] == '\n' || buffer[0] == '#'){ 
        return -1;
    }

    char * key = NULL;

    key = strtrim(strtok(buffer, "="));
    *value = strtrim(strtok(NULL, "="));
    if(key == NULL || *value == NULL || strtok(NULL, "=") != NULL){
        fprintf(stderr, "WARNING: Syntex error in SHTTPD.conf, around \"%s\"\n", buffer_copy);
        return -1;
    }

    for(int i = 0; i < CONF_COUNT; i++){
        if(strcmp(conf_name[i], key) == 0){
            return i;
        }
    }
    return -1;
}

// parse configuration from file
int parse_conf_file(struct conf_opts* opts, char* file_name){
    FILE * file = fopen(file_name, "r");
    if(file == NULL){
        return -1;
    }
    char * buffer;
    size_t buffer_len = 0;
    ssize_t read_len = 0;
    while((read_len = getline(&buffer, &buffer_len, file)) != -1){
        char * value;
        int conf_id = parse_line(buffer, &value);
        switch(conf_id){
            case 0:
            strncpy(opts->CGIRoot, value, 128);
            break;

            case 1:
            strncpy(opts->DefaultFile, value, 128);
            break;

            case 2:
            strncpy(opts->DocumentRoot, value, 128);
            break;

            case 3:
            strncpy(opts->ConfigFile, value, 128);
            break;

            case 4:
            opts->ListenPort = atoi(value);
            break;

            case 5:
            opts->MaxClient = atoi(value);
            break;

            case 6:
            opts->TimeOut = atoi(value);
            break;

            case -1:
            break;
        }

        free(buffer);
    }
    fclose(file);
    return 0;
}

// parse params from command line
int parse_conf_cmd(struct conf_opts * opts, int argc, char ** argv){
    assert(opts != NULL);
    
    if(argc <= 1){
        return OPT_SET_NONE;
    }

    int map = OPT_SET_NONE;
    int opt_response = 0;
    int index = 0;
    while(EOF != (opt_response = getopt_long(argc, argv, short_opts, long_opts, &index))){
        switch(opt_response){
            case 'c':
            strncpy(opts->CGIRoot, optarg, 128);
            map |= OPT_SET_c;
            break;

            case 'd':
            strncpy(opts->DefaultFile, optarg, 128);
            map |= OPT_SET_d;
            break;

            case 'o':
            strncpy(opts->DocumentRoot, optarg, 128);
            map |= OPT_SET_o;
            break;

            case 'f':
            strncpy(opts->ConfigFile, optarg, 128);
            map |= OPT_SET_f;
            break;

            case 'l':
            opts->ListenPort = atoi(optarg);
            map |= OPT_SET_l;
            break;

            case 'm':
            opts->MaxClient = atoi(optarg);
            map |= OPT_SET_m;
            break;

            case 't':
            opts->TimeOut = atoi(optarg);
            map |= OPT_SET_t;
            break;

            case 'h':
            opts->help = 1;
            map |= OPT_SET_h;
            break;

            case '?':
            fprintf(stderr, "ERROR: Unkonwn option: %c", (char)opt_response);
            break;

            case ':':
            fprintf(stderr, "ERROR: Missing Argument of option %c\n", (char)opt_response);
            break;

            default:
            break;
        }
    }
    return map;
}

// print struct conf*opts
#ifndef NDEBUG
void print_conf(struct conf_opts* opts){
    printf(
        "CGIRoot:%s\n"
        "DefaultFile:%s\n"
        "DocumentRoot:%s\n"
        "ConfigFile:%s\n"
        "ListenPort=%d\n"
        "MaxClient=%d\n"
        "TimeOut=%d\n",
        opts->CGIRoot,
        opts->DefaultFile,
        opts->DocumentRoot,
        opts->ConfigFile,
        opts->ListenPort,
        opts->MaxClient,
        opts->TimeOut
    );
}
#else
void print_conf(struct conf_opts* opts){}
#endif

void parse_conf(struct conf_opts* opts, int argc, char ** argv){
    struct conf_opts cmdline_opts = default_conf_opt;
    int map = OPT_SET_NONE;

    *opts = default_conf_opt;
    
    // parse command line paramaters
    map = parse_conf_cmd(&cmdline_opts, argc, argv);
    if(is_set(map, OPT_SET_ALL_CONF)){
        *opts = cmdline_opts;
    }
    else{
        char conf_file[128] = {0};
        if (is_set(map, OPT_SET_f)){
            strncpy(opts->ConfigFile, cmdline_opts.ConfigFile, 128);
        }
        strncpy(conf_file, opts->ConfigFile, 128);
        parse_conf_file(opts, conf_file);
        
        if (is_set(map, OPT_SET_c)){
            strncpy(opts->CGIRoot, cmdline_opts.CGIRoot, 128);
        }

        if (is_set(map, OPT_SET_d)){
            strncpy(opts->DefaultFile, cmdline_opts.DefaultFile, 128);
        }
        
        if (is_set(map, OPT_SET_o)){
            strncpy(opts->DocumentRoot, cmdline_opts.DocumentRoot, 128);
        }

        if (is_set(map, OPT_SET_l)){
            opts->ListenPort = cmdline_opts.ListenPort;
        }

        if (is_set(map, OPT_SET_m)){
            opts->MaxClient = cmdline_opts.MaxClient;
        }

        if (is_set(map, OPT_SET_t)){
            opts->TimeOut = cmdline_opts.TimeOut;
        }

        if (is_set(map, OPT_SET_h)){
            opts->help = cmdline_opts.help;
        }
    }
}
