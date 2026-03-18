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

    // Тестовый комметарий
    ssize_t len = readlink("/proc/self/exe", currentModule, sizeof(currentModule) - 1);
    if (len != -1) {
        currentModule[len] = '\0';
    } else {
        printf("Error getting module path\n");
        closedir(dp);
        return 1;
    }

    // �������� ������ ��� �����
    char* fileName = basename(currentModule);
    strcpy(currentFileName, fileName);

    while ((entry = readdir(dp)) != NULL) {
        // ���������� . � ..
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        // �������� ���������� � �����
        if (stat(entry->d_name, &file_stat) == -1) {
            continue;
        }

        // ���������, ��� ��� �� ���������� � �� ������� ����
        if (!S_ISDIR(file_stat.st_mode) && entry->d_name[0] != '.') {

            // �� �������� ��� ������������� �����
            if (strncmp(entry->d_name, "copy_", 5) == 0) {
                continue;
            }

            // �� �������� ������� ����������� ����
            if (strcmp(entry->d_name, currentFileName) == 0) {
                continue;
            }

            // ������� ����� ��� �����
            char newName[4096];
            snprintf(newName, sizeof(newName), "copy_%s", entry->d_name);

            // �������� ����
            FILE *src = fopen(entry->d_name, "rb");
            if (src == NULL) {
                printf("Failed to open source: %s\n", entry->d_name);
                continue;
            }

            FILE *dst = fopen(newName, "xb"); // 'x' - �� ��������������
            if (dst == NULL) {
                printf("Failed to create destination: %s (error: %s)\n",
                       newName, strerror(errno));
                fclose(src);
                continue;
            }

            // ����� ��� �����������
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
