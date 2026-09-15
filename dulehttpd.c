#include "./dulehttpd_cli.h"
#include "./dulehttpd_srv.h"
#include "./dulehttpd_help.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>


int main(int argc,char* argv[]){
        if (argc > 1){
            if (!strcmp(argv[1],"-help")) {
                help();
                //printf("There's no help yet lol XD 67 sigma\n");
                return 0;
            }
        }   

    struct config* conf;
    conf = malloc(sizeof(struct config));
    
    /*
    Сначала проверяем ОС, если Линукс то продожаем работу
    */
    #if defined(_WIN32) || defined(_WIN64)
        // *Пока что* версии под виндовс не существует
        printf("Sorry now there's only Linux support :(\n");
        free(conf);
        return 1;
    #elif defined(__linux__)
        // Не завершаем работу


        printf("setup config starting");
        if(!setup_config(argc,argv,&conf)){
            fprintf(stderr,"Error occuried\n");
            free(conf);
            return 1;
        }
        print_out_config(conf);
        int socket = srv_init(conf);

        int client;
        while (1) {
            client = cli_accept(socket);
            if (!client) {
                if(conf->show_debug_output == 2)  fprintf(stderr, "Error occuried, I'm out\n  0.8 seconds\n");
                free(conf);
                return 0;
            }
            if (conf->show_debug_output > 0) {
                printf("Incomming connection\n");
            }
            int f = fork();
            if(!f){
                cli_connection(socket,client,conf);
            }
        }
    #else
        // Неизвестная оперативная система
        fprintf(stderr,"Sorry now there's only Linux support :(\n")
        free(conf);
        return 1;
    #endif
    
    free(conf);
    return 0;
}