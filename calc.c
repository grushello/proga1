#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
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
char* OUTFILENAME = "out.txt";
static struct Token* tokens;
static int tokenCount = 0;
static int tokenIndex = 0;
static int errorPos = -1;
//______________________________________________________________________________________________________________________________________________

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
void tokenize(const char* expression ) // fills 'tokens'
{
    int errorPos = -1;
    size_t length = strlen(expression);
    free(tokens);
    tokens = malloc(length * sizeof(struct Token));
    size_t i = 0;
    size_t tlength = 0;
    while(i < length)
    {
        if(isspace(expression[i])) { i++; continue; }

        int startPos = i + 1;

        if(isdigit(expression[i]) || (expression[i] == '.'))
        {
            char *endptr;
            double val = strtod(&expression[i], &endptr);
            tokens[tlength].type = NUMBER;
            tokens[tlength].pos = startPos;
            tokens[tlength].value = val;
            ++tlength;
            i = (int)(endptr - expression);
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
            if(expression[i+1] == '*')
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
    tokens[tlength++] = (struct Token) {EOL, 0, length+1};
}

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
    while(tokens[tokenIndex].type == POW)
    {
        tokenIndex++;
        double right = parsePower();
        left = pow(left, right);
    }
    return left;
}
double parseTerm()
{
    double left = parsePower();
    {
        while (tokens[tokenIndex].type == STAR || tokens[tokenIndex].type == SLASH)
        {
            struct Token* operation = &tokens[tokenIndex++];
            double right = parsePower();
            if(operation->type == SLASH)
            {
                if(right == 0.0)
                {
                    errorPos = operation->pos;
                    return 0;
                }
                left /= right;
            }
            else if(operation->type == STAR)
            {
                left *= right;
            }
            else
            {
                errorPos = operation->pos;
                return 0;
            }
        }
        return left;
    }
}
double parseExpression()
{
    double left = parseTerm();
    while(tokens[tokenIndex].type == PLUS || tokens[tokenIndex].type == MINUS)
    {
        struct Token* operation = &tokens[tokenIndex++];
        double right = parseTerm();
        if(operation->type == PLUS)
        {
            left += right;
        }
        else if (operation->type == MINUS)
        {
            left -= right;
        }
        else
        {
            errorPos = operation->pos;
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
        fprintf(fout, "Error, char: %d\n", pos);\
        return;
    }
    fprintf(fout, "%g\n", result);
}
int main(int argc, char** argv)
{
    char* fileContent = readFile("expr.txt");
    char* currentChar = fileContent;
    char* expression = NULL;
    FILE* fout = fopen(OUTFILENAME, "w");
    while (*currentChar != '\0')
    {
        currentChar = readNextExpression(&expression, currentChar);
        calc(expression, fout);
        free(expression);
    }
    fclose(fout);
    free(fileContent);
    getchar();
    return 0;
}
