/*********\
TODO:
1. add array support
2. add int support (currently float)
 
\*********/

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include "JsonMethods.h"

typedef int64_t             i64;
typedef int32_t             i32;
typedef int16_t             i16;
typedef int8_t              i8;
typedef uint64_t            u64;
typedef uint32_t            u32;
typedef uint16_t            u16;
typedef uint8_t             u8;

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
    ARRAY,
    DELETED
} TokenType;

typedef struct {
    union {
        double NumberValue;
        char* StringValue;
        char CharValue;
        u8 BoolValue;
    } Value;
    TokenType Type;
} Token;

typedef struct {
    Token* Key;
    Token* Value;
} Pair;

static char* StringWithoutWhitespace(const char* string, int len);
static int GetTokenAmount(char* json);
static u8 FillTokenArray(Token* pTokens, char* json);
static int GetRequiredTokenAmount(Token* pTokens, int tokenAmount);
static void RemoveRedundantTokens(Token* pTokens, int tokenAmount);
static int GetTokenPairAmount(Token* pTokens, int tokenAmount);
static Pair* GetTokenPairs(Token* pTokens, int tokenAmount, int pairAmount);
static void AssignValues(JsonField* pFields, int fieldAmount, Pair* pTokenPairs, int pairAmount);

JsonMethodsError JsonMethods_Deserialize(char* rawJson, JsonField* pFields, int fieldAmount)
{
    char* json = StringWithoutWhitespace(rawJson, strlen(rawJson));
    if (json == NULL) return JSONMETHODS_ERROR_MALLOC_FAILED;

    printf("%s\n", json);

    int tokenAmount = GetTokenAmount(json);
    if (tokenAmount == -1) return JSONMETHODS_ERROR_INVALID_JSON;

    Token* pTokens = calloc(tokenAmount, sizeof(Token));
    if (pTokens == NULL)
    {
        free(json);
        return JSONMETHODS_ERROR_MALLOC_FAILED;
    }

    if (FillTokenArray(pTokens, json) != 0) return JSONMETHODS_ERROR_MALLOC_FAILED;

    //output tokens
    printf("token array\n[");
    for (int i = 0; i < tokenAmount; i++)
    {
        if (pTokens[i].Type == STRING) printf(" %s ", pTokens[i].Value.StringValue);
        else if (pTokens[i].Type == NUMBER) printf(" %f ", pTokens[i].Value.NumberValue);
        else if (pTokens[i].Type == NULLVALUE) printf(" NULL ");
        else if (pTokens[i].Type == BOOL) printf(" %d ", pTokens[i].Value.BoolValue);
        else if (pTokens[i].Type == ARRAY) printf(" %s ", pTokens[i].Value.StringValue);
        else printf(" %c ", pTokens[i].Value.CharValue);
    }
    printf("]\n");

    RemoveRedundantTokens(pTokens, tokenAmount);

    printf("new token array\n[");
    //output tokens
    for (int i = 0; i < tokenAmount; i++)
    {
        if (pTokens[i].Type == STRING) printf(" %s ", pTokens[i].Value.StringValue);
        else if (pTokens[i].Type == NUMBER) printf(" %f ", pTokens[i].Value.NumberValue);
        else if (pTokens[i].Type == NULLVALUE) printf(" NULL ");
        else if (pTokens[i].Type == BOOL) printf(" %d ", pTokens[i].Value.BoolValue);
        else if (pTokens[i].Type == ARRAY) printf(" %s ", pTokens[i].Value.StringValue);
        else printf(" %c ", pTokens[i].Value.CharValue);
    }
    printf("]\n");

    int tokenPairAmount = GetTokenPairAmount(pTokens, tokenAmount);
    Pair* pTokenPairs = GetTokenPairs(pTokens, tokenAmount, tokenPairAmount);
    if (pTokenPairs == NULL)
    {
        free(pTokens);
        free(json);
        return JSONMETHODS_ERROR_MALLOC_FAILED;   
    }

    printf("pair array\n");
    //output pairs
    for (int i = 0; i < tokenPairAmount; i++)
    {
        printf("[ %s : ", pTokenPairs[i].Key->Value.StringValue);
        if (pTokenPairs[i].Value->Type == STRING) printf(" %s ]", pTokenPairs[i].Value->Value.StringValue);
        else if (pTokenPairs[i].Value->Type == NUMBER) printf(" %f ]", pTokenPairs[i].Value->Value.NumberValue);
        else if (pTokenPairs[i].Value->Type == NULLVALUE) printf(" NULL ]");
        else if (pTokenPairs[i].Value->Type == BOOL) printf(" %d ]", pTokenPairs[i].Value->Value.BoolValue);
        else if (pTokenPairs[i].Value->Type == ARRAY) printf(" %s ]", pTokenPairs[i].Value->Value.StringValue);
        else printf(" %c ]", pTokenPairs[i].Value->Value.CharValue);
    }
    printf("\n");

    AssignValues(pFields, fieldAmount, pTokenPairs, tokenPairAmount);

    //free unneeded memory
    for (int i = 0; i < tokenAmount; i++)
        if (pTokens[i].Type == STRING || pTokens[i].Type == ARRAY) free(pTokens[i].Value.StringValue);
    free(pTokenPairs);
    free(pTokens);
    free(json);

    return JSONMETHODS_ERROR_NONE;
}

