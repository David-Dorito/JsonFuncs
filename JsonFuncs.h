#ifndef JSONFUNCS_H
#define JSONFUNCS_H

#include <stddef.h>
#include <ctype.h>

typedef enum {
	JSONFUNCS_UINT8,
	JSONFUNCS_UINT16,
	JSONFUNCS_UINT32,
	JSONFUNCS_UINT64,
	JSONFUNCS_INT8,
	JSONFUNCS_INT16,
	JSONFUNCS_INT32,
	JSONFUNCS_INT64,
	JSONFUNCS_FLOAT,
	JSONFUNCS_DOUBLE,
	JSONFUNCS_CHAR,
	JSONFUNCS_STRING,
	JSONFUNCS_BOOL,
	JSONFUNCS_ARRAY,
} JsonFuncs_Type;

typedef struct {
	const char*    KeyName;
	void*          Destination;
	void*          IsDestinationSet;
	JsonFuncs_Type Type;
} JsonField;

typedef enum {
	JSONFUNCS_OK = 0,
	JSONFUNCS_ERROR_INVALID_JSON,
	JSONFUNCS_ERROR_MALLOC_FAILED,
	JSONFUNCS_ERROR_NESTED_VALUE,
	JSONFUNCS_ERROR_INVALID_PATH,
	JSONFUNCS_ERROR_NULL_PTR,
	JSONFUNCS_ERROR_TYPE_MISMATCH,
} JsonFuncs_Return;

typedef enum {
	JSONFUNCS_INPUT_JSONSTRING,
	JSONFUNCS_INPUT_JSONFILE,
} JsonFuncs_InputType;

/*************************************\
  fn: @JsonFuncs_Deserialize

  param1 char*: raw json string
  param2 JsonField*: array of fields to deserialize into
  param3 int: amount of fields in the array
  param4 JsonFuncs_InputType: determines if param1 is a json string or a file to get json from

  return JsonFuncsError: error code, look into JsonFuncs_Return enum for possible values

  desc: takes a json string or file path and deserializes the data and saves it

  note: doesnt support arrays yet, also you cant put a struct inside the fields array,
        only basic types (int, float, bool, char*, etc) and bools are u8's

\**************************************/
JsonFuncs_Return JsonFuncs_Deserialize(char* rawJson, JsonField* fields, int fieldCount,
                                       JsonFuncs_InputType fileOrString);

/*************************************\
  fn: @JsonFuncs_Serialize

  param1:

  return:

  desc: takes a json string and deserializes the values into the provided fields

  note:

\**************************************/
void JsonFuncs_Serialize();

#endif
