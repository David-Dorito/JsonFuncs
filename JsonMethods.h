#ifndef JSONMETHODS_H
#define JSONMETHODS_H

typedef struct {
    char* KeyName;
    void* Destination;
    size_t Size;
} JsonField;

typedef enum {
    JSONMETHODS_ERROR_NONE = 0,
    JSONMETHODS_ERROR_INVALID_JSON,
    JSONMETHODS_ERROR_MALLOC_FAILED,
    JSONMETHODS_ERROR_UNKNOWN
} JsonMethodsError;

JsonMethodsError JsonMethods_Deserialize(char* RawJson, JsonField* pFields, int fieldAmount);
void JsonMethods_Serialize();

#endif