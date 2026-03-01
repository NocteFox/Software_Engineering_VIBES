#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <shlwapi.h>

int main() {
    //структура, которая будет содержать информацию о найденных файлах
    WIN32_FIND_DATAA findData;
    //Дескриптор (уникальный идентификатор) поиска
    HANDLE hFind = FindFirstFileA("*", &findData);

    if (hFind == INVALID_HANDLE_VALUE) {
        printf("Error opening directory\n");
        return 1;
    }

    //Массивы символов для полного пути и для названия файла
    char currentModule[MAX_PATH];
    char currentFileName[MAX_PATH];

    //Получаем полный путь
    GetModuleFileNameA(NULL, currentModule, MAX_PATH);

    //Создаем указатель, который будет хранить только название файла, и копируем название в currentFileName
    char* fileName = PathFindFileNameA(currentModule);
    strcpy(currentFileName, fileName);

    do {
        //Не будем копировать директории и не будем копировать скрытые файлы
        //Здесь используется побитовое сложение, об этом ниже
        if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
            !(findData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)) {

            //Так же не будем копировать уже скопированные файлы
            if (strncmp(findData.cFileName, "copy_", 5) == 0) {
                continue;
            }

            //И не будем копировать текущий исполняемый файл
            if (strcmp(findData.cFileName, currentFileName) == 0) {
                continue;
            }

            //создаем новое имя файла, и пытаемся создать с ним копию
            char newName[MAX_PATH];
            sprintf(newName, "copy_%s", findData.cFileName);

            //FALSE не позволит перезаписать файл
            if (CopyFileA(findData.cFileName, newName, FALSE)) {
                printf("Copied: %s -> %s\n", findData.cFileName, newName);
            } else {
                printf("Failed to copy: %s (error: %lu)\n", findData.cFileName, GetLastError());
            }
        }

    //Продолжаем, пока находим новые файлы для копирования
    } while (FindNextFileA(hFind, &findData) != 0);

    //Важно не забыть закрыть процесс поиска
    FindClose(hFind);
    return 0;
}
