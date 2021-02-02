#include "utils.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*
    * Check if a pointer is NULL or not, throw error when NULL
    * INPUT: pointer 
    * 1 if valid, 0 otherwise
*/
int checkMemoryValid(void * p)
{
    if (!p)
    {
        fprintf(stderr,"PLTsh> Allocation error\n");
        return 0;
    }
    return 1;
}


/*
    * Read a command from the screen and erase the meaningless spaces.
    * INPUT: void
    * OUTPUT: pointer to the read command.
*/
char * readLine()
{
    int arrSize = BUFFER_SIZE; // size of string array
    int n = 0; // length of string
    char c; // temporary character

    // allocate memory for the string 
    char * str = malloc(sizeof(char)*arrSize);
    
    // check if succefully allocate
    if (!checkMemoryValid(str))
        exit(EXIT_FAILURE);   

    // loop to read character by character
    while (1)
    {
        // read a character
        c = getchar();

        // end of string character
        if (c==EOF || c == '\n') 
        {
            if (n != 0 && str[n-1]==' ') 
                n--;
            str[n] = 0;
            break;
        }
        else
        {
            if (c ==' ' && n == 0) continue; // ignore leading space
            if (c ==' ' && str[n-1] ==' ') continue; // ignore consecutive space 
            str[n++] = c;
        }

        if (n>=arrSize) // check whether we need to expand ther arrSize
        {   
            // Increase the array size
            arrSize += BUFFER_SIZE;
            
            // Check if the command is too long??
            if (arrSize>MAX_LENGTH)
            {
                fprintf(stderr,"PLTsh> Command is too long\n");
                free(str);
                exit(EXIT_FAILURE);
            }

            // expand the string and check 
            str = realloc(str,sizeof(char) * arrSize);
            if (!checkMemoryValid(str))
                exit(EXIT_FAILURE);
        }
    }
    return str;
}


   /*
    *   Parse string of command to array of arguments and identify the mode of the command: 0 if the command does not include '&' and 1 otherwise.
    *   INPUT: pointer to string, pointer to an interger to receive the mode of the command
    *   OUTPUT: array of arguments
    */
char ** parseArgs(char * str, int * mode)
{
    int idx = 0,prev_idx = 0; // iterator
    int numArgs = 0; // number of arguments
    char **args = 0; // result

    // Iterate throught the string
    while (str[idx])
    {
        if (str[idx] == ' ') // if see a space 
        {
            // Increase number of arguments
            numArgs++; 
            args = realloc(args, numArgs * sizeof(char *));

            // check memory
            if (!checkMemoryValid(args))
                exit(EXIT_FAILURE);

            // Copy substring in *str to the new argument
            args[numArgs - 1] = malloc(idx - prev_idx+1);
            if (!checkMemoryValid(args[numArgs - 1]))
                exit(EXIT_FAILURE);
            memcpy(args[numArgs-1], (str+prev_idx), idx - prev_idx);

            // end of string
            args[numArgs-1][idx-prev_idx] = 0;

            // update prev_idx
            prev_idx = idx+1;

        }
        idx++;
    }

    // Update the last argument
    numArgs++;
    args = realloc(args, (numArgs) * sizeof(char *));
    if (!checkMemoryValid(args))
        exit(EXIT_FAILURE);
    args[numArgs - 1] = malloc(idx - prev_idx+1);
    memcpy(args[numArgs-1], (str+prev_idx), idx - prev_idx);
    args[numArgs-1][idx-prev_idx] = 0;

    // if the last argument is "&"
    if (strcmp(args[numArgs-1],"&")==0)
    {
        // assign the last element of the array to 0
        *mode = 1;
        free(args[numArgs-1]);
        args[numArgs-1] = 0;
    }
    else 
    {
        // allocate one more element of the array and assign it to 0
        *mode = 0;
        numArgs++;
        args = realloc(args, (numArgs) * sizeof(char *));
        if (!checkMemoryValid(args))
            exit(EXIT_FAILURE);
        args[numArgs-1] = 0;
    }

    return args;
}


/* Free memory of every element in the array of arguments
   INPUT: array of command's arguments
   OUTPUT: None
*/
void freeArgs(char ** args){
    char ** temp = args;
    while (*args)
    {
        free(*args);
        args++;
    }
    free(temp);
}


/*
    * get the number of arguments in the array of arguments.
    * INPUT: array of command's arguments
    * OUTPUT: number of arguments
*/
int getNumArgs(char ** args)
{
    int ans = 0;
    // Loop and count
    while (*args)
    {
        ans++;
        args++;
    }
    return ans;
}



/*
    * find position of | character
    * INPUT: array of command's arguments
    * OUTPUT: interger shows position of | character in args
*/
int positionPipe(char** args)
{
    int i = 0;
    int position = -1;
    
    // reverse elements of args and compare it with "|"
    while (args[i] != 0)
    {
        if (strcmp(args[i], "|") == 0)
            position = i;
        i++;
    }

    return position;
}

/*
    *  Get arguments before | character from args
    *  Input: array of command's arguments and position of argument which contains | character
    *  Output: array of arguments which are before |
*/
char** parseFirstArgsPipe(char ** args, int position)
{
    int i = 0;
    char** argsFirst = 0;

    // loop to copy data from args which before | to argsFrist corresponding
    while (i != position)
    {
        // allocation for argsFirst
        argsFirst = (char**)realloc(argsFirst, (i + 1) * sizeof(char*));
        if (!checkMemoryValid(argsFirst))
            exit(EXIT_FAILURE);

        // allocation for elements of argsFirst
        argsFirst[i] = (char*)malloc(sizeof(char) * (strlen(args[i])+1));
        argsFirst[i] = strdup(args[i]);
        i++;
    }
    
    // allocation for NULL character
    argsFirst = (char**)realloc(argsFirst, (i + 1) * sizeof(char*));
    argsFirst[i] = 0;

    return argsFirst;
}

/*
    *  Get arguments after | character from args
    *  Input: array of command's arguments and position of argument which contains | character
    *  Output: array of arguments which are after |
*/
char** parseSecondArgsPipe(char ** args, int position)
{
    int i = 0;
    char** argsSecond = 0;
    
    // loop to copy data from args which after | to argsSecond corresponding
    while (args[position + i + 1] != 0)
    {
        // allocation for argsSecond
        argsSecond = (char**) realloc(argsSecond, (i + 1) * sizeof(char*));
        if (!checkMemoryValid(argsSecond))
            exit(EXIT_FAILURE);

        // allocation for elements of argsSecond
        argsSecond[i] = (char*)malloc(sizeof(char) * (strlen(args[position + i + 1])+1));
        argsSecond[i] = strdup(args[position + i + 1]);
        i++;
    }
    
    // allocation for NULL character
    argsSecond = (char**) realloc(argsSecond, (i + 1) * sizeof(char*));
    argsSecond[i] = 0;

    return argsSecond;
}