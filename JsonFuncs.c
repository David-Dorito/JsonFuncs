/*********\
TODO: add array support
TODO: add int support (currently double)
FIXME: fix removing whitespace in strings in RemoveWhiteSpace func
FIXME: make Deserialize more robust, sometimes it can even segfault with incorrect json
\*********/

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include "JsonFuncs.h"

#define BUFFER_SIZE         1024

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
    ARRAY
} TokenType;

typedef struct {
    union {
        double NumberValue;
        char* StringValue;
        char CharValue;
        u8 BoolValue;
    } Value;
    TokenType Type;
    u16 Depth;
} Token;

typedef struct Node Node;
struct Node {
    Node* pParentNode;
    Node* pChildNodes;
    u16 ChildCount;
    Token* pToken;
};

static char* StringWithoutWhitespace(const char* string, u32 len);
static i32 GetTokenAmount(char* json);
static JsonFuncs_Return FillTokenArray(Token* pTokens, u16 tokenAmount, char* json);
static JsonFuncs_Return AssignValues(JsonField* pFields, int fieldAmount, Node* pRootNode);
static JsonFuncs_Return BuildTree(Token* pTokens, u16 start, u16 end, Node* pParentNode);
static char* fgetsFromString(char* destination, u32 len, const char** source, char splittingCharacter);
static JsonFuncs_Return FileToString(const char* fileName, char** out);
static void FreeTree(Node* pNode);


JsonFuncs_Return JsonFuncs_Deserialize(char* rawJson, JsonField* pFields, int fieldAmount, JsonFuncs_InputType FileOrString)
{
    char* json;
    if (FileOrString == JSONFUNCS_INPUT_JSONFILE)
    {
        char* fileContents;
        JsonFuncs_Return errorCode = FileToString(rawJson, &fileContents);
        if (errorCode) return errorCode;
        json = StringWithoutWhitespace(fileContents, strlen(fileContents));
        free(fileContents);
    }
    else json = StringWithoutWhitespace(rawJson, strlen(rawJson));
    
    if (json == NULL) return JSONFUNCS_ERROR_MALLOC_FAILED;

    i32 tokenAmount = GetTokenAmount(json);
    if (tokenAmount == -1) return JSONFUNCS_ERROR_INVALID_JSON;

    Token* pTokens = malloc(tokenAmount * sizeof(Token));
    if (pTokens == NULL)
    {
        free(json);
        return JSONFUNCS_ERROR_MALLOC_FAILED;
    }

    if (FillTokenArray(pTokens, tokenAmount, json) != 0) return JSONFUNCS_ERROR_MALLOC_FAILED;

    Token rootToken = (Token){
        .Depth = 0,
        .Type = STRING,
        .Value.StringValue = "root"
    };
    Node rootNode = (Node){
        .pChildNodes = NULL,
        .pToken = &rootToken,
        .ChildCount = 0
    };
    if (BuildTree(pTokens, 0, tokenAmount, &rootNode)) return JSONFUNCS_ERROR_INVALID_JSON;

    AssignValues(pFields, fieldAmount, &rootNode);

    //output stuff for debugging
    printf("%s\n", json);
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

    //free unneeded memory
    for (u32 i = 0; i < tokenAmount; i++)
        if (pTokens[i].Type == STRING || pTokens[i].Type == ARRAY) free(pTokens[i].Value.StringValue);
    free(pTokens);
    free(json);

    return JSONFUNCS_OK;
}

void JsonFuncs_Serialize()
{
    
}

/*************************************\
  fn: @StringWithoutWhitespace 
  
  param1: const char*: string to remove whitespace from
  param2: int: length of the string
  
  return char*: pointer to the start of the string without whitespace
  
  desc: removes all whitespace from a string
  
  note: allocates new memory for the new string, remember to free it after use
  
\**************************************/
static char* StringWithoutWhitespace(const char* string, u32 len)
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
  fn: @GetTokenAmount 
  
  param1: const char*: string to remove whitespace from
  
  return i32: pointer to the start of the string without whitespace
  
  desc: removes all whitespace from a string
  
  note: allocates new memory for the new string, remember to free it after use
  
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
  fn: @FillTokenArray 
  
  param1 Token*: array of tokens to fill
  param2 u16: amount of tokens in the array
  param2: char*: string to parse tokens from
  
  return JsonFuncsError: error code
  
  desc: fills an array of tokens from a json string
  
  note: allocates memory for string and array tokens, remember to free them after use
  
