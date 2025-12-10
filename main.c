#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "JsonMethods.h"

char* FileToString(const char* filename);

int main(void)
{
    char* fileContents = FileToString("input.json");

    if (!fileContents) {
        printf("Failed to read JSON file.\n");
        return 1;
    }

    JsonMethods_Deserialize(fileContents);

    free(fileContents);

    return 0;
}

char* FileToString(const char* fileName)
{
    FILE* file = fopen(fileName, "rb");
    if (!file) return NULL;

    // move to end to determine file size
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    rewind(file);

    // allocate buffer
    char* buffer = malloc(size + 1);
    if (!buffer)
    {
        fclose(file);
        return NULL;
    }

    // read file
    fread(buffer, 1, size, file);
    buffer[size] = '\0'; // null-terminate

    fclose(file);
    return buffer;
}