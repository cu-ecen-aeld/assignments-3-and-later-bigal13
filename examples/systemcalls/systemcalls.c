#include "systemcalls.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h> // for dup2(), STDOUT_FILENO
#include <stdarg.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h> // for open(), O_WRONLY, O_CREAT, etc

/**
 * @param cmd the command to execute with system()
 * @return true if the command in @param cmd was executed
 *   successfully using the system() call, false if an error occurred,
 *   either in invocation of the system() call, or if a non-zero return
 *   value was returned by the command issued in @param cmd.
*/
bool do_system(const char *cmd)
{

/*
 * TODO  add your code here
 *  Call the system() function with the command set in the cmd
 *   and return a boolean true if the system() call completed with success
 *   or false() if it returned a failure
*/

// call the system() function with the command set in the cmd
int status = system(cmd);

if (status==0) {
    return true; // completed with success
} else {
    return false; // returned a failure
}

}

/**
* @param count -The numbers of variables passed to the function. The variables are command to execute.
*   followed by arguments to pass to the command
*   Since exec() does not perform path expansion, the command to execute needs
*   to be an absolute path.
* @param ... - A list of 1 or more arguments after the @param count argument.
*   The first is always the full path to the command to execute with execv()
*   The remaining arguments are a list of arguments to pass to the command in execv()
* @return true if the command @param ... with arguments @param arguments were executed successfully
*   using the execv() call, false if an error occurred, either in invocation of the
*   fork, waitpid, or execv() command, or if a non-zero return value was returned
*   by the command issued in @param arguments with the specified arguments.
*/

bool do_exec(int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;
    va_end(args);
    /*
    * TODO:
    *   Execute a system command by calling fork, execv(),
    *   and wait instead of system (see LSP page 161).
    *   Use the command[0] as the full path to the command to execute
    *   (first argument to execv), and use the remaining arguments
    *   as second argument to the execv() command.
    *
    */

    // flush stdout
    fflush(stdout);

    // fork a child process and check if failed
    pid_t pid = fork();

    if (pid < 0) {
        return false;
    } else if (pid == 0) { 
        // we are in the child process
        // here we execute the command with it's arguments using execv
        execv(command[0], command);

        // execv only returns on errors
        exit(EXIT_FAILURE);
        // after this exit(), the parent is notified of the exit status and child dies
        // immediately if the parent was waiting, else it becomes a zombie
    } else {
        // we are in the parent process
        // here we wait for the child process using waitpid()
        int status;

        if (waitpid(pid, &status, 0) == -1) {
            return false;
        }

        // now we check the status of the child process
        if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
            // this is the case where the child process exited normally and the
            // command returned a zero exit status! yay
            return true;
        } else {
            // something went wrong, either in the command or the child process
            return false;
        }
    }
    return true;

}
    


/**
* @param outputfile - The full path to the file to write with command output.
*   This file will be closed at completion of the function call.
* All other parameters, see do_exec above
*/
bool do_exec_redirect(const char *outputfile, int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;
    
/*
 * TODO
 *   Call execv, but first using https://stackoverflow.com/a/13784315/1446624 as a refernce,
 *   redirect standard out to a file specified by outputfile.
 *   The rest of the behaviour is same as do_exec()
 *
*/

    // open the output file; write only mode
    int fd = open(outputfile, O_WRONLY|O_TRUNC|O_CREAT, 0644);
    if (fd == -1) {
        return false;
    }

    // similar flow to the above do_exec() function
    fflush(stdout);

    pid_t pid = fork();
    if (pid < 0) {
        return false;
    } else if (pid == 0) { 
        // we are in the child process
        if (dup2(fd, 1) < 0) {
            perror("Dup2 error.");
            exit(EXIT_FAILURE);
        }
        close(fd);
        // here we execute the command with it's arguments using execv
        execv(command[0], command);

        // execv only returns on errors
        exit(EXIT_FAILURE);
        // after this exit(), the parent is notified of the exit status and child dies
        // immediately if the parent was waiting, else it becomes a zombie
    } else {
        close (fd);
        // we are in the parent process
        // here we wait for the child process using waitpid()
        int status;

        if (waitpid(pid, &status, 0) == -1) {
            return false;
        }

        // now we check the status of the child process
        if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
            // this is the case where the child process exited normally and the
            // command returned a zero exit status! yay
            return true;
        } else {
            // something went wrong, either in the command or the child process
            return false;
        }
    }
    va_end(args);
    return true;
}
