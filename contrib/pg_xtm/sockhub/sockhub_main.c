#include <stdio.h>
#include <stdlib.h>
#include "sockhub.h"
                   
int main(int argc, char* argv[]) 
{
    int i;
    Shub shub;
    ShubParams params;
    
    ShubInitParams(&params);
    
    for (i = 1; i < argc-1; i++) { 
        if (argv[i][0] == '-') { 
            switch (argv[i][1]) { 
              case 'h':
                params.host = argv[++i];
                continue;
              case 'p':
                params.port = atoi(argv[++i]);
                continue;
              case 'f':
                params.file = argv[++i];
                continue;
              case 'd':
                params.delay = atoi(argv[++i]);
                continue;
              case 'b':
                params.buffer_size = atoi(argv[++i]);
                continue;
              case 'r':
                params.max_attempts = atoi(argv[++i]);
                continue;
            }
        }
      Usage:
        fprintf(stderr, "sockhub: combine several local unix socket connections into one inet socket\n"
                "Options:\n"
                "\t-h HOST\tremote host name\n"                
                "\t-p PORT\tremote port\n"
                "\t-f FILE\tunix socket file name\n"
                "\t-d DELAY\tdelay for waiting income requests (milliseconds)\n"
                "\t-b SIZE\tbuffer size\n"
                "\t-q SIZE\tlisten queue size\n"
                "\t-r N\tmaximun connect attempts\n"
             );
        
        return 1;
    }
    if (params.host == NULL || params.file == NULL || i != argc) { 
        goto Usage;
    }
    
    ShubInitialize(&shub, &params);
    
    ShubLoop(&shub);

    return 0;
}
