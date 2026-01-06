#ifndef JSONFUNCS_H
#define JSONFUNCS_H

#include <ctype.h>

typedef struct {
    const char* KeyName;
    void* Destination;
    size_t Size;
} JsonField;

typedef enum {
    JSONFUNCS_OK = 0,
    JSONFUNCS_ERROR_INVALID_JSON,
    JSONFUNCS_ERROR_MALLOC_FAILED,
    JSONFUNCS_ERROR_NESTED_VALUE
} JsonFuncsReturn;

/*************************************\
 * fn: @JsonMethods_Deserialize 
 * 
 * param1 char*: raw json string
 * param2 JsonField*: array of fields to deserialize into
 * param3 int: amount of fields in the array
 * 
 * return JsonMethodsError: error code, look into JsonMethodsError enum for possible values
 * 
 * desc: takes a json string and deserializes the values into the provided fields
 * 
 * note: doesnt support arrays yet, also you cant put a struct inside the fields array,
 *       only basic types (int, float, bool, char*, etc) and bools are u8's
 * 
\**************************************/
JsonFuncsReturn JsonFuncs_Deserialize(char* RawJson, JsonField* pFields, int fieldAmount);

/*************************************\
 * fn: @JsonMethods_Serialize 
 * 
 * param1:
 * 
 * return: 
 * 
 * desc: takes a json string and deserializes the values into the provided fields
 * 
 * note:
 * 
\**************************************/
void JsonFuncs_Serialize();

#endif