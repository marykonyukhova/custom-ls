#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "src/vector.h"
#include "src/ls.h"

void InitListArgs(ListArgs *args) {
    args->all = false;
    args->almostAll = false;
    args->ignoreBackups = false;
    args->directory = false;
    args->dereference = false;
    args->humanReadable = false;
    args->si = false;
    args->size = false;
    args->reverse = false;
    args->longFormat = false;
    args->sort = SORT_NONE;
}


// Освобождение всех путей в векторе
void FreePaths(GenericVector *paths) {
    if (!paths) return;
    FreeGenericVector(paths);
}


int main(int argc, char **argv) {
    ListArgs args;
    InitListArgs(&args);

    // Вектор для хранения путей
    GenericVector *paths = NewGenericVector(1);
    if (!paths) {
        fprintf(stderr, "Failed to allocate memory for paths vector.\n");
        return EXIT_FAILURE;
    }

    bool pathProvided = false;

    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            if ((strcmp(argv[i], "--all") == 0) || (strcmp(argv[i], "-a") == 0)) {
                args.all = true;
            } else if ((strcmp(argv[i], "--almost-all") == 0) || (strcmp(argv[i], "-A") == 0)) {
                args.almostAll = true;
            } else if ((strcmp(argv[i], "--ignore-backups") == 0) || (strcmp(argv[i], "-B") == 0)) {
                args.ignoreBackups = true;
            } else if ((strcmp(argv[i], "--directory") == 0) || (strcmp(argv[i], "-d") == 0)) {
                args.directory = true;
            } else if ((strcmp(argv[i], "--dereference") == 0) || (strcmp(argv[i], "-L") == 0)) {
                args.dereference = true;
            } else if ((strcmp(argv[i], "--human-readable") == 0) || (strcmp(argv[i], "-h") == 0)) {
                args.humanReadable = true;
            } else if (strcmp(argv[i], "--si") == 0) {
                args.si = true;
            } else if ((strcmp(argv[i], "--size") == 0) || (strcmp(argv[i], "-s") == 0)) {
                args.size = true;
            } else if ((strcmp(argv[i], "--sort") == 0) && (i + 1) < argc) {
                if (strcmp(argv[i + 1], "size") == 0) {
                    args.sort = SORT_SIZE;
                } else if (strcmp(argv[i + 1], "time") == 0) {
                    args.sort = SORT_TIME;
                } else if (strcmp(argv[i + 1], "none") == 0) {
                    args.sort = SORT_NONE;
                } else {
                    fprintf(stderr, "Unknown sort option: %s\n", argv[i + 1]);
                    FreePaths(paths);
                    return EXIT_FAILURE;
                }
                i++;
            } else if ((strcmp(argv[i], "--reverse") == 0) || (strcmp(argv[i], "-r") == 0)) {
                args.reverse = true;
            } else if (strcmp(argv[i], "-S") == 0) {
                args.sort = SORT_SIZE;
            } else if (strcmp(argv[i], "-t") == 0) {
                args.sort = SORT_TIME;
            } else if (strcmp(argv[i], "-U") == 0) {
                args.sort = SORT_NONE;
            } else if (strcmp(argv[i], "-l") == 0) {
                args.longFormat = true;
            } else {
                fprintf(stderr, "Unknown argument: %s\n", argv[i]);
                FreePaths(paths);
                return EXIT_FAILURE;
            }
        } else {
            // Считаем все остальные аргументы путями
            char *path = strdup(argv[i]);
            if (!path) {
                fprintf(stderr, "Failed to allocate memory for path.\n");
                FreePaths(paths);
                return EXIT_FAILURE;
            }
            Append(paths, path);
            pathProvided = true;
        }
    }

    // Если не был указан путь, используем текущую директорию
    if (!pathProvided) {
        char *defaultPath = strdup(".");
        if (!defaultPath) {
            fprintf(stderr, "Failed to allocate memory for default path.\n");
            FreePaths(paths);
            return EXIT_FAILURE;
        }
        Append(paths, defaultPath);
    }

    // Выполняем глоббинг
    ExpandPathsWithGlob(paths);
    
    // Вызов функции для обработки путей
    ListErrorCode result = ListPaths(paths, &args, stdout);
    if (result != LIST_SUCCESS) {
        fprintf(stderr, "Error occurred during listing: %d\n", result);
        FreePaths(paths);
        return EXIT_FAILURE;
    }

    FreePaths(paths); // Освобождение всех путей и самого вектора
    return 0;
}
