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
    ARRAY
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

typedef struct {
    Token* Key;
    Token* Value;
} Pair;

char* StringWithoutWhitespace(const char* array, int len);
int GetTokenAmount(char* json);
void FillTokenArray(Token* tokens, char* json);
Token* RemoveRedundantTokens(Token* tokens, int tokenAmount, int newTokenAmount);
int GetRequiredTokenAmount(Token* tokens, int tokenAmount);
int GetTokenPairAmount();
Pair* GetTokenPairs();

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

    if (tokenAmount == -1)
    {
        printf("invalid json\n");
        return;
    }

    printf("token amount: %d\n", tokenAmount);
    Token* tokens = calloc(tokenAmount, sizeof(Token));

    if (tokens == NULL)
    {
        printf("Failed to allocate token memory\n");
        goto TokenAllocError;
    }
    
    FillTokenArray(tokens, json);

    //output tokens
    printf("token array\n[");
    for (int i = 0; i < tokenAmount; i++)
    {
        if (tokens[i].Type == STRING) printf(" %s ", tokens[i].Value.StringValue);
        else if (tokens[i].Type == NUMBER) printf(" %f ", tokens[i].Value.NumberValue);
        else if (tokens[i].Type == NULLVALUE) printf(" NULL ");
        else if (tokens[i].Type == BOOL) printf(" %f ", tokens[i].Value.NumberValue);
        else if (tokens[i].Type == ARRAY) printf(" %s ", tokens[i].Value.StringValue);
        else printf(" %c ", tokens[i].Value.CharValue);
    }
    printf("]\n");

    int requiredTokenAmount = GetRequiredTokenAmount(tokens, tokenAmount);
    Token* requiredTokens = RemoveRedundantTokens(tokens, tokenAmount, requiredTokenAmount);

    if (requiredTokens == NULL)
    {
        printf("Failed to allocate required token memory\n");
        goto RequiredTokenAllocError;
    }

    printf("new token array\n[");
    //output tokens
    for (int i = 0; i < requiredTokenAmount; i++)
    {
        if (requiredTokens[i].Type == STRING) printf(" %s ", requiredTokens[i].Value.StringValue);
        else if (requiredTokens[i].Type == NUMBER) printf(" %f ", requiredTokens[i].Value.NumberValue);
        else if (requiredTokens[i].Type == NULLVALUE) printf(" NULL ");
        else if (requiredTokens[i].Type == BOOL) printf(" %f ", requiredTokens[i].Value.NumberValue);
        else if (requiredTokens[i].Type == ARRAY) printf(" %s ", requiredTokens[i].Value.StringValue);
        else printf(" %c ", requiredTokens[i].Value.CharValue);
    }
    printf("]\n");

    int tokenPairAmount = GetTokenPairAmount(requiredTokens, requiredTokenAmount);
    Pair* tokenPairs = GetTokenPairs(requiredTokens, requiredTokenAmount, tokenPairAmount);

    printf("pair array\n");
    //output pairs
    for (int i = 0; i < tokenPairAmount; i++)
    {
        printf("[ %s : ", tokenPairs[i].Key->Value.StringValue);
        if (tokenPairs[i].Value->Type == STRING) printf(" %s ]", tokenPairs[i].Value->Value.StringValue);
        else if (tokenPairs[i].Value->Type == NUMBER) printf(" %f ]", tokenPairs[i].Value->Value.NumberValue);
        else if (tokenPairs[i].Value->Type == NULLVALUE) printf(" NULL ]");
        else if (tokenPairs[i].Value->Type == BOOL) printf(" %f ]", tokenPairs[i].Value->Value.NumberValue);
        else if (tokenPairs[i].Value->Type == ARRAY) printf(" %s ]", tokenPairs[i].Value->Value.StringValue);
        else printf(" %c ]", tokenPairs[i].Value->Value.CharValue);
    }
    printf("\n");

    //free string value in tokens
    for (int i = 0; i < tokenAmount; i++)
        if (tokens[i].Type == STRING || tokens[i].Type == ARRAY) free(tokens[i].Value.StringValue);

    free(requiredTokens);

    RequiredTokenAllocError:
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

void FillTokenArray(Token* tokens, char* json)
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

            tokens[nextTokenIndex++] = (Token){
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
            
            tokens[nextTokenIndex++] = (Token){
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
            
            tokens[nextTokenIndex++] = (Token){
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

            tokens[nextTokenIndex++] = (Token){
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
            if (!strcmp(buffer, "null")) tokens[nextTokenIndex++] = (Token){
                .Type = NULLVALUE,
                .Value.CharValue = '\0'
            };
            else if (!strcmp(buffer, "true")) tokens[nextTokenIndex++] = (Token){
                .Type = BOOL,
                .Value.NumberValue = 1
            };
            else if (!strcmp(buffer, "false")) tokens[nextTokenIndex++] = (Token){
                .Type = BOOL,
                .Value.NumberValue = 0
            };
        }
    }
}

int GetRequiredTokenAmount(Token* tokens, int tokenAmount)
{
    int newTokenAmount = tokenAmount;
    for (int i = 0; i < tokenAmount; i++)
    {
        if (tokens[i].Type == LEFT_BRACE)
        {
            newTokenAmount--;
            if (i > 1 && tokens[i-1].Type == COLON) newTokenAmount-=2;
        }
        else if (tokens[i].Type == RIGHT_BRACE) newTokenAmount--;
    }
    return newTokenAmount;
}

Token* RemoveRedundantTokens(Token* tokens, int tokenAmount, int newTokenAmount)
{
    int invalidIndexes[tokenAmount];
    for (int i = 0; i < tokenAmount; i++)
    {
        if (tokens[i].Type == LEFT_BRACE)
        {
            invalidIndexes[i] = 1;
            if (i > 1 && tokens[i-1].Type == COLON)
            {
                invalidIndexes[i-1] = 1;
                invalidIndexes[i-2] = 1;
            }
        }
        else if (tokens[i].Type == RIGHT_BRACE) invalidIndexes[i] = 1;
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
                .Type = tokens[i].Type
            };

            switch (tokens[i].Type)
            {
                case STRING: newToken.Value.StringValue = tokens[i].Value.StringValue; break;
                case NUMBER: newToken.Value.NumberValue = tokens[i].Value.NumberValue; break;
                case BOOL: newToken.Value.NumberValue = tokens[i].Value.NumberValue; break;
                case ARRAY: newToken.Value.StringValue = tokens[i].Value.StringValue; break;
                default: newToken.Value.CharValue = tokens[i].Value.CharValue; break;
            }

            newTokenArray[index++] = newToken;
        }
    }

    return newTokenArray;
}

int GetTokenPairAmount(Token* tokens, int tokenAmount)
{
    int pairAmount = 0;
    for (int i = 0; i < tokenAmount; i++)
        if (tokens[i].Type == COLON)
            pairAmount++;
    
    return pairAmount;
}

Pair* GetTokenPairs(Token* tokens, int tokenAmount, int pairAmount)
{
    Pair* jsonPairs = malloc(pairAmount * sizeof(Pair));

    int index = 0;
    for (int i = 0; i < tokenAmount; i++)
        if (tokens[i].Type == COLON)
            jsonPairs[index++] = (Pair){
                .Key = &tokens[i-1],
                .Value = &tokens[i+1]
            };
    
    return jsonPairs;
}