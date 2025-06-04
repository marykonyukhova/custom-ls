#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <dirent.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <unistd.h>
#include <math.h>
#include <libgen.h>
#include <ctype.h>

#include "vector.h"
#include "ls.h"

typedef struct FileEntry {
    char name[1024];
    struct stat statbuf;
} FileEntry;

typedef struct {
    size_t block_width;
    size_t link_width;
    size_t user_width;
    size_t group_width;
    size_t size_width;
} ColumnWidths;

// Сортировка по времени последней модификации, при равенстве - по имени
int CompareByTime(const void *a, const void *b) {
    const FileEntry *entryA = (const FileEntry *)a;
    const FileEntry *entryB = (const FileEntry *)b;

    if (entryA->statbuf.st_mtime != entryB->statbuf.st_mtime) {
        return (entryB->statbuf.st_mtime - entryA->statbuf.st_mtime);
    }
    return strcmp(entryA->name, entryB->name);
}

// Сортировка по размеру, при равенстве - по имени
int CompareBySize(const void *a, const void *b) {
    const FileEntry *entryA = (const FileEntry *)a;
    const FileEntry *entryB = (const FileEntry *)b;

    if (entryA->statbuf.st_size != entryB->statbuf.st_size) {
        return (entryB->statbuf.st_size - entryA->statbuf.st_size);
    }
    return strcmp(entryA->name, entryB->name);
}

// Сортировка по имени с учетом регистра
int CompareByName(const void *a, const void *b) {
    const FileEntry *entryA = (const FileEntry *)a;
    const FileEntry *entryB = (const FileEntry *)b;

    return strcmp(entryA->name, entryB->name);
}


void FormatSize(char *buf, size_t bufsize, off_t size, bool human_readable, bool si) {
    double formattedSize = (double)size;
    int divisor = si ? 1000 : 1024;
    const char *units[] = {"", si ? "k" : "K", "M", "G", "T"};
    int unit_index = 0;

    while (formattedSize >= divisor && unit_index < (sizeof(units) / sizeof(units[0])) - 1) {
        formattedSize /= divisor;
        unit_index++;
    }

    if (formattedSize >= 10) {
        snprintf(buf, bufsize, "%.0f%s", formattedSize, units[unit_index]); // NOLINT(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
    } else {
        formattedSize = ((double)(formattedSize * 10 + 0.5)) / 10.0;
        if ((formattedSize - floor(formattedSize)) < 0.100000) {
            if (unit_index == 0) {
                snprintf(buf, bufsize, "%.0f%s", formattedSize, units[unit_index]); // NOLINT(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
            } else {
                snprintf(buf, bufsize, "%.1f%s", formattedSize, units[unit_index]); // NOLINT(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
            }
        } else {
            snprintf(buf, bufsize, "%.1f%s", formattedSize, units[unit_index]); // NOLINT(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
        }

    }
}


void CalculateMaxWidths(const FileEntry *entries, size_t entry_count, ColumnWidths *widths, const ListArgs *args) {
    widths->block_width = 0;
    widths->link_width = 0;
    widths->user_width = 0;
    widths->group_width = 0;
    widths->size_width = 0;

    for (size_t i = 0; i < entry_count; i++) {
        const struct stat *entry_stat = &entries[i].statbuf;

        // Размер блока
        if (args->size) {
            long blocks = entry_stat->st_blocks;
            char block_str[20];
            if (args->humanReadable || args->si) {
                FormatSize(block_str, sizeof(block_str), blocks * 512, args->humanReadable, args->si);

                size_t block_len = strlen(block_str);
                if (block_len > widths->block_width) {
                    widths->block_width = block_len;
                }
            } else {
                size_t block_len = (size_t)snprintf(NULL, 0, "%ld", entry_stat->st_blocks); // NOLINT(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
                if (block_len > widths->block_width) {
                    widths->block_width = block_len;
                }
            }
        }

        // Количество ссылок
        size_t link_len = (size_t)snprintf(NULL, 0, "%lu", entry_stat->st_nlink); // NOLINT(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
        if (link_len > widths->link_width) {
            widths->link_width = link_len;
        }

        // Имя пользователя
        struct passwd *pwd = getpwuid(entry_stat->st_uid);
        size_t user_len = strlen(pwd ? pwd->pw_name : "?");
        if (user_len > widths->user_width) {
            widths->user_width = user_len;
        }

        // Имя группы
        struct group *grp = getgrgid(entry_stat->st_gid);
        size_t group_len = strlen(grp ? grp->gr_name : "?");
        if (group_len > widths->group_width) {
            widths->group_width = group_len;
        }

        // Размер файла
        char size_str[20];
        if (args->humanReadable || args->si) {
            FormatSize(size_str, sizeof(size_str), entry_stat->st_size, args->humanReadable, args->si);
        
            size_t size_len = strlen(size_str);
            if (size_len > widths->size_width) {
                widths->size_width = size_len;
            }
        } else {
            size_t size_len = (size_t)snprintf(NULL, 0, "%ld", entry_stat->st_size); // NOLINT(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
            if (size_len > widths->size_width) {
                widths->size_width = size_len;
            }
        }
    }
}


