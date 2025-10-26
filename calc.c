//Viacheslav Pototskyi - 241ADB183
//Project github: https://github.com/grushello/proga1
//Nothing specific for compilation (gcc calc.c)

//The program is multiplatform (windows, linux)
//Everything for grade 10 is implemented, the project only lacks unary operators

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <sys/stat.h>
#include <sys/types.h>

#ifdef _WIN32
    #include <direct.h>
    #include <windows.h>
    #include <io.h>
    #define GET_USERNAME(buf, size) \
    do { \
        DWORD _len = (DWORD)(size); \
        if(!GetUserNameA(buf, &_len)) { \
            strncpy(buf, "unknown", size); buf[size-1] = '\0'; \
        } \
    } while(0)
    #define MAKE_DIR(dir) _mkdir(dir)
    #define PATH_SEP "\\"
    #ifndef PATH_MAX
        #define PATH_MAX 260
    #endif
#else
    #include <unistd.h>
    #include <dirent.h>
    #include <pwd.h>
    #define MAKE_DIR(dir) mkdir(dir, 0755)
    #define GET_USERNAME(buf, size) \
    do { \
        if(getlogin_r(buf, (size_t)size) != 0) { \
            struct passwd *pw = getpwuid(geteuid()); \
            if(pw && pw->pw_name) strncpy(buf, pw->pw_name, size); \
            else strncpy(buf, "unknown", size); \
            buf[(size)-1] = '\0'; \
        } \
    } while(0)
    #define PATH_SEP "/"
#endif

enum tokenType
{
    INVALID, NUMBER, PLUS, MINUS, STAR, SLASH, POW, LPAREN, RPAREN, EOL
};
struct Token
{
    enum tokenType type;
    double value;
    int pos; // 1-based
};

// DECLARING STUFF__________________________________________________________________________________________________________________________
const size_t MAX_USERNAME_LENGTH = 256;
static struct Token* tokens = NULL;
static int tokenCount = 0;
static int tokenIndex = 0;
static int errorPos = -1;
//______________________________________________________________________________________________________________________________________________

//caller must free. Reads file. Returns pointer to the file string
char* readFile(const char* filename)
{
    char* fileContent = NULL;
    long filesize = 0;

    //opening in binary, cause otherwise it breaks the string (on windows at least)
    FILE *fp = fopen(filename, "rb");
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

    if (fseek(fp, 0, SEEK_SET) != 0) {
        perror("fseek failed");
        fclose(fp);
        return NULL;
    }

    fileContent = (char*) malloc((size_t)filesize + 1);
    if(fileContent == NULL)
    {
        perror("malloc failed");
        fclose(fp);
        return NULL;
    }

    size_t bytesRead = fread(fileContent, 1, (size_t)filesize, fp);
    if(bytesRead != (size_t)filesize) //should find a way to treat this. Upd: decided to treat it with hope
    {
        printf("Warning: expected to read %ld bytes, but only read %zu\n", filesize, bytesRead);
    }
    fileContent[bytesRead] = '\0';

    fclose(fp);
    return fileContent;
}

// Caller must free expression. Reads one expression and  writes read data to expression, returns next expression pointer or file end pointer. 
char* readNextExpression(char** expression, char* currentChar)
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

