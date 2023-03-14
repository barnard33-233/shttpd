#ifndef _CONF_H
#define _CONF_H

#include <getopt.h>

#define CONF_COUNT 7

#define OPT_SET_NONE (0x0)
#define OPT_SET_c (0x01)
#define OPT_SET_d (0x02)
#define OPT_SET_o (0x04)
#define OPT_SET_f (0x08)
#define OPT_SET_l (0x010)
#define OPT_SET_m (0x020)
#define OPT_SET_t (0x040)
#define OPT_SET_h (0x080)
#define OPT_SET_ALL_CONF (\
                    OPT_SET_c |\
                    OPT_SET_d |\
                    OPT_SET_o |\
                    OPT_SET_f |\
                    OPT_SET_l |\
                    OPT_SET_m |\
                    OPT_SET_t \
                    )
#define OPT_SET_ALL (\
                    OPT_SET_c |\
                    OPT_SET_d |\
                    OPT_SET_o |\
                    OPT_SET_f |\
                    OPT_SET_l |\
                    OPT_SET_m |\
                    OPT_SET_t |\
                    OPT_SET_h \
                    )

struct conf_opts{
    char CGIRoot[128];
    char DefaultFile[128];
    char DocumentRoot[128];
    char ConfigFile[128];
    int ListenPort;
    int MaxClient;
    int TimeOut;
    int help;
};

// consts
extern const struct conf_opts default_conf_opt;
extern const char conf_name[CONF_COUNT][64];
extern const struct option long_opts[];
extern const char * short_opts;
extern const char * help_info;

// functions:
void parse_conf(struct conf_opts* opts, int argc, char ** argv);
int parse_conf_file(struct conf_opts* opts, char* file_name);
int parse_conf_cmd(struct conf_opts* conf_opts_cmd, int argc, char ** argv);
void print_conf(struct conf_opts * conf);
char* strtrim(char* str);
int is_set(int map, int mask);

#endif
