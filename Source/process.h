#pragma once
void shLoop();
void processParallel(char ** args, int mode); 
void processPipe(char ** args);
void processSimpleCommand(char **args);
void processRedirectCommand(char **args);
void executeExternalCommand(char ** args);
int executeInternalCommand(char ** args);
