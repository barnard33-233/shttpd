#include "conf.h"
#include <getopt.h>
#include <stdlib.h>

const struct conf_opts default_conf_opt = {
    "/usr/local/var/www/cgi-bin",
    "index.html",
    "/usr/local/var/www",
    "/etc/SHTTPD.conf",
    8080,
    4,
    3,
    0
};

// Used to parse .conf file
const char conf_name[CONF_COUNT][64] = {
    "CGIRoot",
    "DefaultFile",
    "DocumentRoot",
    "ConfigFile",
    "ListenPort",
    "MaxClient",
    "TimeOut",
};

const struct option long_opts[] = {
    {"CGIRoot", required_argument, NULL, 'c'},
    {"DefaultFile", required_argument, NULL, 'd'},
    {"DocumentRoot", required_argument, NULL, 'o'},
    {"ConfigFile", required_argument, NULL, 'f'},
    {"ListenPort", required_argument, NULL, 'l'},
    {"MaxClient", required_argument, NULL, 'm'},
    {"TimeOut", required_argument, NULL, 't'},
    {"Help", no_argument, NULL, 'h'},
    {0,0,0,0},
};

const char * short_opts = "c:d:o:f:l:m:t:h";

const char * help_info = "Usage: shttpd [command] [command] ...\n"
                         "Commands:\n"
                         "\t--CGIRoot       (-c) <path>\n"
                         "\t--DefaultFile   (-d) <path>\n"
                         "\t--DocumentRoot  (-o) <path>\n"
                         "\t--ConfigFile    (-f) <path>\n"
                         "\t--ListenPort    (-l) <port>\n"
                         "\t--MaxClient     (-m) <number>\n"
                         "\t--TimeOut       (-t) <number>\n"
                         "\t--Help          (-h)"
;
