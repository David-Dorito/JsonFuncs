/*********\
TODO:
1. add bool support
 
\*********/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include "JsonMethods.h"

typedef enum {
    COLON,
    LEFT_BRACE,
    RIGHT_BRACE,
    COMMA,
    LEFT_BRACKET,
    RIGHT_BRACKET,
    STRING,
    NUMBER,
    BOOL,
    NULLVALUE,
} TokenType;
 
typedef struct {
    TokenType Type;
    union {
        char* StringValue;
        double NumberValue;
        int BoolValue;
        char CharValue;
    } Value;
} Token;

char* StringWithoutWhitespace(const char* array, int len);
int GetTokenAmount(char* json);
void FillTokenArray(Token* tokens, char* json);

void JsonMethods_Deserialize(char* rawJson)
{
    char* json = StringWithoutWhitespace(rawJson, strlen(rawJson));

    if (json == NULL)
    {
        printf("Failed to remove whitespace.\n");
        return;
    }

    printf("%s\n", json);

    int tokenAmount = GetTokenAmount(json);
    printf("Token amount: %d\n", tokenAmount);
    Token* tokens = calloc(tokenAmount, sizeof(Token));

    if (tokens == NULL)
    {
        printf("Failed to allocate token memory.\n");
        goto TokenAllocError;
    }
    
    FillTokenArray(tokens, json);

    //output tokens
    for (int i = 0; i < tokenAmount; i++)
    {
        if (tokens[i].Type == STRING) printf("[ %s ]", tokens[i].Value.StringValue);
        else if (tokens[i].Type == NUMBER) printf("[ %f ]", tokens[i].Value.NumberValue);
        else if (tokens[i].Type == NULLVALUE) printf("[ NULL ]");
        else printf("[ %c ]", tokens[i].Value.CharValue);
    }

    //free string value in tokens
    for (int i = 0; i < tokenAmount; i++)
    {
        free(tokens[i].Value.StringValue);
    }

    free(tokens);

    TokenAllocError:
    free(json);
}

void JsonMethods_Serialize()
{
    
}

char* StringWithoutWhitespace(const char* array, int len)
{
    int newArraySize = 0;
    for (int i = 0; i < len; i++)
        if (!isspace((unsigned char)array[i]))
            newArraySize++;

    char* newArray = calloc(newArraySize + 1, sizeof(char));
    if (newArray == NULL) return NULL;

    int OriginalArrayOffset = 0;
    for (int i = 0; i-OriginalArrayOffset < newArraySize; i++)
        if (!isspace((unsigned char)array[i])) newArray[i-OriginalArrayOffset] = array[i];
        else OriginalArrayOffset++;
    newArray[newArraySize] = '\0';

    return newArray;
}

int GetTokenAmount(char* json)
{
    int tokenAmount = 0;
    int len = strlen(json);
    
    for (int i = 0; i < len; i++)
    {
        if (strchr("1234567890.", json[i]) != NULL)
        {
            tokenAmount++;
            while (strchr("1234567890.", json[i+1]) != NULL) i++;
        }
        else if (json[i] == '\"')
        {
            tokenAmount++;
            i++;
            while (json[i] != '\"') i++;
        }
        else if (json[i] == 'n')
        {
            tokenAmount++;
            while (strchr("ul", json[i+1]) != NULL) i++;
        }
        else if (strchr("{}[]:,", json[i]) != NULL) tokenAmount++;
    }

    return tokenAmount;
}

void FillTokenArray(Token* tokens, char* json)
{
    int nextTokenIndex = 0;
    for (int i = 0; i < strlen(json); i++)
    {
        if (strchr("1234567890.", json[i]) != NULL)
        {
            int NumIndex = 0;
            char value[20];
            for (int j = 0; j < 20; j++) value[j] = '\0';
            value[NumIndex++] = json[i];
            while (json[i + 1] != '\0' && strchr("1234567890.", json[i + 1]) != NULL)
            {
                i++;
                value[NumIndex++] = json[i];
            }
            Token NextToken = {
                .Type = NUMBER,
                .Value.NumberValue = atof(value)
            };
            tokens[nextTokenIndex++] = NextToken;
        }
        else if (json[i] == '\"')
        {
            i++;
            int j = i;
            while (json[j] != '\"') j++;
            int stringSize = j-i;

            char* value = malloc(stringSize+1);
            for (int l = 0; l < stringSize+1; l++) value[l] = '\0';
            
            int index = 0;
            while (json[i] != '\"') value[index++] = json[i++];
            
            Token NextToken = {
                .Type = STRING,
                .Value.StringValue = value
            };
            tokens[nextTokenIndex++] = NextToken;
        }
        else if (strchr("{}[]:,", json[i]) != NULL)
        {
            TokenType type;
            switch (json[i])
            {
                case '{': type = LEFT_BRACE; break;
                case '}': type = RIGHT_BRACE; break;
                case '[': type = LEFT_BRACKET; break;
                case ']': type = RIGHT_BRACKET; break;
                case ':': type = COLON; break;
                case ',': type = COMMA; break;
            }

            Token NextToken = {
                .Type = type,
                .Value.CharValue = json[i]
            };
            tokens[nextTokenIndex++] = NextToken;
        }
        else if (json[i] == 'n')
        {
            while (strchr("ul", json[i+1]) != NULL) i++;

            Token NextToken = {
                .Type = NULLVALUE,
                .Value.CharValue = '\0'
            };
            tokens[nextTokenIndex++] = NextToken;
        }
    }
}