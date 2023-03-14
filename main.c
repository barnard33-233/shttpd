#include "conf.h"
#include <stdio.h>
#include <string.h>

int main(int argc, char ** argv){
    struct conf_opts opts;
    parse_conf(&opts, argc, argv);
    if(opts.help == 1){
        printf("%s", help_info);
    }
    print_conf(&opts);

    // start daemon and register SIGINT

    return 0;
}
