#ifndef DULEHTTPD_CLI_H
#define DULEHTTPD_CLI_H
#include "dulehttpd_reqres.h"
#include "dulehttpd_config.h"
#include "dulehttpd_fs.h"

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

/*
Return 0 in case of error
*/
int cli_accept(int sock){
    int c;
    socklen_t addrlen;
    struct sockaddr_in client;

    addrlen = 0;
    memset(&client, 0, sizeof(client));
    c = accept(sock, (struct sockaddr *)&client, &addrlen);
    if(c<0){
        fprintf(stderr,"accept() error");
        return 0;
    }

    return c;
}

char* cli_read(int c){
    static char buf[512];

    memset(buf, 0, 512);
    int r = read(c, buf, 511);
    if(r < 0){
        fprintf(stderr, "read() error\n");
        return 0;
    }else{
        return buf;
    }

}

int cli_headers(short HTTP_CODE,int client){
    char buf[512];

    snprintf(buf,511,"HTTP/1.0 %d\r\nDate: Sat, 09 Oct 2010 14:28:02 GMT\r\nServer: DuLeHttpd(cool thing btw)\r\nLast-Modified: Tue, 01 Dec 2009 20:18:22 GMT\r\nCache-Control: no-store, no-cache, max-age=0, private\r\nContent-Language: en\r\nExpires: -1\r\nX-Frame-Options: SAMEORIGIN\r\n",HTTP_CODE);
    

    int n = strlen(buf);
    write(client, buf, n);
    return 0;
}

int cli_body(int socket,int client,struct DuLeFile* file,char* mime,struct config* cfg){
    int n;
    n = file->size + strlen(mime) + 128;
    char* buf;
    buf = malloc(n);
    
    // memset(buf,0,n);
    snprintf(buf, n-1, "Content-Type: %s; charset=utf-8\r\nContent-Length: %llu\r\n\r\n",mime,file->size);
    
    write(client, buf, strlen(buf));

    if (cfg->show_debug_output == 2) fprintf(stderr, "write buffet(http headers,cli_body())\n");
    write(client, file->body,file->size);

    if (cfg->show_debug_output == 2) fprintf(stderr, "write file->body\n");
    free(buf);

    if (cfg->show_debug_output == 2) fprintf(stderr, "free buffet\n");
    return 1;
}

int cli_write(int socket,int client,int http_error_code,struct config* cfg){
    cli_headers(http_error_code,client);

    if (cfg->show_debug_output == 2) printf("popaaaaZ\n");
    
    if ((http_error_code == 404) && (cfg->file_at_404)) {
        struct DuLeFile* f;

        f = read_file(cfg, cfg->file_at_404);
        cli_body(socket, client, f, "text/html", cfg);
        
        free(f->body);
        free(f);
        return 0;
    }
    // Allocate sufficient room for the full HTML response + integers + '\0'
    char default_error_page[512];
    

    memset(default_error_page, 0, sizeof(default_error_page));

    // Pass sizeof(...) so snprintf uses the full allocated space safely
    snprintf(default_error_page, sizeof(default_error_page)-1,
        "<!DOCTYPE html><html><head><title>Error occurred on this website</title></head>"
        "<body style=\"font-family: Arial;\"><h1><strong>%d</strong></h1><a href=\"/\">/</a><br><p><em>Error occurred. Served by: "
        "<span style=\"color: green\">Du</span><span style=\"color: blue\">Le</span>Httpd</em></p></body></html>",
        http_error_code);
    
    struct DuLeFile* file;
    file = malloc(sizeof(struct DuLeFile));
    file->size = strlen(default_error_page);

    file->body = (char*)malloc(file->size+1);
    memset(file->body, 0, file->size+1);
    strncpy(file->body,default_error_page,file->size);

    cli_body(socket,client,file,"text/html",cfg);
    free(file->body);
    free(file);
    return 0;
};

int cli_response(int socket,int client,struct config* conf,struct http_req* req){
    if (strcmp(req->method,"GET")) {
        cli_write(socket,client,405,conf);
        return 0;
    }
    if(contains_upper_dir(req->uri)){
        cli_write(socket,client,400,conf);
        return 0;
    }
    if ('/' != req->uri[0]) {
        cli_write(socket, client, 400, conf);
    }
    if (!strcmp(req->uri,"/")) {
        req->uri = "/index.html";
    }
    fprintf(stderr, "file allowance\n");
    int allowed;
    allowed = is_file_allowed(req->uri,conf);
    if (!allowed) {
        cli_write(socket,client,403,conf);
        return 0;
    }
    if (conf->show_debug_output == 2) fprintf(stderr, "Responsing fine\n");

    struct DuLeFile* file = read_file(conf,req->uri);
    char* file_body = file->body;

    if (conf->show_debug_output == 2) fprintf(stderr, "REaded\n");

    // Файла не существует либо он пустой
    if (file_body == NULL || file->size == 0) {
        if (conf->show_debug_output == 2) fprintf(stderr, "IT'S NULL\n");
        cli_write(socket,client,404,conf);
        return 0;
    }
    if (conf->show_debug_output == 2) fprintf(stderr, "File checked\n");
    

    char* file_mime;
    file_mime = (char*)get_mime_type(conf,req->uri); 

    if (conf->show_debug_output == 2) fprintf(stderr, "We are getting right to the body\n");


    cli_headers(200, client);
    cli_body(socket,client,file,file_mime,conf);
    free(file->body);
    free(file);
    return 1;
};

int cli_connection(int socket,int client,struct config* conf){
    struct http_req* req;
    char* p;
    char* res;
    
    p = cli_read(client);
    if (!p) {
        fprintf(stderr, "Error in reading client\n");
        close(client);
        return 0;
    }
    if (conf->show_debug_output == 2)  fprintf(stderr, "Client readed\n");

    req = (struct http_req*)malloc(sizeof(struct http_req));
    int phr = parse_http_request(&req, p,conf->show_debug_output);

    if (!phr) {
        if(conf->show_debug_output > 0)fprintf(stderr, "Error: some unexpected request happend and it couldnt be handled. Dropping it.\n");
        close(client);
        return 0;
    }

    if (conf->show_debug_output == 2) fprintf(stderr, "Привет мир\n");
    
    int cli_res = cli_response(socket,client,conf,req);

    // free(req->uri);
    free(req);
    return 1;
};
#endif