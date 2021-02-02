#pragma once
#define BUFFER_SIZE 16 // Buffer size to read command from the screen.
#define MAX_LENGTH 512 // Maximum length of the command.

char * readLine();
char ** parseArgs(char * str, int* mode);
void freeArgs(char ** args);
int checkMemoryValid (void * p);
int getNumArgs(char ** args);
int isInternal(char **args);
int positionPipe(char** args);
char** parseFirstArgsPipe(char ** args, int position);
char** parseSecondArgsPipe(char ** args, int position);

