#ifndef JSONMETHODS_H
#define JSONMETHODS_H

typedef struct {
    char* KeyName;
    void* Destination;
    size_t Size;
} JsonField;

void JsonMethods_Deserialize(char* RawJson, JsonField* pFields, int fieldAmount);
void JsonMethods_Serialize();

#endif