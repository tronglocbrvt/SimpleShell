#include "process.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>

char OLDPWD[MAX_LENGTH] = "";
/*
 * Create loop for the shell, process history and exit
*/
void shLoop() 
{
    char * command = 0;
    char * last_command = 0;
    char ** args = 0;
    // Infinte Loop
    do
    {
        printf("PLTsh> ");
        // if last command and command refer to the same memory - don't free memory of the history
        if (last_command != command && last_command)
            free(last_command);
        last_command = command;

        // read command
        command = readLine();

        // if the command is "exit" -> exit
        if (strcmp(command,"exit")==0)
        {
            free(command);
            if (last_command)
                free(last_command);
            break;
        }

        // if the command is "!!", get the last_command
        if (strcmp(command,"!!")==0) {
            free(command);
            command = last_command;
            // if there is a command in the history -> echo it, otherwise: error.
            if (command)
                printf("%s\n",command);
            else 
                printf("No command in the history!\n");
        }
        
        if (command) {
            // mode to check whether there is a "&" symbol in the arguments
            int mode = 0;
            
            // Parse the command
            args = parseArgs(command,&mode);
            
            // call processParallel 
            processParallel(args,mode);

            // Free memory of args
            freeArgs(args);
        }

    }
    while(1);
}

/*
 * Process the ampersand (&) operator, creating a new subshell and execute the command within that new subshell. The main shell does not wait for subshell to finish.
 * Input: 
 *	(1) char ** args : the white-space-parsed command.
 *	(2) int mode: 1 if '&' was specified and 0 if not.
 * NOTE: Called by shLoop().
*/
void processParallel(char ** args, int mode)
{
    // NOTE: This function should not be taking mode as an arg, and should be returning an execution status (int).
	// if & exists
	if (mode == 1) {
		pid_t pid = fork();
		if (pid < 0) {
			perror("[Error] Can not create new subshell for execution. Command aborted");
			return;
		}
		else if (pid == 0) {
			processPipe(args);
			exit(0);
		}
		else {
		
		}
	}
	else {
		processPipe(args);
	}
}

/*
 * Detect and process the pipe (|) operators. Connect the STDOUT of the command to the left of each '|' with STDIN of the command to the right of it, and sequentially execute each command in a new child process.
 * Input:
 *	 char **args : the parsed command line. Specifically this command has to be parsed using whitespace, and stripped of the trailing '&'.
 * NOTE: Called by processParallel().
*/
void processPipe(char ** args)
{
    // find position of | character via positionPipe function.
    int position = positionPipe(args);
    // if don't exist | character => call Function to process Redirect Command
    if (position == -1)
    {
        processRedirectCommand(args);
    }
    // if | character is at the first argument => syxtax error
    else if (position == 0)
    {
        printf("[Error] Syntax Error\n");
        return;
    }
    else
    {
        // file descriptor. There are file types: fds[0]: read end of pipe; fds[1]: write end of pipe.
        int fds[2];
        // make pipe
        int create_pipe = pipe(fds); 

        // if make pipe failed
        if (create_pipe == -1) 
            { 
                perror("Create pipe failed");
                return;
            }

            // Get arguments before and after | character
            char ** argsFirst = parseFirstArgsPipe(args, position);
            char ** argsSecond = parseSecondArgsPipe(args, position);
        
            if (argsFirst && argsSecond)
            {
                pid_t pid = fork();
                // error
		        if (pid < 0) 
                {
                    freeArgs(argsFirst);
                    freeArgs(argsSecond);
			        perror("[Error] Can not create child process. Failed to execute command.");
			        return;
		        }
		        else if (pid == 0)
                {     
                    // duplicates fds[1] to standard output. 
                    dup2(fds[1], STDOUT_FILENO);
                    if (close(fds[0]) == -1 || close(fds[1]) == -1)
                    {
                        perror("[Error] Close file descriptor failed");
                        exit(EXIT_FAILURE);
                    }
                    // process first command 
                    processRedirectCommand(argsFirst);
                    exit(0);
                }

                pid = fork();
                // error
                if (pid < 0) 
                {
                    freeArgs(argsFirst);
                    freeArgs(argsSecond);
			        perror("[Error] Can not create child process. Failed to execute command.");
			        return;
		        }
                else if (pid == 0)
                {
                    // duplicates fds[0] to standard input. 
                    dup2(fds[0],STDIN_FILENO);
                    if (close(fds[1]) == -1 || close(fds[0]) == -1)
                    {
                        perror("[Error] Close file descriptor failed");
                        exit(EXIT_FAILURE);
                    }
                    // process second command 
                    processRedirectCommand(argsSecond);
                    exit(0);
                } 
                
                // close file descriptors
                close(fds[0]);
                close(fds[1]);
                
                // wait for first command
                wait(0); 
                // wait for second command
                wait(0);
            }   
        // Delete argsFirst and argsSecond arguments.
        freeArgs(argsFirst);
        freeArgs(argsSecond);
    }
}
/*
   * Identifies and processes the redirection operator ( ">", "<" ). Redirects input or output to files specified in the command.
   * Input: array of command's arguments
   * Output: None
   * NOTE: called by processPipe().
*/
void processRedirectCommand(char **args)
{
        // get number of command's arguments 
        int numArgs = getNumArgs(args);
        int fd_out = 0;
        int fd_in = 0;
        // when number of arugments is greater than 2, the command could include the redirection operators. 
        if (numArgs>2)
            {
                // ">" operator
                if (strcmp(args[numArgs-2],">")==0) 
                {
                        // Create new file
                        fd_out =creat(args[numArgs-1],S_IRWXU);
                        
                        // Error 
                        if (fd_out==-1)
                        {
                            perror("Redirect output failed");
                            return;
                        }
                        
                        // Save current stdout to turn back latter 
                        int saved_stdout = dup(STDOUT_FILENO);

                        // Redirect STDOUT to fd_out 
                        dup2(fd_out,STDOUT_FILENO);

                        // Close file 
                        if (close(fd_out) == -1)
                        {
                            perror("Close output failed");
                            return;
                        }    

                        // Delete 2 last arguments then call processSimpleCommand to process a simple command.
                        free(args[numArgs-1]);
                        free(args[numArgs-2]);
                        args[numArgs-2] = 0;
                        processSimpleCommand(args);
                        
                        // Redirect STDOUT to the saved stdout. 
                        dup2(saved_stdout,STDOUT_FILENO);
                        
                        // close
                        if (close(saved_stdout) == -1)
                        {
                            perror("Close output failed");
                            return;
                        }   
                        
                        return;
                }
                // "<" command
                else if (strcmp(args[numArgs-2],"<")==0)
                {
                        // Open file
                        fd_in =open(args[numArgs-1],O_RDONLY);
                        
                        // Error 
                        if (fd_in==-1)
                        {
                            perror("Redirect input failed");
                            return;
                        }

                        // Save current stdin to turn back latter 
                        int saved_stdin = dup(STDIN_FILENO);
                        
                        // Redirect STDIN
                        dup2(fd_in,STDIN_FILENO);

                        // Close file 
                        if (close(fd_in) == -1)
                        {
                            perror("Close input failed");
                            return;
                        }

                        // Delete 2 last arguments then call processSimpleCommand to process a simple command.
                        free(args[numArgs-1]);
                        free(args[numArgs-2]);
                        args[numArgs-2] = 0;   
                        processSimpleCommand(args); 
                        
                        // Redirect STDIN to the saved stdin. 
                        dup2(saved_stdin,STDIN_FILENO);
                        
                        // close
                        if (close(saved_stdin) == -1)
                        {
                            perror("Close input failed");
                            return;
                        }
                        return;

                }
        }
        
    // if there is no redirection command, just call processSimleCommand
    processSimpleCommand(args);

}

