#ifndef DULEHTTPD_CONFIG_H
#define DULEHTTPD_CONFIG_H
#if defined(_WIN32) || defined(_WIN64)
    
#elif defined(__linux__)
    #include <netinet/in.h>
    #include <sys/socket.h>
    #include <arpa/inet.h>
    #include <sys/types.h>
    #include <stdio.h>
    #include <unistd.h>
    #include <stdlib.h>
    #include <string.h>
#endif

typedef short boolean;

struct config{
    /* shall show debug output or no. 0 for none, 1 for short, 2 for full. default = 2. -do to config*/
    short show_debug_output;
    /* port that server will run on, default = 8080 . -p to config*/
    int port;
    /* listen address, default = 127.0.0.1 . -la to config */
    char* listen_addr;
    /* directory that conatains site's files, default = /var/www/ (cannot contain ~) . -rd to config*/
    char* root_directory;
    /* Maximum connections at once, default = 5 . -mc to config*/
    int max_connections;
    /* at request will throw 403 Forbidden divided by '///' . -fd to config*/
    char* forbidden_directories;
    /* shows if needs to check for forbidden directories, default = 0*/
    boolean is_forbidden_directories;
    /* at request will throw 403 Forbidden divided by '///' . -ff to config*/
    char* forbidden_files;
    /* shows if needs to check for forbidden directories, default = 0*/
    boolean is_forbidden_files;
    /* In bytes, default = 10240B(10 kilobytes) . -mbs to config*/
    unsigned long long max_body_size;
    /* File will be sent on 404 Not Found default=0 (the default dulehttpd 404 file). -404 to config*/
    char* file_at_404;
    /* File will be sent on 403 Forbidden default=0 (the default dulehttpd 404 file). -403 to config*/
    char* file_at_403;
    /* maximum size of an incomming file default=10240. -mis to config*/
    unsigned long long max_incomming_file_size;
};
/*
Устанавливает базовые значения всем параметрам конфига
*/
void set_default_config(struct config** conf){
    // Числовые настройки
    (*conf)->show_debug_output = 2;
    (*conf)->port = 8080;
    (*conf)->max_connections = 5;
    (*conf)->max_body_size = 10240ULL; // 10 КБ в байтах
    (*conf)->max_incomming_file_size = 1024ULL;

    // Флаги проверки
    (*conf)->is_forbidden_directories = 0;
    (*conf)->is_forbidden_files = 0;

    // Строковые настройки (дублируем строки в динамическую память)
    (*conf)->listen_addr = strdup("127.0.0.1");
    (*conf)->root_directory = strdup("/var/www/");

    // Указатели на опциональные файлы/списки по умолчанию NULL
    (*conf)->forbidden_directories = NULL;
    (*conf)->forbidden_files = NULL;
    (*conf)->file_at_404 = NULL;
    (*conf)->file_at_403 = NULL;
}

void print_out_config(struct config* conf){
    if (conf->show_debug_output == 2) {
        fprintf(stdout, "DuLe httpd config:\n");

        fprintf(stdout, "Port: %d\n",conf->port);
        fprintf(stdout, "Listening Addres: %s\n",conf->listen_addr);
        fprintf(stdout, "Root Directory: %s\n",conf->root_directory);
        fprintf(stdout, "Max Connections: %d\n",conf->max_connections);
        fprintf(stdout, "Forbidden Directories: %s\n",conf->forbidden_directories);
        fprintf(stdout, "Are there forbidden directories: %d\n",conf->is_forbidden_directories);
        fprintf(stdout, "Forbidden files: %s\n",conf->forbidden_files);
        fprintf(stdout, "Are there forbidden files: %d\n",conf->is_forbidden_files);
        fprintf(stdout, "Maximum body size(in bytes): %llu\n",conf->max_body_size);
        fprintf(stdout, "Maximum size of an incomming file: %llu\n",conf->max_incomming_file_size);
        fprintf(stdout, "404 error file: %s\n", conf->file_at_404);
        fprintf(stdout, "403 error file: %s\n", conf->file_at_403);
        
    }else if (conf->show_debug_output == 1) {
        fprintf(stdout, "DuLe httpd config:\n");

        fprintf(stdout, "Listening to %s:%d",conf->listen_addr,conf->port);
        fprintf(stdout, "Root Directory: %s\n",conf->root_directory);
        if (conf->is_forbidden_directories) fprintf(stdout, "Forbidden Directories: %s\n",conf->forbidden_directories);
        if (conf->is_forbidden_files) fprintf(stdout, "Forbidden files: %s\n",conf->forbidden_files);
    }
}

