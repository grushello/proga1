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
    
    //Setting file pointer to EOF, to read file size
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

    //reading file contents
    size_t bytesRead = fread(fileContent, sizeof(char), filesize, fp);

    if(bytesRead != filesize) //to do: I must come up with a way to treat this
    {
        fprintf(stderr, "Warning: expected to read %ld bytes, but only read %zu\n", filesize, bytesRead);
    }
    fileContent[bytesRead] = '\0';

    fclose(fp);
    return fileContent;
}
char* readNextExpression(char** expression, char* currentChar) // writes read data to expression, returns next expression pointer
{
    const char* end = strchr(currentChar, '\n');
    if(end == NULL)
        end = strchr(currentChar, '\0');
    size_t len = end - currentChar;

    *expression = (char*) malloc(len + 1);
    if(!*expression)
        return NULL;
    strncpy(*expression, currentChar, len);
    (*expression)[len] = '\0';

    if(*end == '\n') //advancing to next expression (via return value)
        return (char*)end + 1;
    return (char*)end;
}
int main(int argc, char** argv)
{
    char* fileContent = readFile("expr.txt");
    char* currentChar = fileContent;
    char* expression = NULL;
    while (*currentChar != '\0')
    {
        currentChar = readNextExpression(&expression, currentChar);
        printf("%s\n", expression);
        free(expression);
    }

    free(fileContent);
    getchar();
    return 0;
}
