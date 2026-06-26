/*********\
TODO: add array support
FIXME: make Deserialize more robust, sometimes it can even segfault with incorrect json
\*********/

#define JSONFUNCS_DEBUG

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include "JsonFuncs.h"

#define BUFFER_SIZE 1024

#define ROOT_NODE_DEPTH 0

#ifdef JSONFUNCS_DEBUG
#define DEBUG_RUN(code)                                                                                      \
	do {                                                                                                     \
		code                                                                                                 \
	} while (0)
#else
#define DEBUG_RUN(code)                                                                                      \
	do {                                                                                                     \
	} while (0)
#endif

typedef int64_t  int64;
typedef int32_t  int32;
typedef int16_t  int16;
typedef int8_t   int8;
typedef uint64_t uint64;
typedef uint32_t uint32;
typedef uint16_t uint16;
typedef uint8_t  uint8;

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
		double NumValue;
		char*  StringValue;
		char   CharValue;
		uint8  BoolValue;
	} Value;
	TokenType Type;
	uint16    Depth;
} Token;

typedef struct Node Node;
struct Node {
	Token* Token;
	Node*  ParentNode;
	Node*  ChildNodes;
	uint16 ChildCount;
};

static char*            getStringWithoutWhitespace(const char* string, uint32 len);
static int32            getTokenCount(char* json);
static JsonFuncs_Return fillTokenArray(Token* tokens, uint16 tokenCount, char* json);
static JsonFuncs_Return assignValues(JsonField* fields, int fieldCount, Node* rootNode);
static JsonFuncs_Return buildTree(Token* tokens, uint16 start, uint16 end, Node* parentNode);
static char*            fgetsFromString(char* destination, uint32 len, const char** source, char splitChar);
static JsonFuncs_Return getFileToString(const char* fileName, char** out);
static void             freeTree(Node* node);
static JsonFuncs_Return getJsonString(JsonFuncs_InputType fileOrString, char** destination,
                                      char* jsonFileOrString);

JsonFuncs_Return JsonFuncs_Deserialize(char* rawJson, JsonField* fields, int fieldCount,
                                       JsonFuncs_InputType fileOrString) {
	if (rawJson == NULL || fields == NULL)
		return JSONFUNCS_ERROR_NULL_PTR;

	JsonFuncs_Return ret = JSONFUNCS_OK;

	char* json;
	ret = getJsonString(fileOrString, &json, rawJson);
	if (ret != JSONFUNCS_OK) {
		return ret;
	}

	int32 tokenCount = getTokenCount(json);
	if (tokenCount == -1) {
		free(json);
		return JSONFUNCS_ERROR_INVALID_JSON;
	}

	Token* tokens = malloc(tokenCount * sizeof(Token));
	if (tokens == NULL) {
		free(json);
		return JSONFUNCS_ERROR_MALLOC_FAILED;
	}
	ret = fillTokenArray(tokens, tokenCount, json);
	if (ret != JSONFUNCS_OK) {
		free(json);
		free(tokens);
		return ret;
	}

	Token rootToken = (Token){
	    .Depth = 0,
	    .Type = STRING,
	    .Value.StringValue = "root",
	};
	Node rootNode = (Node){
	    .ChildNodes = NULL,
	    .Token = &rootToken,
	    .ChildCount = 0,
	};
	ret = buildTree(tokens, 0, tokenCount, &rootNode);
	if (ret != JSONFUNCS_OK) {
		free(json);
		free(tokens);
		return ret;
	}

	ret = assignValues(fields, fieldCount, &rootNode);
	if (ret != JSONFUNCS_OK) {
		freeTree(&rootNode);
		for (uint32 i = 0; i < tokenCount; i++)
			if (tokens[i].Type == STRING || tokens[i].Type == ARRAY)
				free(tokens[i].Value.StringValue);
		free(tokens);
		free(json);
		return ret;
	}

	// clang-format off
	DEBUG_RUN(
	    // output stuff for debugging
	    printf("json without whitespace\n%s\n", json);
		printf("token array\n");
	    for (uint32 i = 0; i < tokenCount; i++) {
		    if (tokens[i].Type == STRING)
			    printf(" %s ", tokens[i].Value.StringValue);
		    else if (tokens[i].Type == NUMBER)
			    printf(" %f ", tokens[i].Value.NumValue);
		    else if (tokens[i].Type == NULLVALUE)
			    printf(" NULL ");
		    else if (tokens[i].Type == BOOL)
			    printf(" %d ", tokens[i].Value.BoolValue);
		    else if (tokens[i].Type == ARRAY)
			    printf(" %s ", tokens[i].Value.StringValue);
		    else
			    printf(" %c ", tokens[i].Value.CharValue);
	    }
		printf("\n");
	);
	// clang-format on

	// free unneeded memory
	freeTree(&rootNode);
	for (uint32 i = 0; i < tokenCount; i++)
		if (tokens[i].Type == STRING || tokens[i].Type == ARRAY)
			free(tokens[i].Value.StringValue);
	free(tokens);
	free(json);

	return JSONFUNCS_OK;
}