// fills global 'tokens'. Caller must free 'tokens' afterwards
void tokenize(const char* expression )
{
    errorPos = -1;
    size_t length = strlen(expression);
    free(tokens);
    tokens = malloc((length + 2) * sizeof(struct Token));
    if(!tokens) {
        perror("malloc failed in tokenize\n");
        return;
    }
    size_t i = 0;
    size_t tlength = 0;
    while(i < length)
    {
        // skipping spaces
        if(isspace((unsigned char)expression[i])) { i++; continue; }

        int startPos = (int)i + 1;
        //reading a number
        if(isdigit((unsigned char)expression[i]) || (expression[i] == '.'))
        {
            char *endptr;
            double val = strtod(&expression[i], &endptr);
            tokens[tlength].type = NUMBER;
            tokens[tlength].pos = startPos;
            tokens[tlength].value = val;
            ++tlength;
            i = (size_t)(endptr - expression);
            continue;
        }
        switch (expression[i])
        {
        case '+':
            tokens[tlength++] = (struct Token){PLUS, 0, startPos};
            ++i;
            break;
        case '-':
            tokens[tlength++] = (struct Token){MINUS, 0, startPos};
            ++i;
            break;
        case '*':
        //checking for power
            if(i + 1 < length && expression[i+1] == '*')
            {
                tokens[tlength++] = (struct Token){POW, 0, startPos}; 
                i+=2;
            }
            else
            {
                tokens[tlength++] = (struct Token){STAR, 0, startPos};
                ++i;
            }
            break;
        case '/':
            tokens[tlength++] = (struct Token) { SLASH, 0, startPos };
            ++i;
            break;
        case '(' : tokens[tlength++] = (struct Token) { LPAREN, 0, startPos };
            ++i;
            break;
        case ')' : tokens[tlength++] = (struct Token) { RPAREN, 0, startPos };
            ++i;
            break;
        default:
            tokens[tlength++] = (struct Token) { INVALID, 0, startPos };
            ++i;
            break;
        }
    }
    tokens[tlength++] = (struct Token) {EOL, 0, (int)length + 1};
    tokenCount = (int)tlength;
}

// Parser_____________________________________________________________

double parseExpression();

double parsePrimary()
{
    if(tokens[tokenIndex].type == NUMBER)
    {
        return (tokens[tokenIndex++]).value;
    }
    else if(tokens[tokenIndex].type == LPAREN)
    {
        tokenIndex++;
        double val = parseExpression();
        if(tokens[tokenIndex].type == RPAREN)
        {
            tokenIndex++;
            return val;
        }
        else
        {
            errorPos = tokens[tokenIndex].pos;
            return 0;
        }
    }
    else
    {
        errorPos = tokens[tokenIndex].pos;
        return 0;
    }
}

double parsePower()
{
    double left = parsePrimary();
    if (errorPos != -1) return 0;
    while(tokens[tokenIndex].type == POW)
    {
        int opPos = tokens[tokenIndex].pos;
        tokenIndex++;
        double right = parsePower();
        if (errorPos != -1) return 0;
        left = pow(left, right);
    }
    return left;
}

double parseTerm()
{
    double left = parsePower();
    if (errorPos != -1) return 0;
    while (tokens[tokenIndex].type == STAR || tokens[tokenIndex].type == SLASH)
    {
        struct Token operation = tokens[tokenIndex++];
        double right = parsePower();
        if (errorPos != -1) return 0;
        if(operation.type == SLASH)
        {
            if(right == 0.0)
            {
                errorPos = operation.pos; // report position of '/'
                return 0;
            }
            left /= right;
        }
        else if(operation.type == STAR)
        {
            left *= right;
        }
        else
        {
            errorPos = operation.pos;
            return 0;
        }
    }
    return left;
}

double parseExpression()
{
    double left = parseTerm();
    if (errorPos != -1) return 0;
    while(tokens[tokenIndex].type == PLUS || tokens[tokenIndex].type == MINUS)
    {
        struct Token operation = tokens[tokenIndex++];
        double right = parseTerm();
        if (errorPos != -1) return 0;
        if(operation.type == PLUS)
        {
            left += right;
        }
        else if (operation.type == MINUS)
        {
            left -= right;
        }
        else
        {
            errorPos = operation.pos;
            return 0.0;
        }
    }
    return left;
}

void calc(const char* expression, FILE* fout)
{
    tokenize(expression);
    tokenIndex = 0;
    errorPos = -1;

    double result = parseExpression();
    if(errorPos != -1 || tokens[tokenIndex].type != EOL)
    {
        int pos = errorPos != -1? errorPos : tokens[tokenIndex].pos;
        fprintf(fout, "ERROR:%d\n", pos);
        return;
    }
    fprintf(fout, "%.15g\n", result);
}

//caller must free. Returns pointer to the out file name
char* getOutFilename(const char* inFilename)
{
    const char* base = strrchr(inFilename, PATH_SEP[0]);
    const char* hat = "_viacheslav_pototskyi_241ADB183.txt";

    if (base) base++; // skip the separator
    else base = inFilename;

    //cropping '.txt' from base without changes to inFilename
    char* baseCrop = malloc(strlen(base) + 1);
    if(baseCrop == NULL)
    {
        perror("malloc failed");
        return NULL;
    }
    strcpy(baseCrop, base);
    char* dot = strrchr(baseCrop, '.');
    if(dot != NULL)
        *dot = '\0';

    //constructing final filename
    size_t need = strlen(baseCrop) + strlen(hat) + 1;
    char* outFilename = malloc(need);
    if (!outFilename) 
    {
        free(baseCrop);
        return NULL;
    }
    strcpy(outFilename, baseCrop);
    strcat(outFilename, hat);
    free(baseCrop);
    return outFilename;
}