\**************************************/
static JsonFuncs_Return FillTokenArray(Token* pTokens, u16 tokenAmount, char* json)
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
            if (value == NULL) return JSONFUNCS_ERROR_INVALID_JSON;
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
            if (value == NULL) return JSONFUNCS_ERROR_INVALID_JSON;
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
            if (json[j] == '\0') return JSONFUNCS_ERROR_INVALID_JSON; // invalid json
            u32 len = (j - i) + 1; // include closing bracket

            char* arrayString = malloc(len + 1);
            if (arrayString == NULL) return JSONFUNCS_ERROR_INVALID_JSON;
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

    u16 tokenDepthCounter = 0;
    for (int i = 0; i < tokenAmount; i++)
        if (pTokens[i].Type == LEFT_BRACE) pTokens[i].Depth = ++tokenDepthCounter;
        else if (pTokens[i].Type == RIGHT_BRACE) pTokens[i].Depth = tokenDepthCounter--;
        else pTokens[i].Depth = tokenDepthCounter;
    
    if (tokenDepthCounter != 0) return JSONFUNCS_ERROR_INVALID_JSON; //invalid json

    return JSONFUNCS_OK;
}

/*************************************\
  fn: @AssignValues
  
  param1: JsonField*: array of fields to assign values to given by the API caller
  param2: u32: amount of fields given by the API caller
  param3: Node*: pointer to the root node of the tree
  
  return JsonFuncsError: return error code
  
  desc: assigns values from the key-value pairs to the corresponding fields by matching key names
  
  note: does not support arrays yet
  
\**************************************/
static JsonFuncs_Return AssignValues(JsonField* pFields, int fieldAmount, Node* pRootNode)
{
    u8 result = JSONFUNCS_OK;
    for (u16 i = 0; i < fieldAmount; i++)
    {
        char stringBuffer[BUFFER_SIZE];
        const char* stringPointer = pFields[i].KeyName; //will get incremented
        Node* pCurrentNode = pRootNode;
        while (fgetsFromString(stringBuffer, BUFFER_SIZE, &stringPointer, '.'))
            for (u16 j = 0; j < pCurrentNode->ChildCount; j++)
                    if (!strcmp(pCurrentNode->pChildNodes[j].pToken->Value.StringValue, stringBuffer))
                        pCurrentNode = &pCurrentNode->pChildNodes[j];
                

        if (pCurrentNode->ChildCount != 1 || pCurrentNode->pChildNodes[0].ChildCount != 0)
        {
            result = JSONFUNCS_ERROR_NESTED_VALUE;
            break;
        }

        switch (pCurrentNode->pChildNodes[0].pToken->Type)
        {
            case NUMBER: 
                *((double*)pFields[i].Destination) = pCurrentNode->pChildNodes[0].pToken->Value.NumberValue;
                break;
            case BOOL:
                *((u8*)pFields[i].Destination) = pCurrentNode->pChildNodes[0].pToken->Value.BoolValue;
                break;
            case STRING: 
                char* copy = malloc(strlen(pCurrentNode->pChildNodes[0].pToken->Value.StringValue) + 1);
                strcpy(copy, pCurrentNode->pChildNodes[0].pToken->Value.StringValue);
                *((char**)pFields[i].Destination) = copy;
                break;
            case NULLVALUE:
                memset(pFields[i].Destination, 0, pFields[i].Size);
                break;
        }
    }
    return result;
}

/*************************************\
  fn: @fgetsFromString
  
  param1: char*: the place where to return the string
  param2: u32: len of the string
  param3: const char**: the pointer to the current part of the string, will be changed by this function
  param4: char: the character that splits the strings
  
  return char*: the returned string
  
  desc: basically like fgets but for strings and you can choose the splitting character
  
  note: keep in mind that the source ptr will get incremented
  
\**************************************/
static char* fgetsFromString(char* destination, u32 len, const char** source, char splittingCharacter)
{
    if (!source || !*source || **source == '\0') return NULL;  // end of string

    u32 i = 0;
    while (i < len - 1 && **source != '\0' && **source != splittingCharacter)
        destination[i++] = *(*source)++;

    destination[i] = '\0';

    // If we stopped because of a newline, consume it so subsequent calls
    // continue from the next character (mimics fgets behavior).
    if (**source == splittingCharacter)
        (*source)++;

    return destination;
}

