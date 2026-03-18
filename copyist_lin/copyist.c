#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <libgen.h>
#include <sys/stat.h>
#include <errno.h>

int main() {
    struct dirent *entry;
    struct stat file_stat;
    DIR *dp = opendir(".");

    if (dp == NULL) {
        printf("Error opening directory\n");
        return 1;
    }

    char currentModule[4096];
    char currentFileName[4096];

    // Получаем полный путь
    ssize_t len = readlink("/proc/self/exe", currentModule, sizeof(currentModule) - 1);
    if (len != -1) {
        currentModule[len] = '\0';
    } else {
        printf("Error getting module path\n");
        closedir(dp);
        return 1;
    }

    // Получаем только имя файла
    char* fileName = basename(currentModule);
    strcpy(currentFileName, fileName);

    while ((entry = readdir(dp)) != NULL) {
        // Пропускаем . и ..
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        // Получаем информацию о файле
        if (stat(entry->d_name, &file_stat) == -1) {
            continue;
        }

        // Проверяем, что это не директория и не скрытый файл
        if (!S_ISDIR(file_stat.st_mode) && entry->d_name[0] != '.') {

            // Не копируем уже скопированные файлы
            if (strncmp(entry->d_name, "copy_", 5) == 0) {
                continue;
            }

            // Не копируем текущий исполняемый файл
            if (strcmp(entry->d_name, currentFileName) == 0) {
                continue;
            }

            // Создаем новое имя файла
            char newName[4096];
            snprintf(newName, sizeof(newName), "copy_%s", entry->d_name);

            // Копируем файл
            FILE *src = fopen(entry->d_name, "rb");
            if (src == NULL) {
                printf("Failed to open source: %s\n", entry->d_name);
                continue;
            }

            FILE *dst = fopen(newName, "xb"); // 'x' - не перезаписывать
            if (dst == NULL) {
                printf("Failed to create destination: %s (error: %s)\n",
                       newName, strerror(errno));
                fclose(src);
                continue;
            }

            // Буфер для копирования
            char buffer[8192];
            size_t bytes;
            while ((bytes = fread(buffer, 1, sizeof(buffer), src)) > 0) {
                fwrite(buffer, 1, bytes, dst);
            }

            fclose(src);
            fclose(dst);

            printf("Copied: %s -> %s\n", entry->d_name, newName);
        }
    }

    closedir(dp);
    return 0;
}
