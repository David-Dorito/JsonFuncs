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
static i32 GetTokenAmount(char* json);
static u8 FillTokenArray(Token* pTokens, char* json);
static void RemoveRedundantTokens(Token* pTokens, u32 tokenAmount);
static u32 GetTokenPairAmount(Token* pTokens, u32 tokenAmount);
static Pair* GetTokenPairs(Token* pTokens, u32 tokenAmount, u32 pairAmount);
static void AssignValues(JsonField* pFields, int fieldAmount, Pair* pTokenPairs, u32 pairAmount);

JsonMethodsError JsonMethods_Deserialize(char* rawJson, JsonField* pFields, int fieldAmount)
{
    char* json = StringWithoutWhitespace(rawJson, strlen(rawJson));
    if (json == NULL) return JSONMETHODS_ERROR_MALLOC_FAILED;

    printf("%s\n", json);

    u32 tokenAmount = GetTokenAmount(json);
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
    for (u32 i = 0; i < tokenAmount; i++)
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
    for (u32 i = 0; i < tokenAmount; i++)
    {
        if (pTokens[i].Type == STRING) printf(" %s ", pTokens[i].Value.StringValue);
        else if (pTokens[i].Type == NUMBER) printf(" %f ", pTokens[i].Value.NumberValue);
        else if (pTokens[i].Type == NULLVALUE) printf(" NULL ");
        else if (pTokens[i].Type == BOOL) printf(" %d ", pTokens[i].Value.BoolValue);
        else if (pTokens[i].Type == ARRAY) printf(" %s ", pTokens[i].Value.StringValue);
        else printf(" %c ", pTokens[i].Value.CharValue);
    }
    printf("]\n");

    u32 tokenPairAmount = GetTokenPairAmount(pTokens, tokenAmount);
    Pair* pTokenPairs = GetTokenPairs(pTokens, tokenAmount, tokenPairAmount);
    if (pTokenPairs == NULL)
    {
        free(pTokens);
        free(json);
        return JSONMETHODS_ERROR_MALLOC_FAILED;   
    }

    printf("pair array\n");
    //output pairs
    for (u32 i = 0; i < tokenPairAmount; i++)
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
    for (u32 i = 0; i < tokenAmount; i++)
        if (pTokens[i].Type == STRING || pTokens[i].Type == ARRAY) free(pTokens[i].Value.StringValue);
    free(pTokenPairs);
    free(pTokens);
    free(json);

    return JSONMETHODS_ERROR_NONE;
}

void JsonMethods_Serialize()
{
    
}

/*************************************\
 * fn: @StringWithoutWhitespace 
 * 
 * param1: const char*: string to remove whitespace from
 * param2: int: length of the string
 * 
 * return char*: pointer to the start of the string without whitespace
 * 
 * desc: removes all whitespace from a string
 * 
 * note: allocates new memory for the new string, remember to free it after use
 * 
\**************************************/
static char* StringWithoutWhitespace(const char* string, int len)
{
    u32 newStringSize = 0;
    for (u32 i = 0; i < len; i++)
        if (!isspace((unsigned char)string[i]))
            newStringSize++;

    char* newArray = malloc((newStringSize + 1) * sizeof(char));
    if (newArray == NULL) return NULL;

    u32 OriginalArrayOffset = 0;
    for (u32 i = 0; i-OriginalArrayOffset < newStringSize; i++)
        if (!isspace((unsigned char)string[i])) newArray[i-OriginalArrayOffset] = string[i];
        else OriginalArrayOffset++;
    newArray[newStringSize] = '\0';

    return newArray;
}