/*
Настраивает conf конфиг.
В случае удачи возвращает 1
В случае ошибки возвращает 0(0x0(0b0(0h( cos (90deg) ))))
*/
int setup_config(int argcount,char* arguments[],struct config** conf){
    /*
    Обрабатываемый параметр, возможные значения:
    -p
    -la
    -rd
    -mc
    -fd
    -ff
    -do
    -mbs
    -mis
    -404
    -403
    */
    set_default_config(conf);

    if (argcount < 2) {
        return 67;
    }

    char* setting_parametr;
    
    for (int i = 1; i < argcount; i++) {
        char* type = arguments[i];
        if (strcmp(type, "-p") == 0) {
            (*conf)->port = atoi(arguments[i+1]);

            //ERRORS VALIDATION
            if (!(*conf)->port) {
                fprintf(stderr, "Sorry buddy, we principally do not support 0th port\n");
                return 0;
            }else
            if((*conf)->port < 0){
                fprintf(stderr, "Nah bro, a negative port?! Why?...\n");
                return 0;
            }else if((*conf)->port < 1024){
                fprintf(stderr, "May be errors beacause of a privilege port(Basically I recomend better using a port between 1024-49151)\n");
            }else if((*conf)->port > 65535){
                fprintf(stderr, "there are no ports after 65535, so you gotta choose something smaller :(\n");
                return 0;
            }else if((*conf)->port > 49151){
                fprintf(stderr, "May be errors beacause of a dynamic/private ports(Basically I recomend better using a port between 1024-49151)\n");
            }
        } else if (strcmp(type, "-la") == 0) {
            (*conf)->listen_addr = arguments[i+1];
            // ERRORS VALIDATION
            // none yet :0
        } else if (strcmp(type, "-rd") == 0) {
            (*conf)->root_directory = arguments[i+1];
        } else if (strcmp(type, "-mc") == 0) {
            (*conf)->max_connections = atoi(arguments[i+1]);
            if ((*conf)->max_connections < 1) {
                fprintf(stderr, "bruh, less than 1 connection is diabolical\n");
                return 0;
            }
        } else if (strcmp(type, "-fd") == 0) {
            (*conf)->forbidden_directories = arguments[i+1];
            (*conf)->is_forbidden_directories = 1;
        } else if (strcmp(type, "-do") == 0) {
            (*conf)->show_debug_output = atoi(arguments[i+1]);
            if ((*conf)->show_debug_output < 0 || (*conf)->show_debug_output > 2) {
                fprintf(stderr, "-do can only accept 0 or 1 or 2. why would you pick other number \n");
                return 1;
            }
        } else if (strcmp(type, "-ff") == 0) {
            (*conf)->forbidden_files = arguments[i+1];
            (*conf)->is_forbidden_files = 1;
        } else if (strcmp(type, "-mbs") == 0) {
            (*conf)->max_body_size = atoi(arguments[i+1]);
            if ((*conf)->max_body_size <= 0) {
                fprintf(stderr, "at least 1 byte please brooooo\n");
                return 0;
            }else if ((*conf)->max_body_size < 1024) {
                fprintf(stderr, "Uf, less than kB of max body size is tough\n");
            }
        } else if (strcmp(type, "-mis") == 0) {
            (*conf)->max_incomming_file_size = atoi(arguments[i+1]);
            if ((*conf)->max_incomming_file_size <= 0) {
                fprintf(stderr, "at least 1 byte please brooooo\n");
                return 0;
            }else if ((*conf)->max_incomming_file_size < 1024) {
                fprintf(stderr, "Uf, less than kB of max icomming file size is tough\n");
            }
        }  else if (strcmp(type, "-404") == 0) {
            printf("I hope you wrote the path respect to root folder(if you didn't set it, by default it's ~/.public/)\n");
            (*conf)->file_at_404 = arguments[i+1];
        } else if (strcmp(type, "-403") == 0) {
            printf("I hope you wrote the path respect to root folder(if you didn't set it, by default it's ~/.public/)\n");
            (*conf)->file_at_403 = arguments[i+1];
        } else {
            fprintf(stderr, "Unknown argument/param or what you just wrote(btw I'm going further) %s\n",type);
            continue;    
        }
        // we go to next pair '-param(arguments[i]) value(arguments[i+1])', basically doing i+=2 at every param unless there's something unusual
        i++;
    }
    return 67;
}

#endif