// caller must free. Returns parent dir string, If no parent, returns "." string
char* getParentDir(const char* path)
{
    if (path == NULL)
        return NULL;

    // Find the last path separator
    const char sep = PATH_SEP[0];
    const char* lastSep = strrchr(path, sep);

    if (lastSep == NULL)
    {
        // No separator - no parent directory
        return strdup(".");
    }

    // if the separator is at the start
    if (lastSep == path)
        return strdup(PATH_SEP);

    // Compute the length of the parent path
    size_t len = (size_t)(lastSep - path);
    char* parent = malloc(len + 1);
    if (parent == NULL)
        return NULL;

    strncpy(parent, path, len);
    parent[len] = '\0';
    return parent;
}

//caller must free. Returns pointer to default outDirectory string
char* getDefaultOutDir(const char* input)
{
    if (!input) return NULL;

    struct stat st;
    char *baseDir = NULL;

    if (stat(input, &st) == 0 && S_ISDIR(st.st_mode))
    {
        // input is a directory
        baseDir = malloc(strlen(input) + 1);
        strcpy(baseDir, input);
    }
    else
    {
        // input is a file path
        baseDir = getParentDir(input);
    }
    if (!baseDir) return NULL;

    char username[MAX_USERNAME_LENGTH];
    GET_USERNAME(username, MAX_USERNAME_LENGTH);

    const char* hat = "_241ADB183";

    // +1 for PATH_SEP, +1 for '\0'
    size_t len = strlen(baseDir) + 1 + strlen(username) + strlen(hat) + 1;
    char* outDir = malloc(len);
    if (!outDir) { free(baseDir); return NULL; }

    //assembling the output directory
    strcpy(outDir, baseDir);
    strcat(outDir, PATH_SEP);
    strcat(outDir, username);
    strcat(outDir, hat);


    free(baseDir);
    return outDir;
}
//create a dir, if it doesn't exist already
void makeDir(const char* dir)
{
    if (!dir) return;
    struct stat st = {0};
    if(stat (dir, &st) == -1)
    {
        if(MAKE_DIR(dir) != 0)
            perror("Error while creating directory");
    }
}

void processFile(const char* inFilename, const char* outDir)
{
    if (!inFilename || !outDir) return ;
    char* outFilename = getOutFilename(inFilename);
    char* fileContent = readFile(inFilename);
    if (!fileContent || !outFilename) { free(outFilename); free(fileContent); return; }

    // +1 for PATH_SEP, +1 for '\0'
    size_t pathLen = strlen(outDir) + 1 + strlen(outFilename) + 1;
    char* path = malloc(pathLen);
    if (!path) { perror("malloc failed"); free(outFilename); free(fileContent); return; }

    //constructing out path
    strcpy(path, outDir);
    strcat(path, PATH_SEP);
    strcat(path, outFilename);

    FILE* fout = fopen(path, "w");
    if (!fout) 
    {
        printf("Error, unable to open '%s' for writing\n", path);
        free(outFilename);
        free(path);
        free(fileContent);
        return;
    }

    char* currentChar = fileContent;
    char* expression = NULL;
    while (*currentChar != '\0')
    {
        // handling comments. Skipping lines starting with # (ignoring spaces)
        char* scan = currentChar;
        while(*scan && *scan != '\n' && isspace((unsigned char)*scan)) scan++;
        if(*scan == '#')
        {
            // skip to next line
            char* next = strchr(currentChar, '\n');
            if(next) currentChar = next + 1;
            else break;
            continue;
        }

        currentChar = readNextExpression(&expression, currentChar);
        if (expression)
        {
            // Call calc only if the line contains a non-space character
            for (size_t i = 0; expression[i]; ++i)
            {
                if (!isspace(expression[i]))
                {
                    calc(expression, fout);
                    break;
                }
            }
            free(expression);
            expression = NULL;
        }
    }

    fclose(fout);
    free(outFilename);
    free(path);
    free(fileContent);
    return;
}

