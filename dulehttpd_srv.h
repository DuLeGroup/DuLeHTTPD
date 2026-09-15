#include "dulehttpd_config.h"
#if defined(_WIN32) || defined(_WIN64)
    
#elif defined(__linux__)
    #include <netinet/in.h>
    #include <sys/socket.h>
    #include <arpa/inet.h>
    #include <sys/types.h>
    #include <stdio.h>
    #include <unistd.h>
#endif



/*
    Binds the socket
    Returns socket
    Returns -1 in case of error
*/
int srv_init(struct config* conf){
#if defined(_WIN32) || defined(_WIN64)
    return (-1);
#elif defined(__linux__)
    int sock;
    struct sockaddr_in srv;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock < 0){
        return -1;
    }
    
    srv.sin_family = AF_INET;
    srv.sin_addr.s_addr = inet_addr(conf->listen_addr);
    srv.sin_port = htons(conf->port);

    int bind_result = bind(sock, (struct sockaddr*)&srv, sizeof(srv));

    if (bind_result) {
        close(sock);
        fprintf(stderr, "bind error. Supposedly the port %d is already used\n",conf->port);
        return -1;
    }

    int l = listen(sock, conf->max_connections);
    if (l) {
        fprintf(stderr, "Listen error\n");
        return -1;
    }else{
        fprintf(stdout, "Started listening at http://%s:%d\n",conf->listen_addr,conf->port);
    }
    
    return sock;
#else
    return -1;
#endif
}