#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "JsonFuncs.h"

typedef struct {
    char* Username;
    uint8_t Level;
    uint8_t IsActive;
    char* test;
    char CharTest;
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
            .KeyName = "Player.Username",
            .Destination = &player.Username,
            .Size = sizeof(player.Username),
            .Datatype = JSONFUNCS_STRING
        },
        (JsonField){
            .KeyName = "Player.IsActive",
            .Destination = &player.IsActive,
            .Size = sizeof(player.IsActive),
            .Datatype = JSONFUNCS_BOOL
        },
        (JsonField){
            .KeyName = "Player.Level",
            .Destination = &player.Level,
            .Size = sizeof(player.Level),
            .Datatype = JSONFUNCS_U8
        },
        (JsonField){
            .KeyName = "Player.test.test",
            .Destination = &player.test,
            .Size = sizeof(player.test),
            .Datatype = JSONFUNCS_STRING
        },
        (JsonField){
            .KeyName = "Player.CharTest",
            .Destination = &player.CharTest,
            .Size = sizeof(player.CharTest),
            .Datatype = JSONFUNCS_CHAR
        }
    };

    if (JsonFuncs_Deserialize(fileContents, pFields, sizeof(pFields)/sizeof(pFields[0]), JSONFUNCS_INPUT_JSONSTRING) != JSONFUNCS_OK) {
        printf("Failed to deserialize JSON\n");
        free(fileContents);
        return 1;
    }

    printf("player username: %s\n", player.Username);
    printf("player IsActive: %d\n", player.IsActive);
    printf("player Level: %d\n", player.Level);
    printf("player test: %s\n", player.test);
    printf("player CharTest: %c\n", player.CharTest);

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