void PrintLongFormat(FILE *out, char *path, const struct stat *entry_stat, const ListArgs *args, const ColumnWidths *widths) {

    if (args->size && !args->longFormat) {
        long blocks = entry_stat->st_blocks;
        char block_str[20];
        if (args->humanReadable || args->si) {
            FormatSize(block_str, sizeof(block_str), blocks * 512, args->humanReadable, args->si);
            fprintf(out, "%*s ", (int)widths->block_width, block_str);
        } else {
            fprintf(out, "%*ld ", (int)widths->block_width, entry_stat->st_blocks/2);
        }

        if (S_ISDIR(entry_stat->st_mode)) {
            if ((out != stdout) && (out != stderr)) {
                if (args->directory) {fprintf(out, "%s\n", path);}
                else {fprintf(out, "%s\n", basename(path));}
            } else {
                if (args->directory) {fprintf(out, "\033[36m%s\033[0m\n", path);}
                else {fprintf(out, "\033[36m%s\033[0m\n", basename(path));}
            }
        } else {
            if (S_ISLNK(entry_stat->st_mode) && !args->dereference) {
                char link_target[1024];
                ssize_t len = readlink(path, link_target, sizeof(link_target) - 1);
                if (len != -1) {
                    link_target[len] = '\0';
                    fprintf(out, "\033[31m%s\033[0m -> \033[31m%s\033[0m\n", basename(path), link_target);
                } else {
                    perror("readlink");
                }
            } else {
                fprintf(out, "%s\n", basename(path));
            }
        }

    } else {
        if (args->size) {
            long blocks = entry_stat->st_blocks;
            char block_str[20];
            if (args->humanReadable || args->si) {
                FormatSize(block_str, sizeof(block_str), blocks * 512, args->humanReadable, args->si);
                fprintf(out, "%*s ", (int)widths->block_width, block_str);
            } else {
                fprintf(out, "%*ld ", (int)widths->block_width, entry_stat->st_blocks/2);
            }
        }
        
        fprintf(out, "%c", S_ISDIR(entry_stat->st_mode) ? 'd' : (S_ISLNK(entry_stat->st_mode) ? 'l' : '-'));
        fprintf(out, "%c", (entry_stat->st_mode & S_IRUSR) ? 'r' : '-');
        fprintf(out, "%c", (entry_stat->st_mode & S_IWUSR) ? 'w' : '-');
        fprintf(out, "%c", (entry_stat->st_mode & S_IXUSR) ? 'x' : '-');
        fprintf(out, "%c", (entry_stat->st_mode & S_IRGRP) ? 'r' : '-');
        fprintf(out, "%c", (entry_stat->st_mode & S_IWGRP) ? 'w' : '-');
        fprintf(out, "%c", (entry_stat->st_mode & S_IXGRP) ? 'x' : '-');
        fprintf(out, "%c", (entry_stat->st_mode & S_IROTH) ? 'r' : '-');
        fprintf(out, "%c", (entry_stat->st_mode & S_IWOTH) ? 'w' : '-');
        fprintf(out, "%c ", (entry_stat->st_mode & S_IXOTH) ? 'x' : '-');

        fprintf(out, "%*lu ", (int)widths->link_width, entry_stat->st_nlink);

        struct passwd *pwd = getpwuid(entry_stat->st_uid);
        fprintf(out, "%-*s ", (int)widths->user_width, pwd ? pwd->pw_name : "?");

        struct group *grp = getgrgid(entry_stat->st_gid);
        fprintf(out, "%-*s ", (int)widths->group_width, grp ? grp->gr_name : "?");

        char size_str[20];
        if (args->humanReadable || args->si) {
            FormatSize(size_str, sizeof(size_str), entry_stat->st_size, args->humanReadable, args->si);
            fprintf(out, "%*s ", (int)widths->size_width, size_str);
        } else {
            fprintf(out, "%*ld ", (int)widths->size_width, entry_stat->st_size);
        }

        char timeBuf[20];
        struct tm *timeinfo = localtime(&entry_stat->st_mtime);
        strftime(timeBuf, sizeof(timeBuf), "%b %e %H:%M", timeinfo);
        fprintf(out, "%s ", timeBuf);

        if (S_ISDIR(entry_stat->st_mode)) {
            if ((out != stdout) && (out != stderr)) {
                if (args->directory) {fprintf(out, "%s\n", path);}
                else {fprintf(out, "%s\n", basename(path));}
            } else {
                if (args->directory) {fprintf(out, "\033[36m%s\033[0m\n", path);}
                else {fprintf(out, "\033[36m%s\033[0m\n", basename(path));}
            }
        } else {
            if (S_ISLNK(entry_stat->st_mode) && !args->dereference) {
                char link_target[1024];
                ssize_t len = readlink(path, link_target, sizeof(link_target) - 1);
                if (len != -1) {
                    link_target[len] = '\0';
                    fprintf(out, "\033[31m%s\033[0m -> \033[31m%s\033[0m\n", basename(path), link_target);
                } else {
                    perror("readlink");
                }
            } else {
            fprintf(out, "%s\n", basename(path));
            }
        }
    }
}


