#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "JsonMethods.h"

typedef struct {
    char* Username;
    double Level;
    uint8_t IsActive;
} Player;

char* FileToString(const char* filename);

int main(void)
{
    char* fileContents = FileToString("input.json");

    if (!fileContents) {
        printf("Failed to read JSON file.\n");
        return 1;
    }

    Player player;

    JsonField pFields[] = {
        (JsonField){
            .KeyName = "Username",
            .Destination = &player.Username
        },
        (JsonField){
            .KeyName = "Level",
            .Destination = &player.Level
        },
        (JsonField){
            .KeyName = "bool",
            .Destination = &player.IsActive
        }
    };

    if (JsonMethods_Deserialize(fileContents, pFields, 2) != JSONMETHODS_ERROR_NONE) {
        printf("Failed to deserialize JSON.\n");
        free(fileContents);
        return 1;
    }

    printf("player username: %s\n", player.Username);
    printf("player level: %f\n", player.Level);
    printf("player IsActive: %d\n", player.IsActive);

    free(player.Username);
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