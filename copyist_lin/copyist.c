#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <libgen.h>
#include <sys/stat.h>
#include <errno.h>

int main() {
    //структура, которая будет содержать информацию о найденных файлах
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
        //Не будем копировать директории и не будем копировать скрытые файлы
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        //Получаем информацию о файле для проверки типа
        if (stat(entry->d_name, &file_stat) == -1) {
            continue;
        }

        //Проверяем, что это не директория и не скрытый файл
        if (!S_ISDIR(file_stat.st_mode) && entry->d_name[0] != '.') {

            //Так же не будем копировать уже скопированные файлы
            if (strncmp(entry->d_name, "copy_", 5) == 0) {
                continue;
            }

            //И не будем копировать текущий исполняемый файл
            if (strcmp(entry->d_name, currentFileName) == 0) {
                continue;
            }

            //создаем новое имя файла, и пытаемся создать с ним копию
            char newName[4096];
            snprintf(newName, sizeof(newName), "copy_%s", entry->d_name);

            //FALSE не позволит перезаписать файл (используем "xb" - эксклюзивное создание)
            FILE *src = fopen(entry->d_name, "rb");
            if (src == NULL) {
                printf("Failed to open source: %s\n", entry->d_name);
                continue;
            }

            FILE *dst = fopen(newName, "xb"); // 'x' - не перезаписывать существующий файл
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