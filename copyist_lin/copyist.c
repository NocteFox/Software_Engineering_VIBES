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
    //Дескриптор (уникальный идентификатор) поиска
    DIR *dp = opendir(".");

    if (dp == NULL) {
        printf("Error opening directory\n");
        return 1;
    }

    //Массивы символов для полного пути и для названия файла
    char currentModule[4096];
    char currentFileName[4096];

    //Получаем полный путь
    ssize_t len = readlink("/proc/self/exe", currentModule, sizeof(currentModule) - 1);
    if (len != -1) {
        currentModule[len] = '\0';
    } else {
        printf("Error getting module path\n");
        closedir(dp);
        return 1;
    }

    //Создаем указатель, который будет хранить только название файла, и копируем название в currentFileName
    char* fileName = basename(currentModule);
    strcpy(currentFileName, fileName);

    while ((entry = readdir(dp)) != NULL) {
        //Пропускаем текущую и родительскую директории
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        //Получаем информацию о файле для проверки типа
        if (stat(entry->d_name, &file_stat) == -1) {
            continue;
        }

        //Копируем только обычные файлы (не директории) и не скрытые
        if (!S_ISDIR(file_stat.st_mode) && entry->d_name[0] != '.') {

            //Пропускаем уже скопированные файлы
            if (strncmp(entry->d_name, "copy_", 5) == 0) {
                continue;
            }

            //Пропускаем текущий исполняемый файл
            if (strcmp(entry->d_name, currentFileName) == 0) {
                continue;
            }

            //Создаем новое имя для копии
            char newName[4096];
            snprintf(newName, sizeof(newName), "copy_%s", entry->d_name);

            //Проверяем, не существует ли уже файл с таким именем (чтобы не перезаписать)
            FILE *src = fopen(entry->d_name, "rb");
            if (src == NULL) {
                printf("Failed to open source: %s\n", entry->d_name);
                continue;
            }

            FILE *test = fopen(newName, "rb");
            if (test != NULL) {
                printf("Destination already exists: %s\n", newName);
                fclose(test);
                fclose(src);
                continue;
            }

            //Создаем файл для записи копии
            FILE *dst = fopen(newName, "wb");
            if (dst == NULL) {
                printf("Failed to create destination: %s (error: %s)\n",
                       newName, strerror(errno));
                fclose(src);
                continue;
            }

            //Буфер для копирования данных
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

    //Важно не забыть закрыть процесс поиска
    closedir(dp);
    return 0;
}