void JsonFuncs_Serialize() {
}

/*************************************\
  fn: @getStringWithoutWhitespace

  param1: const char*: string to remove whitespace from
  param2: int: length of the string

  return char*: pointer to the start of the string without whitespace

  desc: removes all whitespace from a string

  note: allocates new memory for the new string, remember to free it after use

\**************************************/
static char* getStringWithoutWhitespace(const char* string, uint32 len) {
	uint32 newStringSize = 0;
	bool   isInString = false;
	for (uint32 i = 0; i < len; i++) {
		if (string[i] == '\"')
			isInString = !isInString;
		if (!isspace((unsigned char)string[i]) || isInString)
			newStringSize++;
	}

	char* newArray = malloc((newStringSize + 1) * sizeof(char));
	if (newArray == NULL)
		return NULL;

	isInString = false;
	uint32 OriginalArrayOffset = 0;
	for (uint32 i = 0; i - OriginalArrayOffset < newStringSize; i++) {
		if (string[i] == '\"')
			isInString = !isInString;
		if (!isspace((unsigned char)string[i]) || isInString)
			newArray[i - OriginalArrayOffset] = string[i];
		else
			OriginalArrayOffset++;
	}
	newArray[newStringSize] = '\0';

	return newArray;
}

/*************************************\
  fn: @getTokenCount

  param1: const char*: string to remove whitespace from

  return int32: pointer to the start of the string without whitespace

  desc: removes all whitespace from a string

  note: allocates new memory for the new string, remember to free it after use

\**************************************/
static int32 getTokenCount(char* json) {
	uint32 tokenCount = 0;
	uint32 len = strlen(json);

	for (uint32 i = 0; i < len; i++) {
		if (strchr("1234567890.", json[i]) != NULL) {
			tokenCount++;
			while (strchr("1234567890.", json[i + 1]) != NULL)
				i++;
		} else if (json[i] == '\"') {
			tokenCount++;
			i++;
			while (json[i] != '\"') {
				if (json[i] == '\0')
					return -1; // invalid json
				i++;
			}
		} else if (isalpha(json[i])) {
			const uint32 start = i;
			while (isalpha(json[i]))
				i++;
			const uint32 len = i - start;

			char buffer[8];
			if (len >= (int)sizeof(buffer))
				return -1; // invalid json
			memcpy(buffer, &json[start], len);
			buffer[len] = '\0';

			tokenCount++;
			i--;
			if (!strcmp(buffer, "null"))
				continue;
			else if (!strcmp(buffer, "true"))
				continue;
			else if (!strcmp(buffer, "false"))
				continue;
			else
				return -1; // invalid json
		} else if (json[i] == '[') {
			tokenCount++;
			while (json[i] != ']') {
				if (json[i] == '\0')
					return -1; // invalid json
				i++;
			}
		} else if (strchr("{}:,", json[i]) != NULL)
			tokenCount++;
	}

	return tokenCount;
}

