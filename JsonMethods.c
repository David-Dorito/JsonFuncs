/*********\
TODO:
1. add array support
 
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
    ARRAY
} TokenType;
 
typedef struct {
    TokenType Type;
    union {
        char* StringValue;
        double NumberValue;
        int BoolValue;
        char CharValue;
        int NullValue;
    } Value;
} Token;

typedef struct {
    Token* Key;
    Token* Value;
} Pair;

char* StringWithoutWhitespace(const char* string, int len);
int GetTokenAmount(char* json);
void FillTokenArray(Token* pTokens, char* json);
Token* RemoveRedundantTokens(Token* pTokens, int tokenAmount, int newTokenAmount);
int GetRequiredTokenAmount(Token* pTokens, int tokenAmount);
int GetTokenPairAmount(Token* pTokens, int tokenAmount);
Pair* GetTokenPairs(Token* pTokens, int tokenAmount, int pairAmount);
void AssignValues(JsonField* pFields, int fieldAmount, Pair* pTokenPairs, int pairAmount);

void JsonMethods_Deserialize(char* rawJson, JsonField* pFields, int fieldAmount)
{
    char* json = StringWithoutWhitespace(rawJson, strlen(rawJson));

    if (json == NULL)
    {
        printf("Failed to remove whitespace.\n");
        return;
    }

    printf("%s\n", json);

    int tokenAmount = GetTokenAmount(json);

    if (tokenAmount == -1)
    {
        printf("invalid json\n");
        return;
    }

    printf("token amount: %d\n", tokenAmount);
    Token* pTokens = calloc(tokenAmount, sizeof(Token));

    if (pTokens == NULL)
    {
        printf("Failed to allocate token memory\n");
        goto TokenAllocError;
    }
    
    FillTokenArray(pTokens, json);

    //output tokens
    printf("token array\n[");
    for (int i = 0; i < tokenAmount; i++)
    {
        if (pTokens[i].Type == STRING) printf(" %s ", pTokens[i].Value.StringValue);
        else if (pTokens[i].Type == NUMBER) printf(" %f ", pTokens[i].Value.NumberValue);
        else if (pTokens[i].Type == NULLVALUE) printf(" NULL ");
        else if (pTokens[i].Type == BOOL) printf(" %f ", pTokens[i].Value.BoolValue);
        else if (pTokens[i].Type == ARRAY) printf(" %s ", pTokens[i].Value.StringValue);
        else printf(" %c ", pTokens[i].Value.CharValue);
    }
    printf("]\n");

    int requiredTokenAmount = GetRequiredTokenAmount(pTokens, tokenAmount);
    Token* pRequiredTokens = RemoveRedundantTokens(pTokens, tokenAmount, requiredTokenAmount);

    if (pRequiredTokens == NULL)
    {
        printf("Failed to allocate required token memory\n");
        goto RequiredTokenAllocError;
    }

    printf("new token array\n[");
    //output tokens
    for (int i = 0; i < requiredTokenAmount; i++)
    {
        if (pRequiredTokens[i].Type == STRING) printf(" %s ", pRequiredTokens[i].Value.StringValue);
        else if (pRequiredTokens[i].Type == NUMBER) printf(" %f ", pRequiredTokens[i].Value.NumberValue);
        else if (pRequiredTokens[i].Type == NULLVALUE) printf(" NULL ");
        else if (pRequiredTokens[i].Type == BOOL) printf(" %f ", pRequiredTokens[i].Value.BoolValue);
        else if (pRequiredTokens[i].Type == ARRAY) printf(" %s ", pRequiredTokens[i].Value.StringValue);
        else printf(" %c ", pRequiredTokens[i].Value.CharValue);
    }
    printf("]\n");

    int tokenPairAmount = GetTokenPairAmount(pRequiredTokens, requiredTokenAmount);
    Pair* pTokenPairs = GetTokenPairs(pRequiredTokens, requiredTokenAmount, tokenPairAmount);

    if (pTokenPairs == NULL)
    {
        printf("failed to allocate pair memory");
        goto TokenPairAllocError;
    }

    printf("pair array\n");
    //output pairs
    for (int i = 0; i < tokenPairAmount; i++)
    {
        printf("[ %s : ", pTokenPairs[i].Key->Value.StringValue);
        if (pTokenPairs[i].Value->Type == STRING) printf(" %s ]", pTokenPairs[i].Value->Value.StringValue);
        else if (pTokenPairs[i].Value->Type == NUMBER) printf(" %f ]", pTokenPairs[i].Value->Value.NumberValue);
        else if (pTokenPairs[i].Value->Type == NULLVALUE) printf(" NULL ]");
        else if (pTokenPairs[i].Value->Type == BOOL) printf(" %f ]", pTokenPairs[i].Value->Value.BoolValue);
        else if (pTokenPairs[i].Value->Type == ARRAY) printf(" %s ]", pTokenPairs[i].Value->Value.StringValue);
        else printf(" %c ]", pTokenPairs[i].Value->Value.CharValue);
    }
    printf("\n");

    AssignValues(pFields, fieldAmount, pTokenPairs, tokenPairAmount);

    //free string value in tokens
    for (int i = 0; i < tokenAmount; i++)
        if (pTokens[i].Type == STRING || pTokens[i].Type == ARRAY) free(pTokens[i].Value.StringValue);

    free(pTokenPairs);
    
    TokenPairAllocError:
    free(pRequiredTokens);

    RequiredTokenAllocError:
    free(pTokens);
    
    TokenAllocError:
    free(json);
}

void JsonMethods_Serialize()
{
    
}

char* StringWithoutWhitespace(const char* string, int len)
{
    int newStringSize = 0;
    for (int i = 0; i < len; i++)
        if (!isspace((unsigned char)string[i]))
            newStringSize++;

    char* newArray = calloc(newStringSize + 1, sizeof(char));
    if (newArray == NULL) return NULL;

    int OriginalArrayOffset = 0;
    for (int i = 0; i-OriginalArrayOffset < newStringSize; i++)
        if (!isspace((unsigned char)string[i])) newArray[i-OriginalArrayOffset] = string[i];
        else OriginalArrayOffset++;
    newArray[newStringSize] = '\0';

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
        else if (isalpha(json[i]))
        {
            const int start = i;
            while (isalpha(json[i])) i++;
            const int len = i - start;

            char buffer[8];
            memcpy(buffer, &json[start], len);
            buffer[len] = '\0';

            tokenAmount++;
            i--;
            if (!strcmp(buffer, "null")) continue;
            else if (!strcmp(buffer, "true")) continue;
            else if (!strcmp(buffer, "false")) continue;
            else return -1; //invalid json
        }
        else if (json[i] == '[')
        {
            tokenAmount++;
            while (json[i] != ']') i++;
        }
        else if (strchr("{}:,", json[i]) != NULL) tokenAmount++;
    }

    return tokenAmount;
}

void FillTokenArray(Token* pTokens, char* json)
{
    int nextTokenIndex = 0;
    for (int i = 0; i < strlen(json); i++)
    {
        if (strchr("1234567890.", json[i]) != NULL)
        {
            int j = i;
            while (json[j] != '\0' && strchr("1234567890.", json[j]) != NULL) j++;
            int stringSize = j-i;

            int numIndex = 0;
            char* value = malloc(stringSize+1);
            for (int j = 0; j < stringSize+1; j++) value[j] = '\0';
            value[numIndex++] = json[i];
            while (json[i+1] != '\0' && strchr("1234567890.", json[i+1]) != NULL)
            {
                i++;
                value[numIndex++] = json[i];
            }

            pTokens[nextTokenIndex++] = (Token){
                .Type = NUMBER,
                .Value.NumberValue = atof(value)
            };
            free(value);
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
            
            pTokens[nextTokenIndex++] = (Token){
                .Type = STRING,
                .Value.StringValue = value
            };
        }
        else if (json[i] == '[')
        {
            int start = i;
            int len = json[i+1] != ']'; //start at 1 if array isnt empty
            while (json[++i] != ']') len++;

            char* arrayString = malloc(len + 1);
            memcpy(arrayString, &json[start], len + 1);
            arrayString[len] == '\0';
            
            pTokens[nextTokenIndex++] = (Token){
                .Type = ARRAY,
                .Value.StringValue = arrayString
            };
        }
        else if (strchr("{}:,", json[i]) != NULL)
        {
            TokenType type;
            switch (json[i])
            {
                case '{': type = LEFT_BRACE; break;
                case '}': type = RIGHT_BRACE; break;
                case ':': type = COLON; break;
                case ',': type = COMMA; break;
            }

            pTokens[nextTokenIndex++] = (Token){
                .Type = type,
                .Value.CharValue = json[i]
            };
        }
        else if (isalpha(json[i]))
        {
            const int start = i;
            while (isalpha(json[i])) i++;
            const int len = i - start;

            char buffer[8];
            memcpy(buffer, &json[start], len);
            buffer[len] = '\0';

            i--;
            if (!strcmp(buffer, "null")) pTokens[nextTokenIndex++] = (Token){
                .Type = NULLVALUE,
                .Value.NullValue = 0
            };
            else if (!strcmp(buffer, "true")) pTokens[nextTokenIndex++] = (Token){
                .Type = BOOL,
                .Value.BoolValue = 1
            };
            else if (!strcmp(buffer, "false")) pTokens[nextTokenIndex++] = (Token){
                .Type = BOOL,
                .Value.BoolValue = 0
            };
        }
    }
}

int GetRequiredTokenAmount(Token* pTokens, int tokenAmount)
{
    int newTokenAmount = tokenAmount;
    for (int i = 0; i < tokenAmount; i++)
    {
        if (pTokens[i].Type == LEFT_BRACE)
        {
            newTokenAmount--;
            if (i > 1 && pTokens[i-1].Type == COLON) newTokenAmount-=2;
        }
        else if (pTokens[i].Type == RIGHT_BRACE) newTokenAmount--;
    }
    return newTokenAmount;
}

Token* RemoveRedundantTokens(Token* pTokens, int tokenAmount, int newTokenAmount)
{
    int invalidIndexes[tokenAmount];
    for (int i = 0; i < tokenAmount; i++)
    {
        if (pTokens[i].Type == LEFT_BRACE)
        {
            invalidIndexes[i] = 1;
            if (i > 1 && pTokens[i-1].Type == COLON)
            {
                invalidIndexes[i-1] = 1;
                invalidIndexes[i-2] = 1;
            }
        }
        else if (pTokens[i].Type == RIGHT_BRACE) invalidIndexes[i] = 1;
        else invalidIndexes[i] = 0;
    }

    Token* newTokenArray = calloc(newTokenAmount, sizeof(Token));
    if (newTokenArray == NULL) return NULL;

    int index = 0;
    for (int i = 0; i < tokenAmount; i++)
    {
        if (!invalidIndexes[i])
        {
            Token newToken = {
                .Type = pTokens[i].Type
            };

            switch (pTokens[i].Type)
            {
                case STRING: newToken.Value.StringValue = pTokens[i].Value.StringValue; break;
                case NUMBER: newToken.Value.NumberValue = pTokens[i].Value.NumberValue; break;
                case BOOL: newToken.Value.BoolValue = pTokens[i].Value.BoolValue; break;
                case ARRAY: newToken.Value.StringValue = pTokens[i].Value.StringValue; break;
                default: newToken.Value.CharValue = pTokens[i].Value.CharValue; break;
            }

            newTokenArray[index++] = newToken;
        }
    }

    return newTokenArray;
}

int GetTokenPairAmount(Token* pTokens, int tokenAmount)
{
    int pairAmount = 0;
    for (int i = 0; i < tokenAmount; i++)
        if (pTokens[i].Type == COLON)
            pairAmount++;
    
    return pairAmount;
}

Pair* GetTokenPairs(Token* pTokens, int tokenAmount, int pairAmount)
{
    Pair* pJsonPairs = malloc(pairAmount * sizeof(Pair));
    if (pJsonPairs == NULL) return NULL;

    int index = 0;
    for (int i = 0; i < tokenAmount; i++)
        if (pTokens[i].Type == COLON)
            pJsonPairs[index++] = (Pair){
                .Key = &pTokens[i-1],
                .Value = &pTokens[i+1]
            };
    
    return pJsonPairs;
}

void AssignValues(JsonField* pFields, int fieldAmount, Pair* pTokenPairs, int pairAmount)
{
    for (int i = 0; i < fieldAmount; i++)
    {
        for (int j = 0; j < pairAmount; j++)
        {
            if (strcmp(pFields[i].KeyName, pTokenPairs[j].Key->Value.StringValue)) continue;

            switch (pTokenPairs[j].Value->Type)
            {
                case NUMBER: 
                    *((double*)pFields[i].Destination) = pTokenPairs[j].Value->Value.NumberValue;
                    break;
                case BOOL: 
                    *((int*)pFields[i].Destination) = pTokenPairs[j].Value->Value.BoolValue;
                    break;
                case STRING: 
                    char* copy = malloc(strlen(pTokenPairs[j].Value->Value.StringValue) + 1);
                    strcpy(copy, pTokenPairs[j].Value->Value.StringValue);
                    *((char**)pFields[i].Destination) = copy;
                    break;
                case NULLVALUE:
                    memset(pFields[i].Destination, 0, pFields[i].Size);
                    break;
            }
            break;
        }
    }
}