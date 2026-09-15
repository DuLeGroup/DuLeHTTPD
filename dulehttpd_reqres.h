#ifndef DULEHTTPD_REQRES_H
#define DULEHTTPD_REQRES_H

#ifdef __linux__
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
#endif

struct http_req{
    char method[8];
    char* uri;
};

int parse_http_request(struct http_req** req,char* str,short debug){
    if (debug == 2)  fprintf(stderr, "entring parsing\n");
    char* p;

    for(p=str;*p && *p != ' ';p++);
    if (*p == ' ') {
        *p = 0;
    }else{
        if (debug > 0)        fprintf(stderr, "NOSPACE Error in request.bb\n");
        return 0;
    }

    strncpy((*req)->method, str, 7);

    int uri_length = 0;
    
    for(str=++p; *p && *p != ' '; p++){uri_length++;};
    if (*p == ' ') {
        *p = 0;
    }else{
        if (debug > 0) fprintf(stderr,"parse_http() 2nd NOSPACE error");
        return 0;
    }
    
    str[uri_length] = '\0';

    if (debug == 2) fprintf(stderr, "Ended up parsing\n");
    if(debug == 2)    fprintf(stderr, "'%s'",str);
    (*req)->uri = (char*)malloc(uri_length+1);  
    memset((*req)->uri,0,uri_length+1);
    strncpy((*req)->uri, &(*str), uri_length);
    // strcpy((*req)->uri, str);


    if (debug == 2)        printf("request parsed\nMethod: '%s'\nURI: '%s'\n",(*req)->method,(*req)->uri); 
    return 1;
    
}   
#endif