/*************************************\
  fn: @fillTokenArray

  param1 Token*: array of tokens to fill
  param2 uint16: amount of tokens in the array
  param2: char*: string to parse tokens from

  return JsonFuncsError: error code

  desc: fills an array of tokens from a json string

  note: allocates memory for string and array tokens, remember to free them after use

\**************************************/
static JsonFuncs_Return fillTokenArray(Token* tokens, uint16 tokenCount, char* json) {
	uint32 nextTokenIndex = 0;
	for (uint32 i = 0; i < strlen(json); i++) {
		if (strchr("1234567890.", json[i]) != NULL) {
			uint32 j = i;
			while (json[j] != '\0' && strchr("1234567890.", json[j]) != NULL)
				j++;
			uint32 stringSize = (j - i) + 1;

			uint32 numIndex = 0;
			char*  value = malloc(stringSize);
			if (value == NULL)
				return JSONFUNCS_ERROR_INVALID_JSON;
			for (uint32 j = 0; j < stringSize; j++)
				value[j] = '\0';
			value[numIndex++] = json[i];
			while (json[i + 1] != '\0' && strchr("1234567890.", json[i + 1]) != NULL) {
				i++;
				value[numIndex++] = json[i];
			}

			tokens[nextTokenIndex++] = (Token){
			    .Type = NUMBER,
			    .Value.NumValue = atof(value),
			};
			free(value);
		} else if (json[i] == '\"') {
			i++;
			uint32 j = i;
			while (json[j] != '\"')
				j++;
			uint32 stringSize = (j - i) + 1;

			char* value = malloc(stringSize);
			if (value == NULL)
				return JSONFUNCS_ERROR_INVALID_JSON;
			for (uint32 l = 0; l < stringSize; l++)
				value[l] = '\0';

			uint32 index = 0;
			while (json[i] != '\"')
				value[index++] = json[i++];

			tokens[nextTokenIndex++] = (Token){
			    .Type = STRING,
			    .Value.StringValue = value,
			};
		} else if (json[i] == '[') {
			uint32 j = i;
			while (json[j] != ']' && json[j] != '\0')
				j++;
			if (json[j] == '\0')
				return JSONFUNCS_ERROR_INVALID_JSON; // invalid json
			uint32 len = (j - i) + 1;                // include closing bracket

			char* arrayString = malloc(len + 1);
			if (arrayString == NULL)
				return JSONFUNCS_ERROR_INVALID_JSON;
			memcpy(arrayString, &json[i], len);
			arrayString[len] = '\0';
			i = j; // advance to closing bracket

			tokens[nextTokenIndex++] = (Token){
			    .Type = ARRAY,
			    .Value.StringValue = arrayString,
			};
		} else if (strchr("{}:,", json[i]) != NULL) {
			TokenType type;
			switch (json[i]) {
			case '{':
				type = LEFT_BRACE;
				break;
			case '}':
				type = RIGHT_BRACE;
				break;
			case ':':
				type = COLON;
				break;
			case ',':
				type = COMMA;
				break;
			}

			tokens[nextTokenIndex++] = (Token){
			    .Type = type,
			    .Value.CharValue = json[i],
			};
		} else if (isalpha(json[i])) {
			const uint32 start = i;
			while (isalpha(json[i]))
				i++;
			const uint32 len = i - start;

			char buffer[8];
			memcpy(buffer, &json[start], len);
			buffer[len] = '\0';

			i--;
			if (!strcmp(buffer, "null"))
				tokens[nextTokenIndex++] = (Token){
				    .Type = NULLVALUE,
				    .Value.BoolValue = 0,
				};
			else if (!strcmp(buffer, "true"))
				tokens[nextTokenIndex++] = (Token){
				    .Type = BOOL,
				    .Value.BoolValue = 1,
				};
			else if (!strcmp(buffer, "false"))
				tokens[nextTokenIndex++] = (Token){
				    .Type = BOOL,
				    .Value.BoolValue = 0,
				};
		}
	}

	uint16 tokenDepthCounter = 0;
	for (int i = 0; i < tokenCount; i++)
		if (tokens[i].Type == LEFT_BRACE)
			tokens[i].Depth = ++tokenDepthCounter;
		else if (tokens[i].Type == RIGHT_BRACE)
			tokens[i].Depth = tokenDepthCounter--;
		else
			tokens[i].Depth = tokenDepthCounter;

	if (tokenDepthCounter != 0)
		return JSONFUNCS_ERROR_INVALID_JSON; // invalid json

	return JSONFUNCS_OK;
}

