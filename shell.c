#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <signal.h>
#include <time.h>

int cnt = 0;
int true = 1;
int false = 0;
int bg;
int arrayLength = 20;
pid_t pidArray[20];
int jobIDArray[20];
FILE *targetFile;
char *pipedCommand[20];
char *fgID = NULL;

/**
 * Method that generates a random number for job ID's
 * @return
 */
int generateRandomNumber() {
    int min = 1;
    int max = 9999;

    srand(time(NULL));

    return rand() % (max - min + 1) + min;
}

/**
 * Method that checks if the randomly generated jobID already exists
 * @param jobID
 * @return
 */
int doesIdExist(int jobID) {
    for (int i = 0; i < arrayLength; i++) {
        if (jobIDArray[i] == jobID) {
            return true;
        }
    }
    return false;
}

/**
 * Method to check if inputted line from user is empty space
 *
 * @param line
 * @return true if whitespace found
 *
 */
int isLineEmpty(char *line) {
        if (isspace(line[0])) {
            // If any non-whitespace character is found, return true
            return true;
        }
    return false;
}

/**
 * Method that determines if a redirect is prompted and sorts arguments
 * @param index
 * @param args
 * @return
 */
int outputRedirect(int index, char *args[]) {
    if (strcmp(args[index], ">") == 0) {
        if (args[index+1] != NULL) {
            close(1);
            targetFile = fopen(args[index+1], "w");
            args[index] = NULL;
            args[index+1] = NULL;
            cnt = cnt - 2;
            return true;
        } else if (args[index+1] == NULL) {
            perror("fopen");
        }
    }
    return false;
}

/**
 * Method that checks if pipe is requested and sorts arguments
 * @param index
 * @param args
 * @return
 */
int pipeChecker(int index, char *args[]) {
    int count = 0;
    if (strcmp(args[index], "|") == 0) {
        args[index] = NULL;
        for (int i = 0; i < cnt; i++) {
            pipedCommand[i] = NULL;
        }

        if (args[index+1] != NULL) {
            for (int i = index + 1; args[i] != NULL; i++) {
                pipedCommand[count] = args[i];
                count++;
                args[i] = NULL;
            }
            return true;
        }

    }
    return false;
}

/**
 * Method that executed external commands
 * @param args
 */
void externalCommands(char *args[]) {
    int requestedPipe = false;
    int requestedRedirect = false;
    int fdt[2];

    pid_t pid = fork();
    if (pid == 0) {         // portion executed by child
        for (int i = 0; i < cnt; i++) {
            if (outputRedirect(i, args) == true) {
                requestedRedirect = true;
                break;
            }
        }

        for (int i = 0; i < cnt; i++) {
            if (pipeChecker(i, args) == true) {
                requestedPipe = true;
                if (pipe(fdt) < 0) {
                    perror("pipe");
                }
                break;
            }
        }

        pid_t pipedChild = -1;

        if (requestedPipe == true) {
            pipedChild = fork();
        }

        if (pipedChild == 0) {
            dup2(fdt[1], 1);
            close(fdt[0]);
            execvp(args[0], args);
        } else {
            if (requestedPipe == true) {
            dup2(fdt[0], 0);
            waitpid(pipedChild, NULL, 0);
            close(fdt[1]);
            execvp(pipedCommand[0], pipedCommand);
            } else {
                execvp(args[0], args);
            }
        }

        if (requestedRedirect == true) fclose(targetFile);

        if (requestedPipe == true) {
            close(fdt[1]);
        }

        perror("execvp");

    } else {                // portion executed by parent
        if (bg == 0) {
            int status;
            waitpid(pid, &status, 0);
        } else {
            for (int i = 0; i < arrayLength; i ++) {
                if (pidArray[i] == 0) {     // places PID at first empty spot in array
                    pidArray[i] = pid;
                    while (1){
                        int jobID = generateRandomNumber();
                        if (doesIdExist(jobID) == false) {
                            jobIDArray[i] = jobID;
                            break;
                        }
                    }
                    break;
                }
            }
        }
    }
}

/**
 * Method that echo's users input
 * @param args tokens inputted
 * @param tokens Number of tokens inputted
 * @return
 */
int echo(char *args[], int tokens) {
    if (args == NULL) {
        printf("Error: NULL pointer passed as argument.\n");
        return 1;
    }
    for (int i = 1; i < tokens; i++) {
        if (args[i] == NULL) {
            printf("Error: NULL pointer found in array.\n");
            return 1;
        }
        printf("%s ", args[i]);
    }
    return 0;
}