/*
 * Process a simple command ( without any redirection, pipe,...). Process both Internal and External Commands
 * Input: array of command's arguments
 * Output: None
 * NOTE: called by processRedirectCommand().
*/
void processSimpleCommand(char **args)
{
    int status = executeInternalCommand(args);
	if (status == -1) {
		// No built-in command available -> Outsource it.
		executeExternalCommand(args);
	}
}

/* process and call execvp().
 * Input: The external command, more specifically its arguments.
 * NOTE: Called by processSimpleCommand().
*/
void executeExternalCommand(char ** args)
{
    pid_t pid = fork(); // clones the program, produces a child process from a parent process
    if (pid < 0) {
        // fork failed
        fprintf(stderr, "[Error] Can not create child process. Failed to execute command.\n");
    }
    else if (pid == 0) {            // children process
        int exeStatus = execvp(args[0], args);
        if (exeStatus < 0) {
            fprintf(stderr, "[Error] Invalid command.\n");
            exit(1);// this one is crucial, without it the child process will not terminate
        }
    }
    else {
        while (wait(NULL) != pid);  // wait for child process
    }
}

/*
 * Execute command that is built-in in the shell itself, for example 'cd'.
 * Input: The built-in command.
 * NOTE: Called by processSimpleCommand().
*/
int executeInternalCommand(char ** args)
{
	/*
	NOTE: this part is supposed to take care of !! and exit, but Phuc moved it to the main loop.
	I will handle this later.
	*/
	// handle 'cd' 
    if (strcmp(args[0],"cd") == 0) {
		char savedPwd[MAX_LENGTH]; // save a current directory
		char *path;

		// set the correct path
		if (args[1] == NULL) {
			// cd without arguments jumps to the home directory
			path = getenv("HOME");
		}
		else if (strcmp(args[1], "-") == 0) {
			// cd - returns to the previous working directory
			if (strcmp(OLDPWD, "") == 0) {
				perror("[Error] No previous working directory.\n");
				return 0; // cd unsucessful
			}
			else printf("%s\n", OLDPWD); // echo the previous path
			path = OLDPWD;
		}
		else path = args[1];

		strcpy(savedPwd, getenv("PWD"));
		// call chdir()
		if (chdir(path) != 0) {
			perror("[Error] No such file or directory.\n");
			return 0;// cd unsuccessful
		} 
		strcpy(OLDPWD, savedPwd); // save new directory
		return 1; // cd successful
	}

	return -1; // No matching built-in command
}
