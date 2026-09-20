#ifndef DULEHTTPD_FS_H
#define DULEHTTPD_FS_H
#include "dulehttpd_config.h"

#ifdef __linux__
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// Иначе stat() плохо
#define _FILE_OFFSET_BITS 64
#include <sys/stat.h>
#endif

struct DuLeFile {
    long long size;
    char* body;
};

int contains_upper_dir(char* uri) {
    char *p, *str;
    for (p = uri; *p; p += 2) {
        int cond = 0;
        if (*p == '.') {
            cond++;

            // for cases like /file/./{...}
            if (*(p + 1) == '/' && *(p - 1) == '/') {
                cond++;
            }
        }
        if (*(p + 1) == '.') {
            cond++;
            p--;
        }
        if (cond == 2)
            return 1;
    };
    return 0;
}

int is_file_allowed(char* uri, struct config* cfg) {
    if (!(cfg->is_forbidden_files || cfg->is_forbidden_directories)) {
        return 1;
    }

    int is_allowed;
    if (cfg->is_forbidden_files) {
        char* ff;
        ff = malloc(strlen(cfg->forbidden_files) + 1);
        strcpy(ff, cfg->forbidden_files);

        char* p;
        char* str;
        int length;
        p = ff;
        str = p;
        for (p = ff; *p && *p != ' ';
             p += 1 + 0 * (fprintf(stderr, "*p=%c\n", *p))) {
            if ((*p == '/' && *(p - 1) == '/' && *(p + 1) == '/')) {
                // Точка разделения
                if ( uri[strlen(uri) - 1] == '/') {
                    // В str оставляем /файл/
                    char tmp_p = *p;
                    *p = 0;
                    short is_str_last_symbol_slash = tmp_p == '/';

                    is_allowed = strcmp(uri, str);

                    fprintf(stderr, "uri'%s' str'%s',result: %d\n", uri, str,is_allowed);
                    *p = '/';
                } else {
                    // В стр оставляем /файл <- БЕЗ СЛЭША
                    *(p - 1) = 0;
                    is_allowed = strcmp(uri, str);

                    fprintf(stderr, "uri'%s' str'%s',result: %d\n", uri, str,is_allowed);
                    *(p - 1) = '/';
                }
                p++;
                str = p;
                if (!is_allowed) {
                    return is_allowed;
                }
            }
        }
        // Последний в /file_a///file_b || /file_a///file_b/
        char tmp_p = *p;
        *p = 0;
        short is_uri_last_symbol_slash = uri[strlen(uri) - 1] == '/';
        short is_str_last_symbol_slash = tmp_p == '/';
        if (cfg->show_debug_output == 2)
            fprintf(stderr, "'%s'(%d);'%s'(%d)", str, is_str_last_symbol_slash,
                    uri, is_uri_last_symbol_slash);
        if (is_str_last_symbol_slash == is_uri_last_symbol_slash) {
            is_allowed = strcmp(str, uri);
        } else if (is_str_last_symbol_slash) {
            // '/' + '\0'
            char* tmp_str = malloc(strlen(str + 2));
            snprintf(tmp_str, strlen(str + 1), "%s/", str);
            is_allowed = strcmp(tmp_str, uri);
            if (cfg->show_debug_output)
                fprintf(stderr, "tmp_str:'%s',uri:'%s'", tmp_str, uri);
            free(tmp_str);
        } else if (is_uri_last_symbol_slash) {
            is_allowed = 0;
            if (cfg->show_debug_output == 2)
                fprintf(stderr, "Is_allowed th\n");
        } else {
            if (cfg->show_debug_output == 2)
                fprintf(stderr, "Is_allowed th\n");
            return 0;
        }
    }
    return is_allowed;
};

struct DuLeFile* read_file(struct config* cfg, char* path) {
    // return data
    struct DuLeFile* dulefile;
    dulefile = malloc(sizeof(struct DuLeFile));
    dulefile->size = 0;
    dulefile->body = NULL;

    int n = strlen(cfg->root_directory) + strlen(path);
    char* full_path = malloc(n + 2);

    full_path[n + 1] = '\0';
    snprintf(full_path, n + 1, "%s%s", cfg->root_directory, path);

    if (cfg->show_debug_output == 2)
        fprintf(stderr, "checking file: %s\n%s:%s\n", full_path,
                cfg->root_directory, path);

    struct stat st;
    if (stat(full_path, &st)) {
        if (cfg->show_debug_output >= 1)
            fprintf(stderr, "error stat()\n");
        free(full_path);
        return dulefile;
    }
    long long size = st.st_size;

    if (size > cfg->max_body_size) {
        if (cfg->show_debug_output >= 1)
            fprintf(stderr,
                    "File(%s) size is(%ldB) more than maximum allowed(%lluB)",
                    full_path, st.st_size, cfg->max_body_size);
        free(full_path);
        return dulefile;
    }

    dulefile->size = size;
    if (cfg->show_debug_output == 2)
        fprintf(stderr, "%llu\n", size);

    FILE* f = fopen(full_path, "rb");

    char* file_body;
    file_body = (char*)malloc(size + 1);
    size_t bytes_read = fread(file_body, 1, size, f);
    file_body[bytes_read] = '\0';

    fclose(f);
    dulefile->body = file_body;
    free(full_path);
    return dulefile;
};

// Структура для сопоставления расширения и MIME-типа
typedef struct {
    const char* ext;
    const char* mime;
} MimeMap;

// Карта популярных расширений
static const MimeMap mime_types[] = {
    // Текст / Веб
    {".html", "text/html"},
    {".htm", "text/html"},
    {".css", "text/css"},
    {".js", "text/javascript"},
    {".json", "application/json"},
    {".xml", "application/xml"},
    {".txt", "text/plain"},
    {".csv", "text/csv"},

    // Изображения
    {".png", "image/png"},
    {".jpg", "image/jpeg"},
    {".jpeg", "image/jpeg"},
    {".gif", "image/gif"},
    {".svg", "image/svg+xml"},
    {".webp", "image/webp"},
    {".ico", "image/x-icon"},

    // Аудио / Видео
    {".mp3", "audio/mpeg"},
    {".wav", "audio/wav"},
    {".mp4", "video/mp4"},
    {".webm", "video/webm"},

    // Архивы и документы
    {".pdf", "application/pdf"},
    {".zip", "application/zip"},
    {".tar", "application/x-tar"},
    {".gz", "application/gzip"}};

#define MIME_MAP_SIZE (sizeof(mime_types) / sizeof(mime_types[0]))

/**
 * Определяет MIME-тип файла по его пути.
 * Возвращает строку с MIME-типом или "application/octet-stream", если тип не
 * найден.
 */
const char* get_mime_type(struct config* cfg, char* path) {
    char* full_path;
    snprintf(full_path, strlen(cfg->root_directory) + strlen(path) + 1, "%s%s",
             cfg->root_directory, path);

    if (!full_path) {
        return "application/octet-stream";
    }

    // Ищем последнюю точку в пути
    const char* ext = strrchr(full_path, '.');

    // Если точки нет или она является символом разделения директорий
    if (!ext || strchr(ext, '/') || strchr(ext, '\\')) {
        return "application/octet-stream"; // Тип по умолчанию для бинарных
                                           // данных
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