void JsonMethods_Serialize()
{
    
}

static char* StringWithoutWhitespace(const char* string, int len)
{
    int newStringSize = 0;
    for (int i = 0; i < len; i++)
        if (!isspace((unsigned char)string[i]))
            newStringSize++;

    char* newArray = malloc((newStringSize + 1) * sizeof(char));
    if (newArray == NULL) return NULL;

    int OriginalArrayOffset = 0;
    for (int i = 0; i-OriginalArrayOffset < newStringSize; i++)
        if (!isspace((unsigned char)string[i])) newArray[i-OriginalArrayOffset] = string[i];
        else OriginalArrayOffset++;
    newArray[newStringSize] = '\0';

    return newArray;
}

static int GetTokenAmount(char* json)
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
            while (json[i] != '\"')
            {
                if (json[i] == '\0') return -1; //invalid json
                i++;
            }
        }
        else if (isalpha(json[i]))
        {
            const int start = i;
            while (isalpha(json[i])) i++;
            const int len = i - start;

            char buffer[8];
            if (len >= (int)sizeof(buffer)) return -1;
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
            while (json[i] != ']')
            {
                if (json[i] == '\0') return -1; //invalid json
                i++;
            }
        }
        else if (strchr("{}:,", json[i]) != NULL) tokenAmount++;
    }

    return tokenAmount;
}

static u8 FillTokenArray(Token* pTokens, char* json)
{
    int nextTokenIndex = 0;
    for (int i = 0; i < strlen(json); i++)
    {
        if (strchr("1234567890.", json[i]) != NULL)
        {
            int j = i;
            while (json[j] != '\0' && strchr("1234567890.", json[j]) != NULL) j++;
            int stringSize = (j-i)+1;

            int numIndex = 0;
            char* value = malloc(stringSize);
            if (value == NULL) return 1;
            for (int j = 0; j < stringSize; j++) value[j] = '\0';
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
            int stringSize = (j-i)+1;

            char* value = malloc(stringSize);
            if (value == NULL) return 1;
            for (int l = 0; l < stringSize; l++) value[l] = '\0';
            
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
            int j = i;
            while (json[j] != ']' && json[j] != '\0') j++;
            if (json[j] == '\0') return 1; // invalid json
            int len = j - start + 1; // include closing bracket

            char* arrayString = malloc(len + 1);
            if (arrayString == NULL) return 1;
            memcpy(arrayString, &json[start], len);
            arrayString[len] = '\0';
            i = j; // advance to closing bracket

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
                .Value.BoolValue = 0
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

    return 0;
}

static void RemoveRedundantTokens(Token* pTokens, int tokenAmount)
{
    for (int i = 0; i < tokenAmount; i++)
        if (pTokens[i].Type == LEFT_BRACE)
            if (i > 1 && pTokens[i-1].Type == COLON)
                for (int k = i - 2; k <= i; k++)
                    pTokens[k].Type = DELETED;
            else pTokens[i].Type = DELETED;
        else if (pTokens[i].Type == RIGHT_BRACE)
            pTokens[i].Type = DELETED;
}

static int GetTokenPairAmount(Token* pTokens, int tokenAmount)
{
    int pairAmount = 0;
    for (int i = 0; i < tokenAmount; i++)
        if (pTokens[i].Type == COLON && i > 0 && pTokens[i-1].Type == STRING)
            pairAmount++;
    
    return pairAmount;
}

static Pair* GetTokenPairs(Token* pTokens, int tokenAmount, int pairAmount)
{
    Pair* pJsonPairs = malloc(pairAmount * sizeof(Pair));
    if (pJsonPairs == NULL) return NULL;

    int index = 0;
    for (int i = 0; i < tokenAmount; i++)
        if (pTokens[i].Type == COLON)
            if (i > 0 && pTokens[i-1].Type == STRING && i+1 < tokenAmount && pTokens[i+1].Type != DELETED)
                if (index < pairAmount)
                    pJsonPairs[index++] = (Pair){
                        .Key = &pTokens[i-1],
                        .Value = &pTokens[i+1]
                    };
    
    return pJsonPairs;
}

static void AssignValues(JsonField* pFields, int fieldAmount, Pair* pTokenPairs, int pairAmount)
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
                    *((u8*)pFields[i].Destination) = pTokenPairs[j].Value->Value.BoolValue;
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