// Проверяет совпадение строки с шаблоном (с `*` и `?`)
bool MatchPattern(const char* pattern, const char* str) {
    if (*pattern == '\0' && *str == '\0') {
        return true;
    }

    if (*pattern == '*') {
        while (*(pattern + 1) == '*') {
            pattern++;
        }
        
        // Если звездочка - последний символ
        if (*(pattern + 1) == '\0') {
            return true;
        }

        while (*str != '\0') {
            if (MatchPattern(pattern + 1, str)) {
                return true;
            }
            str++;
        }
        return MatchPattern(pattern + 1, str);
    }
    
    if (*str != '\0' && (*pattern == '?' || *pattern == *str)) {
        return MatchPattern(pattern + 1, str + 1);
    }
    
    return false;
}


// Глоббинг: находит файлы, соответствующие шаблону
int CustomGlob(const char *pattern, GenericVector *results) {
    char dir_path[1024];
    const char *file_pattern = strrchr(pattern, '/');
    if (file_pattern) {
        snprintf(dir_path, sizeof(dir_path), "%.*s", (int)(file_pattern - pattern), pattern); //NOLINT(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
        file_pattern++; // Шаблон файла
    } else {
        strcpy(dir_path, ".");
        file_pattern = pattern;
    }

    DIR *dir = opendir(dir_path);
    if (!dir) {
        fprintf(stderr, "Cannot open directory: %s\n", dir_path);
        return -1;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (MatchPattern(file_pattern, entry->d_name)) {
            size_t path_len = strlen(dir_path) + strlen(entry->d_name) + 2;
            char *full_path = malloc(path_len);
            if (!full_path) {
                fprintf(stderr, "Memory allocation failed\n");
                closedir(dir);
                return -1;
            }
            snprintf(full_path, path_len, "%s/%s", dir_path, entry->d_name); // NOLINT(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
            Append(results, full_path);
        }
    }

    closedir(dir);
    return 0;
}


// Проверка наличия шаблона в строке
bool ContainsGlobPattern(const char *path) {
    while (*path) {
        if (*path == '*' || *path == '?') {
            return true;
        }
        path++;
    }
    return false;
}

// Интерфейс для выполнения глоббинга перед обработкой путей
void ExpandPathsWithGlob(GenericVector *paths) {
    size_t original_length = GetLength(paths);
    GenericVector *expanded_paths = NewGenericVector(1);
    if (!expanded_paths) {
        fprintf(stderr, "Memory allocation failed for expanded_paths.\n");
        exit(EXIT_FAILURE); // Немедленный выход
    }

    for (size_t i = 0; i < original_length; i++) {
        char *path = (char *)GetElement(paths, i);
        if (ContainsGlobPattern(path)) {
            if (CustomGlob(path, expanded_paths) != 0) {
                fprintf(stderr, "Error in CustomGlob for path: %s\n", path);
                FreeGenericVector(expanded_paths);
                exit(EXIT_FAILURE);
            }
        } else {
            char *copy = strdup(path);
            if (!copy) {
                fprintf(stderr, "Memory allocation failed for path copy.\n");
                FreeGenericVector(expanded_paths);
                exit(EXIT_FAILURE);
            }
            Append(expanded_paths, copy);
        }
    }

    FreeGenericVector(paths);
    for (size_t i = 0; i < GetLength(expanded_paths); i++) {
        Append(paths, GetElement(expanded_paths, i));
    }

    FreeGenericVector(expanded_paths);
}


