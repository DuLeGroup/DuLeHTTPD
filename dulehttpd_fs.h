#ifndef DULEHTTPD_FS_H
#define DULEHTTPD_FS_H
#include "dulehttpd_config.h"

#ifdef __linux__
    #include <ctype.h>
    #include <stdio.h>
    #include <string.h>
    #include <stdbool.h>
    #include <stdlib.h>
    #include <stdbool.h>
    // Иначе stat() пизда
    #define _FILE_OFFSET_BITS 64
    #include <sys/stat.h>
#endif

struct DuLeFile{
    long long size;
    char* body;
};




int contains_upper_dir(char* uri){
    char* p,*str;
    for (p = uri;*p;p+=2){
        int cond = 0;
        if (*p == '.') {
            cond++;

            // for cases like /file/./{...}
            if (*(p+1) == '/' && *(p-1) == '/') {
                cond++;
            }
        }
        if (*(p+1) == '.') {
            cond++;
            p--;
        }
        if(cond == 2) return 1;
    };
    return 0;
}


/**/

// Вспомогательная функция для удаления начальных и конечных слэшей
void trim_slashes(const char *src, char *dst, size_t dst_size) {
    size_t start = 0;
    size_t len = strlen(src);

    // Пропускаем начальные слэши
    while (start < len && src[start] == '/') {
        start++;
    }

    // Игнорируем конечные слэши
    while (len > start && src[len - 1] == '/') {
        len--;
    }

    size_t new_len = len - start;
    if (new_len >= dst_size) {
        new_len = dst_size - 1;
    }

    if (new_len > 0) {
        memcpy(dst, src + start, new_len);
    }
    dst[new_len] = '\0';
}

/**
 * Сверяет target_path со списком путей, разделенных "///".
 * Возвращает true, если путь найден в шаблоне.
 */
bool match_relative_path(const char *target_path, const char *patterns_str) {
    char clean_target[1024];
    trim_slashes(target_path, clean_target, sizeof(clean_target));

    // Создаем копию строки шаблонов, так как strtok меняет исходную строку
    char *patterns_copy = strdup(patterns_str);
    if (!patterns_copy) return false;

    bool match_found = false;
    const char *delimiter = "///";
    char *token = strtok(patterns_copy, delimiter);

    while (token != NULL) {
        char clean_token[1024];
        trim_slashes(token, clean_token, sizeof(clean_token));

        // Сравниваем очищенные пути
        if (strcmp(clean_target, clean_token) == 0) {
            match_found = true;
            break;
        }

        token = strtok(NULL, delimiter);
    }

    free(patterns_copy);
    return match_found;
}
/**/

int is_file_allowed(char* uri,struct config* cfg){    
    char* forbidden_all;
    if (cfg->forbidden_files && cfg->forbidden_directories) {
        snprintf(forbidden_all, sizeof(cfg->forbidden_files) + sizeof(cfg->forbidden_directories) - 1,"%s///%s",cfg->forbidden_files,cfg->forbidden_directories);
    }else if (cfg->is_forbidden_directories) {
        forbidden_all = cfg->forbidden_directories;
    }else if (cfg->is_forbidden_files) {
        forbidden_all = cfg->forbidden_files;
    }else{
        return 1;
    }
    
    return !match_relative_path(uri,forbidden_all);
};

struct DuLeFile* read_file(struct config* cfg,char* path){
    // return data
    struct DuLeFile* dulefile;
    dulefile = malloc(sizeof(struct DuLeFile));
    dulefile->size = 0;
    dulefile->body = NULL;

    int n = strlen(cfg->root_directory)+strlen(path);
    char* full_path = malloc(n+2);

    full_path[n+1] = '\0';
    snprintf(full_path, n+1, "%s%s",cfg->root_directory,path);

    if(cfg->show_debug_output == 2)    fprintf(stderr, "checking file: %s\n%s:%s\n",full_path,cfg->root_directory,path);

    struct stat st;
    if (stat(full_path, &st)) {
        if (cfg->show_debug_output >= 1) fprintf(stderr,"error stat()\n");
        free(full_path);
        return dulefile;        
    }
    long long size = st.st_size;

    if (size > cfg->max_body_size) {
        if(cfg->show_debug_output >= 1) fprintf(stderr, "File(%s) size is(%ldB) more than maximum allowed(%lluB)",full_path,st.st_size,cfg->max_body_size);
        free(full_path);
        return dulefile; 
    }

    dulefile->size = size;
    if (cfg->show_debug_output == 2) fprintf(stderr, "%llu\n",size);

    FILE* f = fopen(full_path,"rb");
    

    char* file_body;
    file_body = (char*)malloc(size+1);
    size_t bytes_read = fread(file_body, 1, size, f);
    file_body[bytes_read] = '\0';
    
    fclose(f);
    dulefile->body = file_body;
    free(full_path);
    return dulefile;
};




// Структура для сопоставления расширения и MIME-типа
typedef struct {
    const char *ext;
    const char *mime;
} MimeMap;

// Карта популярных расширений
static const MimeMap mime_types[] = {
    // Текст / Веб
    {".html", "text/html"},
    {".htm",  "text/html"},
    {".css",  "text/css"},
    {".js",   "text/javascript"},
    {".json", "application/json"},
    {".xml",  "application/xml"},
    {".txt",  "text/plain"},
    {".csv",  "text/csv"},

    // Изображения
    {".png",  "image/png"},
    {".jpg",  "image/jpeg"},
    {".jpeg", "image/jpeg"},
    {".gif",  "image/gif"},
    {".svg",  "image/svg+xml"},
    {".webp", "image/webp"},
    {".ico",  "image/x-icon"},

    // Аудио / Видео
    {".mp3",  "audio/mpeg"},
    {".wav",  "audio/wav"},
    {".mp4",  "video/mp4"},
    {".webm", "video/webm"},

    // Архивы и документы
    {".pdf",  "application/pdf"},
    {".zip",  "application/zip"},
    {".tar",  "application/x-tar"},
    {".gz",   "application/gzip"}
};

#define MIME_MAP_SIZE (sizeof(mime_types) / sizeof(mime_types[0]))

/**
 * Определяет MIME-тип файла по его пути.
 * Возвращает строку с MIME-типом или "application/octet-stream", если тип не найден.
 */
const char* get_mime_type(struct config* cfg,char* path) {
    char* full_path;
    snprintf(full_path, strlen(cfg->root_directory)+strlen(path)+1, "%s%s",cfg->root_directory,path);

    if (!full_path) {
        return "application/octet-stream";
    }

    // Ищем последнюю точку в пути
    const char *ext = strrchr(full_path, '.');
    
    // Если точки нет или она является символом разделения директорий
    if (!ext || strchr(ext, '/') || strchr(ext, '\\')) {
        return "application/octet-stream"; // Тип по умолчанию для бинарных данных
    }

    // Переводим расширение в нижний регистр для регистронезависимого сравнения
    char lower_ext[16];
    size_t i = 0;
    while (ext[i] && i < sizeof(lower_ext) - 1) {
        lower_ext[i] = (char)tolower((unsigned char)ext[i]);
        i++;
    }
    lower_ext[i] = '\0';

    // Поиск по таблице
    for (size_t j = 0; j < MIME_MAP_SIZE; j++) {
        if (strcmp(lower_ext, mime_types[j].ext) == 0) {
            return mime_types[j].mime;
        }
    }

    // Если расширение неизвестно
    return "application/octet-stream";
}

#endif