/*************************************\
  fn: @assignValues

  param1: JsonField*: array of fields to assign values to given by the API caller
  param2: uint32: amount of fields given by the API caller
  param3: Node*: pointer to the root node of the tree

  return JsonFuncsError: return error code

  desc: assigns values from the key-value pairs to the corresponding fields by matching key names

  note: does not support arrays yet

\**************************************/
static JsonFuncs_Return assignValues(JsonField* fields, int fieldCount, Node* rootNode) {
	uint8 result = JSONFUNCS_OK;
	for (uint16 i = 0; i < fieldCount; i++) {
		char        stringBuffer[BUFFER_SIZE];
		const char* stringPointer = fields[i].KeyName; // will get incremented
		Node*       currentNode = rootNode;
		while (fgetsFromString(stringBuffer, BUFFER_SIZE, &stringPointer, '.'))
			for (uint16 j = 0; j < currentNode->ChildCount; j++)
				if (!strcmp(currentNode->ChildNodes[j].Token->Value.StringValue, stringBuffer))
					currentNode = &currentNode->ChildNodes[j];

		if (currentNode->ChildCount != 1 || currentNode->ChildNodes[0].ChildCount != 0) {
			result = JSONFUNCS_ERROR_NESTED_VALUE;
			break;
		}

		// the part to assign values to the api caller defined places
		switch (currentNode->ChildNodes[0].Token->Type) {
		case NUMBER:
			switch (fields[i].Type) {
			case JSONFUNCS_BOOL:
			case JSONFUNCS_INT8:
			case JSONFUNCS_UINT8:
				*((uint8*)fields[i].Destination) = (uint8)currentNode->ChildNodes[0].Token->Value.NumValue;
				break;
			case JSONFUNCS_INT16:
			case JSONFUNCS_UINT16:
				*((uint16*)fields[i].Destination) = (uint16)currentNode->ChildNodes[0].Token->Value.NumValue;
				break;
			case JSONFUNCS_UINT32:
			case JSONFUNCS_INT32:
				*((uint32*)fields[i].Destination) = (uint32)currentNode->ChildNodes[0].Token->Value.NumValue;
				break;
			case JSONFUNCS_FLOAT:
				*((float*)fields[i].Destination) = (float)currentNode->ChildNodes[0].Token->Value.NumValue;
				break;
			case JSONFUNCS_DOUBLE:
				*((double*)fields[i].Destination) = currentNode->ChildNodes[0].Token->Value.NumValue;
				break;
			};
			break;
		case BOOL:
			*((uint8*)fields[i].Destination) = currentNode->ChildNodes[0].Token->Value.BoolValue;
			break;
		case STRING:
			switch (fields[i].Type) {
			case JSONFUNCS_CHAR:
				*((char*)fields[i].Destination) = currentNode->ChildNodes[0].Token->Value.StringValue[0];
				break;
			case JSONFUNCS_STRING:
				char* copy = malloc(strlen(currentNode->ChildNodes[0].Token->Value.StringValue) + 1);
				strcpy(copy, currentNode->ChildNodes[0].Token->Value.StringValue);
				*((char**)fields[i].Destination) = copy;
				break;
			}
			break;
		case NULLVALUE:
			memset(fields[i].Destination, 0, fields[i].Size);
			break;
		default:
			printf("this isnt supposed to happen\n");
			break;
		}
	}
	return result;
}

/*************************************\
  fn: @fgetsFromString

  param1: char*: the place where to return the string
  param2: uint32: len of the string
  param3: const char**: the pointer to the current part of the string, will be changed by this function
  param4: char: the character that splits the strings

  return char*: the returned string

  desc: basically like fgets but for strings and you can choose the splitting character

  note: keep in mind that the source ptr will get incremented

\**************************************/
static char* fgetsFromString(char* destination, uint32 len, const char** source, char splitChar) {
	if (source == NULL || *source == NULL || **source == '\0')
		return NULL; // end of string

	uint32 idx = 0;
	while (idx < len - 1) {
		if (**source == '\0' || **source == splitChar)
			break;
		destination[idx++] = *((*source)++);
	}

	destination[idx] = '\0';

	// If we stopped because of a spltting character, consume it so subsequent calls
	// continue from the next character (mimics fgets behavior).
	if (**source == splitChar)
		(*source)++;

	return destination;
}