/*************************************\
 * fn: @GetTokenAmount 
 * 
 * param1: const char*: string to remove whitespace from
 * 
 * return i32: pointer to the start of the string without whitespace
 * 
 * desc: removes all whitespace from a string
 * 
 * note: allocates new memory for the new string, remember to free it after use
 * 
\**************************************/
static i32 GetTokenAmount(char* json)
{
    u32 tokenAmount = 0;
    u32 len = strlen(json);
    
    for (u32 i = 0; i < len; i++)
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
            const u32 start = i;
            while (isalpha(json[i])) i++;
            const u32 len = i - start;

            char buffer[8];
            if (len >= (int)sizeof(buffer)) return -1; //invalid json
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

/*************************************\
 * fn: @FillTokenArray 
 * 
 * param1: Token*: array of tokens to fill
 * param2: char*: string to parse tokens from
 * 
 * return u8: error code
 * 
 * desc: fills an array of tokens from a json string
 * 
 * note: allocates memory for string and array tokens, remember to free them after use
 * 
\**************************************/
static u8 FillTokenArray(Token* pTokens, char* json)
{
    u32 nextTokenIndex = 0;
    for (u32 i = 0; i < strlen(json); i++)
    {
        if (strchr("1234567890.", json[i]) != NULL)
        {
            u32 j = i;
            while (json[j] != '\0' && strchr("1234567890.", json[j]) != NULL) j++;
            u32 stringSize = (j-i)+1;

            u32 numIndex = 0;
            char* value = malloc(stringSize);
            if (value == NULL) return 1;
            for (u32 j = 0; j < stringSize; j++) value[j] = '\0';
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
            u32 j = i; 
            while (json[j] != '\"') j++;
            u32 stringSize = (j-i)+1;

            char* value = malloc(stringSize);
            if (value == NULL) return 1;
            for (u32 l = 0; l < stringSize; l++) value[l] = '\0';
            
            u32 index = 0;
            while (json[i] != '\"') value[index++] = json[i++];
            
            pTokens[nextTokenIndex++] = (Token){
                .Type = STRING,
                .Value.StringValue = value
            };
        }
        else if (json[i] == '[')
        {
            u32 j = i;
            while (json[j] != ']' && json[j] != '\0') j++;
            if (json[j] == '\0') return 1; // invalid json
            u32 len = (j - i) + 1; // include closing bracket

            char* arrayString = malloc(len + 1);
            if (arrayString == NULL) return 1;
            memcpy(arrayString, &json[i], len);
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
            const u32 start = i;
            while (isalpha(json[i])) i++;
            const u32 len = i - start;

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

/*************************************\
 * fn: @RemoveRedundantTokens 
 * 
 * param1: Token*: array of tokens to process
 * param2: u32: amount of tokens in the array
 * 
 * return:
 * 
 * desc: sets redundant tokens (braces) to DELETED type, so they can be ignored later
 * 
 * note: the memory isnt cleared, just marked as DELETED
 * 
\**************************************/
static void RemoveRedundantTokens(Token* pTokens, u32 tokenAmount)
{
    for (u32 i = 0; i < tokenAmount; i++)
        if (pTokens[i].Type == LEFT_BRACE)
            if (i > 1 && pTokens[i-1].Type == COLON)
                for (u32 k = i - 2; k <= i; k++)
                    pTokens[k].Type = DELETED;
            else pTokens[i].Type = DELETED;
        else if (pTokens[i].Type == RIGHT_BRACE)
            pTokens[i].Type = DELETED;
}

/*************************************\
 * fn: @GetTokenPairAmount 
 * 
 * param1: Token*: array of tokens to process
 * param2: u32: amount of tokens in the array
 * 
 * return u32: amount of key-value pairs found
 * 
 * desc: counts the amount of key-value pairs in the token array
 * 
 * note:
 * 
\**************************************/
static u32 GetTokenPairAmount(Token* pTokens, u32 tokenAmount)
{
    u32 pairAmount = 0;
    for (u32 i = 0; i < tokenAmount; i++)
        if (pTokens[i].Type == COLON && i > 0 && pTokens[i-1].Type == STRING)
            pairAmount++;
    
    return pairAmount;
}

/*************************************\
 * fn: @GetTokenPairs 
 * 
 * param1: Token*: array of tokens to process
 * param2: u32: amount of tokens in the array
 * param3: u32: amount of key-value pairs expected
 * 
 * return Pair*: pointer to an array of pairs
 * 
 * desc: creates an array of key-value pairs from the token array
 * 
 * note: allocates memory for the array, remember to free it after use
 * 
\**************************************/
static Pair* GetTokenPairs(Token* pTokens, u32 tokenAmount, u32 pairAmount)
{
    Pair* pJsonPairs = malloc(pairAmount * sizeof(Pair));
    if (pJsonPairs == NULL) return NULL;

    u32 index = 0;
    for (u32 i = 0; i < tokenAmount; i++)
        if (pTokens[i].Type == COLON)
            if (i > 0 && pTokens[i-1].Type == STRING && i+1 < tokenAmount && pTokens[i+1].Type != DELETED)
                if (index < pairAmount)
                    pJsonPairs[index++] = (Pair){
                        .Key = &pTokens[i-1],
                        .Value = &pTokens[i+1]
                    };
    
    return pJsonPairs;
}

/*************************************\
 * fn: @AssignValues
 * 
 * param1: JsonField*: array of fields to assign values to given by the API caller
 * param2: u32: amount of fields given by the API caller
 * param3: Pair*: array of key-value pairs
 * param4: u32: amount of key-value pairs expected
 * 
 * return:
 * 
 * desc: assigns values from the key-value pairs to the corresponding fields by matching key names
 * 
 * note: does not support arrays yet
 * 
\**************************************/
static void AssignValues(JsonField* pFields, int fieldAmount, Pair* pTokenPairs, u32 pairAmount)
{
    for (u32 i = 0; i < fieldAmount; i++)
    {
        for (u32 j = 0; j < pairAmount; j++)
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