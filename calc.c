#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char* readFile(const char* filename)
{
    char* fileContent = NULL;
    long filesize = 0;

    FILE *fp;
    fp = fopen(filename, "rb");
    if (fp == NULL) {
        perror("fopen failed");
        return NULL;
    }

    if (fseek(fp, 0, SEEK_END) != 0)
    {
        perror("fseek failed");
        fclose(fp);
        return NULL;
    }

    filesize = ftell(fp);
    if(filesize < 0)
    {
        perror("ftell failed");
        fclose(fp);
        return NULL;
    }

    fseek(fp, 0, SEEK_SET);

    fileContent = (char*) malloc(filesize + 1);
    if(fileContent == NULL)
    {
        perror("malloc failed");
        fclose(fp);
        return NULL;
    }

    size_t bytesRead = fread(fileContent, sizeof(char), filesize, fp);

    if(bytesRead != filesize)
    {
        fprintf(stderr, "Warning: expected to read %ld bytes, but only read %zu\n", filesize, bytesRead);
    }
    fileContent[bytesRead] = '\0';

    fclose(fp);
    return fileContent;
}
int main(int argc, char** argv)
{
    char* currentChar = readFile("expr.txt");
    printf("%s", currentChar);
    getchar();
    return 0;
}