/*************************************\
  fn: @BuildTree

  param1: Token*: list of tokens to create tree from
  param2: uint16: pass in start index of tokens
  param3: uint16: pass in length of tokens
  param4: Node*: the root node of the tree

  return JsonFuncsError: error code

  desc: Builds the json tree from the tokens

  note: allocates heap memory

\**************************************/
static JsonFuncs_Return buildTree(Token* tokens, uint16 start, uint16 end, Node* parentNode) {
	JsonFuncs_Return result = JSONFUNCS_OK;
	uint16           searchDepth = parentNode->Token->Depth + 1;

	// Count children
	parentNode->ChildCount = 0;
	for (uint16 i = start; i < end; i++)
		if (tokens[i].Type == COLON && tokens[i].Depth == searchDepth)
			parentNode->ChildCount++;

	if (parentNode->ChildCount == 0) {
		result = JSONFUNCS_ERROR_INVALID_JSON;
		goto cleanup;
	}

	// Allocate array of Node*
	parentNode->ChildNodes = malloc(parentNode->ChildCount * sizeof(Node));
	if (parentNode->ChildNodes == NULL) {
		result = JSONFUNCS_ERROR_MALLOC_FAILED;
		goto cleanup;
	}

	// Initialize child nodes
	uint16 index = 0;
	for (int i = start; i < end; i++)
		if (tokens[i].Type == COLON && tokens[i].Depth == searchDepth) {
			// TODO: check if key is a string
			if (tokens[i - 1].Type != STRING) {
				result = JSONFUNCS_ERROR_INVALID_JSON;
				goto cleanup;
			}
			parentNode->ChildNodes[index++] = (Node){
			    .ChildCount = 0, .ChildNodes = NULL, .Token = &tokens[i - 1], .ParentNode = parentNode};
		}

	// Recurse / add grandchildren
	for (int i = 0; i < parentNode->ChildCount; i++) {
		Node* pChildNode = &parentNode->ChildNodes[i];

		ptrdiff_t valueIndex = (pChildNode->Token - tokens) + 2;

		// small invalid json check -----------------
		if (valueIndex >= end) {
			result = JSONFUNCS_ERROR_INVALID_JSON;
			goto cleanup;
		}
		uint8 jsonValidTest = 0;
		switch (tokens[valueIndex].Type) {
		case RIGHT_BRACE:
			jsonValidTest = 1;
			break;
		case COLON:
			jsonValidTest = 1;
			break;
		case COMMA:
			jsonValidTest = 1;
			break;
		}
		if (jsonValidTest) {
			result = JSONFUNCS_ERROR_INVALID_JSON;
			goto cleanup;
		}
		//------------------------------------------

		if (tokens[valueIndex].Type != LEFT_BRACE) {
			// allocate single child
			pChildNode->ChildCount = 1;
			pChildNode->ChildNodes = malloc(sizeof(Node));
			if (pChildNode->ChildNodes == NULL) {
				result = JSONFUNCS_ERROR_MALLOC_FAILED;
				goto cleanup;
			}

			pChildNode->ChildNodes[0] = (Node){
			    .Token = &tokens[valueIndex], .ChildCount = 0, .ChildNodes = NULL, .ParentNode = pChildNode};
		} else {
			uint16 depth = 1;
			uint16 newEndIndex = 0;
			for (int j = valueIndex + 1; j < end; j++) {
				if (tokens[j].Type == LEFT_BRACE)
					depth++;
				else if (tokens[j].Type == RIGHT_BRACE)
					depth--;

				if (depth == 0) {
					newEndIndex = j;
					break;
				}
			}

			if (newEndIndex == 0 ||
			    buildTree(tokens, valueIndex + 1, newEndIndex, pChildNode)) // short curcuit condition
			{
				result = JSONFUNCS_ERROR_INVALID_JSON;
				goto cleanup;
			}
		}
	}

	return result;

cleanup:
	if (parentNode->Token->Depth == ROOT_NODE_DEPTH) // if its the root node
		freeTree(parentNode);

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
static JsonFuncs_Return getFileToString(const char* fileName, char** out) {
	FILE* file = fopen(fileName, "rb");
	if (!file)
		return JSONFUNCS_ERROR_INVALID_PATH;

	// move to end to determine file size
	fseek(file, 0, SEEK_END);
	long size = ftell(file);
	rewind(file);

	// allocate buffer
	char* buffer = malloc(size + 1);
	if (!buffer) {
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
  fn: @freeTreeMem

  param1 Node*: pointer to the root node

  return:

  desc: frees the json token tree

  note: recursive

\**************************************/
static void freeTree(Node* node) {
	if (node->ChildCount > 0) {
		for (uint32 i = 0; i < node->ChildCount; i++)
			freeTree(&node->ChildNodes[i]);
		node->ChildCount = 0;
		if (node->ChildNodes == NULL)
			return;
		free(node->ChildNodes);
		node->ChildNodes = NULL;
	}
}

/*************************************\
  fn: @getJsonString

  param1 JsonFuncs_InputType: decides if param3 is raw json or a file path
  param2 char**: place to store the json string
  param3 char*: the raw json string or file path

  return JsonFuncs_Return: error code

  desc: gets the json string from the file and removes whitespace or just removes whitespace

  note:

\**************************************/
static JsonFuncs_Return getJsonString(JsonFuncs_InputType fileOrString, char** destination,
                                      char* jsonFileOrString) {
	if (fileOrString == JSONFUNCS_INPUT_JSONFILE) {
		char*            fileContents;
		JsonFuncs_Return ret = getFileToString(jsonFileOrString, &fileContents);
		if (ret != JSONFUNCS_OK)
			return ret;

		*destination = getStringWithoutWhitespace(fileContents, strlen(fileContents));
		free(fileContents);
	} else {
		*destination = getStringWithoutWhitespace(jsonFileOrString, strlen(jsonFileOrString));
	}

	return JSONFUNCS_OK;
}