#ifdef _WIN32
// windows direcrtory logic taken from https://learn.microsoft.com/en-us/windows/win32/fileio/listing-the-files-in-a-directory
void processDirectory(const char* dir, const char* outdir)
{
    if(!dir) return;
    WIN32_FIND_DATA ffd;
    HANDLE hFind = INVALID_HANDLE_VALUE;

    // constructing pattern: dir\\*.txt
    size_t patlen = strlen(dir) + 1 + strlen("*.txt") + 1;
    char* pattern = malloc(patlen);
    if(!pattern) { perror("malloc failed!"); return; }

    // check if '\' or '/' is at the end to build pattern correctly
    if (dir[strlen(dir) - 1] == '\\' || dir[strlen(dir) - 1] == '/')
    {
        strcpy(pattern, dir);
        strcat(pattern, "*.txt");
    }
    else
    {
        strcpy(pattern, dir);
        strcat(pattern, "\\*.txt");
    }

    hFind = FindFirstFile(pattern, &ffd);
    if (hFind == INVALID_HANDLE_VALUE) {
        perror("Unable to open directory, or no matching files found");
        free(pattern);
        return;
    }
    free(pattern);

    do {
        // skipping directories
        if(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
        char fullpath[PATH_MAX];

        // building path, accounting for paths ending with '/'
        if (dir[strlen(dir) - 1] == '\\' || dir[strlen(dir) - 1] == '/')
        {
            strcpy(fullpath, dir);
            strcat(fullpath, ffd.cFileName);
        }
        else
        {
            strcpy(fullpath, dir);
            strcat(fullpath, "\\");
            strcat(fullpath, ffd.cFileName);
        }
        processFile(fullpath, outdir);
    } while (FindNextFile(hFind, &ffd) != 0);

    FindClose(hFind);
    return;
}
#else
// linux directory logic taken from https://www.geeksforgeeks.org/c/c-program-list-files-sub-directories-directory/
void processDirectory(const char* dir, const char* outdir)
{
    if(!dir) return;
    struct dirent *de;  // Pointer for directory entry
    DIR *dr = opendir(dir);

    if (dr == NULL)
    {
        printf("Could not open directory '%s'\n", dir);
        return;
    }

    while ((de = readdir(dr)) != NULL)
    {
        const char *dot = strrchr(de->d_name, '.');
        if(dot && strcmp(dot, ".txt") == 0)
        {
            char fullpath[PATH_MAX];
            strcpy(fullpath, dir);
            strcat(fullpath, "/");
            strcat(fullpath, de->d_name);
            processFile(fullpath, outdir);
        }
    }
    closedir(dr);
    return;
}
#endif

int main(int argc, char** argv)
{
    char* dir = NULL;
    char* outdir = NULL;
    char* inputFile = NULL;

    for(int i = 1; i < argc; ++i)
    {
        if(!strcmp(argv[i], "-d") || !strcmp(argv[i], "--dir"))
        {
            if(++i < argc) dir = argv[i];
            else 
            {
                printf("Missing argument after %s\n", argv[i-1]); 
                return 1;
            }
        }
        else if(!strcmp(argv[i], "-o" )|| !strcmp(argv[i], "--output-dir"))
        {
            if(++i < argc) outdir = argv[i];
            else
            {
                printf("Missing argument after %s\n", argv[i-1]); 
                return 1;
            }
        }
        else
        {
            inputFile = argv[i];
        }
    }
    if(outdir == NULL)
    {
        if(inputFile != NULL)
            outdir = getDefaultOutDir(inputFile);
        else if(dir != NULL)
            outdir = getDefaultOutDir(dir);
    }
    if(!outdir)
    {
        printf("Unable to determine output directory\n");
        return 1;
    }
    makeDir(outdir);
    if(dir != NULL)
    {
        processDirectory(dir, outdir);
    }
    else if(inputFile != NULL)
    {
        processFile(inputFile, outdir);
    }
    else
    {
        printf("No input file or directory specified.\n");
        free(outdir);
        return 1;
    }
    free(outdir);
    free(tokens);
    return 0;
}