/**
 * Method that prints the current working directory
 * @return
 */
int pwd() {
    char workingDir[1024];

    if (getcwd(workingDir, sizeof(workingDir)) != NULL) {
        printf("%s", workingDir);
        return 0;
    } else {
        perror("getcwd");
        return 1;
    }
}

/**
 * Method that changes directory, and prints the current working
 * directory if no argument is found
 * @param directory
 * @return
 */
int cd(char *directory) {
    if (directory == NULL) {
        pwd();
    } else {
        if (chdir(directory) != 0) {
            perror("chdir");
            return 1;
        }
    }
    return 0;
}

/**
 * Method that terminates the shell and all background processes
 * @return
 */
int terminate() {
    for (int i = 0; i < arrayLength; i++) {
        if (pidArray[i] != 0) {
            pid_t pid = pidArray[i];
            kill(pid, SIGKILL);
        }
    }
    exit(0);
}

/**
 * Method that prints all background processes and associated numbers to the user
 * @return
 */
int jobs() {
    printf("%-10s %-10s\n", "ID", "PID");
    for (int i = 0; i < arrayLength; i++) {
        if (pidArray[i] != 0) {
            printf("%-10d %-10d\n", jobIDArray[i], pidArray[i]);
        }
    }
    return 0;
}

/**
 * Method that brings a background process to the foreground
 * @param
 * @return
 */
int fg() {
    int jobID;
    pid_t pid;
    if (fgID != NULL) {
        jobID = atoi(fgID);
        for (int i = 0; i < arrayLength; i++) {
            if (jobIDArray[i] == jobID) {
                pid = pidArray[i];
                waitpid(pid, NULL, 0);
                pidArray[i] = 0;
                jobIDArray[i] = 0;
                return 0;
            }
        }
        printf("Incorrect JobID Inputted\n");
        return 1;
    }
    for (int i = 0; i < arrayLength; i++) {
        if (jobIDArray[i] != 0) {
            pid = pidArray[i];
            waitpid(pid, NULL, 0);
            pidArray[i] = 0;
            jobIDArray[i] = 0;
            return 0;
        }
    }
    return 1;
}

/**
 * Method that executes command from user
 * @param file
 * @param args
 */
void executeCommand(char *file, char **args) {
    if (strcmp(file, "echo") == 0) {
        echo(args, cnt);
    } else if (strcmp(file, "cd") == 0) {
        cd(args[1]);
    } else if (strcmp(file, "pwd") == 0) {
        pwd();          // change this back
    } else if (strcmp(file, "exit") == 0) {
        terminate();
    } else if (strcmp(file, "jobs") == 0) {
        jobs();
    } else if (strcmp(file, "fg") == 0) {
        fgID = args[1];
        fg();
    }
    else {
        externalCommands(args);
    }
}


int getcmd (char *prompt, char *args[], int *background) {
    int length, i = 0;
    char *token, * loc;
    char *line = NULL;
    size_t linecap = 0;

    printf("%s", prompt);
    length = getline(&line, &linecap,stdin);

    if (length <= 0) {
        exit (-1);
    }

    if (isLineEmpty(line) == 1) {
        free(line);
        return 0;
    }

    // Check if background is specified
    if ((loc = index(line, '&')) != NULL) {
        *background = 1;
        *loc = ' ';
    } else
        *background = 0;

    while ((token = strsep(&line, " \t\n")) != NULL) {
        for (int j = 0; j < strlen(token); j++) {
            if (token[j] <= 32) {
                token[j] = '\0';
            }
        }
        if (strlen(token) > 0) {
            args[i++] = token;
        }
    }

    return i;
}

int main(void) {
    char *args[arrayLength];

    for (int i = 0; i < arrayLength; i++) {
        args[i] = NULL;
    }

    while(1) {

        if (cnt > 0) {
            free(args[0]);
            for (int i = 0; i < 20; i++) {
                args[i] = NULL;
            }
        }

        bg = 0;

        cnt = getcmd("\n>> ", args, &bg);

        if (cnt != 0) {
            executeCommand(args[0], args);
        }

        /*
         * After every iteration, checks the background jobs
         * and if they have terminated, removes them from array
         */
        for (int i = 0; i < arrayLength; i++) {
            if (pidArray[i] != 0) {
                int status;
                pid_t pid = pidArray[i];
                if (waitpid(pid, &status, WNOHANG) > 0) {
                    pidArray[i] = 0;    // removes the terminated process from the array
                    jobIDArray[i] = 0;
                }
            }
        }

    }
}