/*************************************\
  fn: @BuildTree
  
  param1: Token*: list of tokens to create tree from
  param2: u16: pass in start index of pTokens
  param3: u16: pass in length of pTokens
  param4: Node*: the root node of the tree
  
  return JsonFuncsError: error code
  
  desc: Builds the json tree from the tokens
  
  note: allocates heap memory
  
\**************************************/
static JsonFuncs_Return BuildTree(Token* pTokens, u16 start, u16 end, Node* pParentNode)
{
    JsonFuncs_Return result = JSONFUNCS_OK;
    u16 searchDepth = pParentNode->pToken->Depth + 1;

    // Count children
    pParentNode->ChildCount = 0;
    for (u16 i = start; i < end; i++)
        if (pTokens[i].Type == COLON && pTokens[i].Depth == searchDepth)
            pParentNode->ChildCount++;

    // Allocate array of Node*
    pParentNode->pChildNodes = malloc(pParentNode->ChildCount * sizeof(Node));
    if (pParentNode->pChildNodes == NULL)
    {
        result = JSONFUNCS_ERROR_MALLOC_FAILED;
        goto cleanup;
    }
    
    // Initialize child nodes
    u16 index = 0;
    for (int i = start; i < end; i++)
        if (pTokens[i].Type == COLON && pTokens[i].Depth == searchDepth)
            pParentNode->pChildNodes[index++] = (Node){
                .ChildCount = 0,
                .pChildNodes = NULL,
                .pToken = &pTokens[i-1],
                .pParentNode = pParentNode
            };

    // Recurse / add grandchildren
    for (int i = 0; i < pParentNode->ChildCount; i++)
    {
        Node* pChildNode = &pParentNode->pChildNodes[i];
        
        ptrdiff_t valueIndex = (pChildNode->pToken - pTokens) + 2;
        if (valueIndex >= end)
        {
            result = JSONFUNCS_ERROR_INVALID_JSON;
            goto cleanup;
        }
        
        if (pTokens[valueIndex].Type != LEFT_BRACE)
        {
            // allocate single child
            pChildNode->ChildCount = 1;
            pChildNode->pChildNodes = malloc(sizeof(Node));
            if (pChildNode->pChildNodes == NULL)
            {
                result = JSONFUNCS_ERROR_MALLOC_FAILED;
                goto cleanup;
            }
            
            pChildNode->pChildNodes[0].pToken = &pTokens[valueIndex];
            pChildNode->pChildNodes[0].ChildCount = 0;
            pChildNode->pChildNodes[0].pChildNodes = NULL;
            pChildNode->pChildNodes[0].pParentNode = pChildNode;
        }
        else
        {
            u16 depth = 1;
            u16 newEndIndex = 0;
            for (int j = valueIndex+1; j < end; j++)
            {
                if (pTokens[j].Type == LEFT_BRACE) depth++;
                else if (pTokens[j].Type == RIGHT_BRACE) depth--;
            
                if (depth == 0)
                {
                    newEndIndex = j;
                    break;
                }
            }
            
            if (newEndIndex == 0 || BuildTree(pTokens, valueIndex + 1, newEndIndex, pChildNode)) //short curcuit condition
            {
                result = JSONFUNCS_ERROR_INVALID_JSON;
                goto cleanup;
            }
        }
    }

    return result;

    cleanup:
        if (!pParentNode->pToken->Depth) //if its the root node
            FreeTree(pParentNode);
    
        return result;
}

/*************************************\
  fn: @FileToString
  
  param1: const char*: the path of the file
  param2: char**: place to save string to
  
  return JsonFuncsError: error code
  
  desc: gets all contents of a file at given path, allocates memory and sets its pointer to the out param
  
  note: allocates heap memory
 
\**************************************/
static JsonFuncs_Return FileToString(const char* fileName, char** out)
{
    FILE* file = fopen(fileName, "rb");
    if (!file) return JSONFUNCS_ERROR_INVALID_PATH;

    // move to end to determine file size
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    rewind(file);

    // allocate buffer
    char* buffer = malloc(size + 1);
    if (!buffer)
    {
        fclose(file);
        return JSONFUNCS_ERROR_MALLOC_FAILED;
    }

    // read file
    fread(buffer, 1, size, file);
    buffer[size] = '\0'; // null-terminate

    fclose(file);
    *out = buffer;
    return JSONFUNCS_OK;
}

/*************************************\
  fn: @FreeTree
  
  param1 Node*: pointer to the root node
  
  return: 
  
  desc: frees the json token tree
  
  note: recursive
 
\**************************************/
static void FreeTree(Node* pNode)
{
    if (pNode->ChildCount)
    {
        for (u32 i = 0; i < pNode->ChildCount; i++)
            FreeTree(&pNode->pChildNodes[i]);
        pNode->ChildCount = 0;
        if (pNode->pChildNodes == NULL) return;
        free(pNode->pChildNodes);
        pNode->pChildNodes = NULL;
    }
}