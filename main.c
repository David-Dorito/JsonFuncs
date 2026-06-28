#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "JsonFuncs.h"

typedef struct {
	char*   Username;
	uint8_t Level;
	uint8_t IsLevelSet;
} Player;

char* fileToString(const char* filename);

int main(void) {
	char* fileContents = fileToString("input.json");

	if (fileContents == NULL) {
		printf("Failed to read JSON file.\n");
		return 1;
	}

	Player player;

	JsonField fields[] = {
	    (JsonField){
	        .KeyName = "Player.Username",
	        .Destination = &player.Username,
	        .IsDestinationSet = NULL,
	        .Type = JSONFUNCS_STRING,
	    },
	    (JsonField){
	        .KeyName = "Player.Level",
	        .Destination = &player.Level,
	        .IsDestinationSet = &player.IsLevelSet,
	        .Type = JSONFUNCS_UINT8,
	    },
	};
	JsonFuncs_Return ret = JsonFuncs_Deserialize(fileContents, fields, sizeof(fields) / sizeof(fields[0]),
	                                             JSONFUNCS_INPUT_JSONSTRING);
	printf("error code: %d\n", ret);
	if (ret != JSONFUNCS_OK) {
		free(fileContents);
		return 1;
	}

	printf("player username: %s\n", player.Username);
	printf("player level: %d\n", player.Level);
	printf("player level set: %d\n", player.IsLevelSet);

	free(player.Username);
	free(fileContents);

	return 0;
}

char* fileToString(const char* fileName) {
	FILE* file = fopen(fileName, "rb");
	if (!file)
		return NULL;

	// move to end to determine file size
	fseek(file, 0, SEEK_END);
	long size = ftell(file);
	rewind(file);

	// allocate buffer
	char* buffer = malloc(size + 1);
	if (!buffer) {
		fclose(file);
		return NULL;
	}

	// read file
	fread(buffer, 1, size, file);
	buffer[size] = '\0'; // null-terminate

	fclose(file);
	return buffer;
}
