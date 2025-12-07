/*********\
TODO:
1. alot
 
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
        char* Stringvalue;
        double NumberValue;
        int BoolValue;
    } Value;
} Token;

char* StringWithoutWhitespace(const char* array, int len);
int GetTokenAmount(char* json);

void JsonMethods_Deserialize(char* RawJson)
{
    char* json = StringWithoutWhitespace(RawJson, strlen(RawJson));

    if (!json) {
        printf("Failed to remove whitespace.\n");
        return;
    }

    printf("Formatted: %s\n", json);

    printf("Token amount: %d\n", GetTokenAmount(json));

    free(json);
}

void JsonMethods_Serialize()
{
    
}

char* StringWithoutWhitespace(const char* array, int len)
{
    int NewArraySize = 0;
    for (int i = 0; i < len; i++)
        if (!isspace((unsigned char)array[i]))
            NewArraySize++;

    char* NewArray = calloc(NewArraySize + 1, sizeof(char));
    if (NewArray == NULL) return NULL;

    int OriginalArrayOffset = 0;
    for (int i = 0; i-OriginalArrayOffset < NewArraySize; i++)
        if (!isspace((unsigned char)array[i])) NewArray[i-OriginalArrayOffset] = array[i];
        else OriginalArrayOffset++;
    NewArray[NewArraySize] = '\0';

    return NewArray;
}

int GetTokenAmount(char* json)
{
    int TokenAmount = 0;
    int len = strlen(json);
    
    for (int i = 0; i < len; i++)
    {
        if (strchr("1234567890.", json[i]) != NULL)
        {
            TokenAmount++;
            while (strchr("1234567890.", json[i+1]) != NULL) i++;
        }
        else if (strchr("\"", json[i]) != NULL)
        {
            TokenAmount++;
            i++;
            while (strchr("\"", json[i]) == NULL) i++;
        }
        else if (strchr("n", json[i]) != NULL)
        {
            TokenAmount++;
            i++;
            while (strchr("ul", json[i]) == NULL) i++;
        }
        else if (strchr("{}[]:,", json[i]) != NULL) TokenAmount++;
    }

    return TokenAmount;
}