ListErrorCode ListPaths(const GenericVector* paths, const ListArgs* args, FILE* out) {
    for (size_t i = 0; i < GetLength(paths); i++) {
        char *path = (char*)GetElement(paths, i);
        struct stat path_stat;
        FileEntry *entries = NULL;
        size_t entry_count = 0;

        if (args->dereference) {
            if (stat(path, &path_stat) != 0) {
                fprintf(stderr, "Error retrieving info for %s\n", path);
                return LIST_ERR_STAT;
            }
        } else {
            if (lstat(path, &path_stat) != 0) {
                fprintf(stderr, "Error retrieving info for %s\n", path);
                return LIST_ERR_STAT;
            }
        }

        if (S_ISREG(path_stat.st_mode)) {
            // Обработка, если это файл
            if (args->size && !args->longFormat) {
                ColumnWidths widths = {1};
                PrintLongFormat(out, path, &path_stat, args, &widths);
            } else {
                if (args->longFormat) {
                    ColumnWidths widths = {1};
                    PrintLongFormat(out, path, &path_stat, args, &widths);
                } else {
                    fprintf(out, "%s\n", path);
                }
                continue;
            }

        } else if (S_ISDIR(path_stat.st_mode)) {
            // Обработка директории
            if (args->size && !args->longFormat) {
                ColumnWidths widths = {1}; 
                PrintLongFormat(out, path, &path_stat, args, &widths);
            } else {
                if (args->directory) {
                    if (args->longFormat) {
                        ColumnWidths widths = {1}; 
                        PrintLongFormat(out, path, &path_stat, args, &widths);
                    } else {
                        fprintf(out, "\033[36m%s\033[0m\n", path);
                    }
                    continue;
                }
            }

            DIR *dir = opendir(path);
            if (!dir) {
                fprintf(stderr, "Could not open directory: %s\n", path);
                return LIST_ERR_OPEN_DIR;
            }

            struct dirent *entry;
            while ((entry = readdir(dir)) != NULL) {
                if (!args->all && entry->d_name[0] == '.') continue;
                if (args->almostAll && (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)) continue;
                if (args->ignoreBackups && entry->d_name[strlen(entry->d_name) - 1] == '~') continue;

                FileEntry *temp_entries = realloc(entries, (entry_count + 1) * sizeof(FileEntry));
                if (!temp_entries) {
                    free(entries);  // Освобождение памяти, если realloc не удастся
                    fprintf(stderr, "Memory allocation failed\n");
                    closedir(dir);
                    return LIST_ERR_MEMORY;
                }
                entries = temp_entries;

                snprintf(entries[entry_count].name, sizeof(entries[entry_count].name), "%s/%s", path, entry->d_name); // NOLINT(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
                if (snprintf(entries[entry_count].name, sizeof(entries[entry_count].name), "%s/%s", path, entry->d_name) >= sizeof(entries[entry_count].name)) { // NOLINT(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
                    fprintf(stderr, "Filename too long: %s/%s\n", path, entry->d_name);
                    continue;
                }

                if (args->dereference) {
                    if (stat(entries[entry_count].name, &entries[entry_count].statbuf) != 0) {
                        fprintf(stderr, "Error retrieving info for %s\n", entries[entry_count].name);
                        continue;
                    }
                } else {
                    if (lstat(entries[entry_count].name, &entries[entry_count].statbuf) != 0) {
                        fprintf(stderr, "Error retrieving info for %s\n", entries[entry_count].name);
                        continue;
                    }
                }
                entry_count++;
            }
            closedir(dir);

            if (args->sort == SORT_TIME) {
                if (entries != NULL) {
                    qsort(entries, entry_count, sizeof(FileEntry), CompareByTime);
                }
            } else if (args->sort == SORT_SIZE) {
                if (entries != NULL) {
                    qsort(entries, entry_count, sizeof(FileEntry), CompareBySize);
                }
            } else {
                if (entries != NULL) {
                    qsort(entries, entry_count, sizeof(FileEntry), CompareByName);
                }
            }

            if (args->reverse) {
                for (size_t j = 0; j < entry_count / 2; j++) {
                    FileEntry temp = entries[j];
                    entries[j] = entries[entry_count - j - 1];
                    entries[entry_count - j - 1] = temp;
                }
            }

            int total = 0;
            for (size_t j = 0; j < entry_count; j++) {
                const struct stat *entry_stat = &entries[j].statbuf;
                total += entry_stat->st_blocks;
            }

            char total_buf[16];
            if (args->longFormat) {
                if (args->humanReadable || args->si) {
                    FormatSize(total_buf, sizeof(total_buf), total*512, args->humanReadable, args->si);
                    fprintf(out, "total %s\n", args->humanReadable || args->si ? total_buf : "");
                } else {
                    fprintf(out, "total %d\n", total/2);
                }
            }

            for (size_t j = 0; j < entry_count; j++) {
                struct stat *entry_stat = &entries[j].statbuf;
                ColumnWidths widths;
                CalculateMaxWidths(entries, entry_count, &widths, args);
                if (args->size && !args->longFormat) {
                    PrintLongFormat(out, entries[j].name, entry_stat, args, &widths);
                } else {
                    if (args->longFormat) {
                        PrintLongFormat(out, entries[j].name, entry_stat, args, &widths);
                    } else {
                        fprintf(out, "%s\n", basename(entries[j].name));
                    }
                }
            }
            free(entries);
        } else {
            fprintf(out, "%s\n", path);
        }
    }
    return LIST_SUCCESS;
}