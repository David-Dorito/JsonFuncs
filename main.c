#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "JsonMethods.h"

char* FileToString(const char* filename);
char* StringWithoutWhitespace(const char* array, int len);

int main(void)
{
    char* FileContents = FileToString("input.json");

    if (!FileContents) {
        printf("Failed to read JSON file.\n");
        return 1;
    }

    printf("File contents:\n%s\n", FileContents);

    JsonMethods_Deserialize(FileContents);

    free(FileContents);

    return 0;
}

char* FileToString(const char* filename)
{
    FILE* file = fopen(filename, "rb");
    if (!file) return NULL;

    // move to end to determine file size
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    rewind(file);

    // allocate buffer
    char* buffer = malloc(sizeof(char) * size + sizeof(char));
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