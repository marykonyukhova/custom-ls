#pragma once

#include <stdio.h>
#include <stdbool.h>

#include "vector.h"

typedef struct ListArgs {
    bool all;
    bool almostAll;
    bool ignoreBackups;
    bool directory;
    bool dereference;
    bool humanReadable;
    bool si;
    bool size;
    bool reverse;     
    bool longFormat;
    enum {
        SORT_NONE,
        SORT_SIZE,
        SORT_TIME,
    } sort;
} ListArgs;

typedef enum ListErrorCode {
    LIST_SUCCESS,           // Успешное завершение
    LIST_ERR_OPEN_DIR,      // Ошибка открытия директории
    LIST_ERR_READ_DIR,      // Ошибка чтения директории
    LIST_ERR_INVALID_ARG,   // Неверные аргументы
    LIST_ERR_STAT,          // Ошибка получения информации о файле
    LIST_ERR_MEMORY         // Ошибка выделения памяти
} ListErrorCode;

/*
*  See the docs for the following functions and types:
*  - malloc, calloc, free
*  - opendir, readdir, closedir
*  - stat, lstat
*  - struct stat: S_ISDIR, S_ISCHR, S_ISBLK, S_ISLNK, S_ISFIFO, S_ISSOCK, getpwuid, getgrgid
*  - time_t: localtime, strftime
*  - snprintf
*  - qsort
*/
ListErrorCode ListPaths(const GenericVector* paths, const ListArgs* args, FILE* out);
void ExpandPathsWithGlob(GenericVector *paths);
