#include <stdlib.h>
#include <sys/wait.h>
#include<sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h> 

void error() 
{
    char error_message[30] = "An error has occurred\n";
    write(STDOUT_FILENO, error_message, strlen(error_message));
}

void myPrint(char *msg)
{
    write(STDOUT_FILENO, msg, strlen(msg));
}


int space_checker(char *pinput) { 
    int result = 0; 
    for (int j=0; j<strlen(pinput); j++) {
        if (isspace(pinput[j]) != 0) {
            result +=1; 
        }
    }
    if (result == strlen(pinput)) {
        return 1; 
    } else {
        return 0; 
    }  
}

char **semicolonparser(char *semicoloncl) 
{
    const char space[4] = ";"; char *token; 
    token = strtok(semicoloncl, space); 
    char **result = (char **)malloc(sizeof(char*) * 514);
    int j=0;
    while (token != NULL) { 
        result[j] = token; 
        token = strtok(NULL, space);
        j++; 
    }
    return result; 
}

char **redirectparser(char *wholecl, int *status) {
    const char carrot[4] = ">"; char *token; 
    token = strtok(wholecl, carrot); 
    char **result = (char **)malloc(sizeof(char*) * 514);
    int j=0;
    while (token != NULL) { 
        result[j] = token; 
        token = strtok(NULL, carrot);
        j++; 
    }
    if (result[1] == NULL) { 
        *status = 0; 
    } else { 
        if (result[1][0] == '+') {
            *status = 2; 
            const char plus[4] = "+ \t\n"; char *tk; 
            tk = strtok(result[1], plus); 
            result[1] = tk; 
        } else {
            *status = 1; 
            const char space[4] = " \t\n"; char *toki; 
            toki = strtok(result[1], space); 
            result[1] = toki; 
        }
    }
    return result;
}

char **parser(char *cl) 
{
    const char space[4] = " \t\n"; char *token; 
    token = strtok(cl, space); 
    char **result = (char **)malloc(sizeof(char*) * 514);
    int j=0;
    while (token != NULL) { 
        result[j] = token; 
        token = strtok(NULL, space);
        j++; 
    }
    return result; 
}

void cd(char **parsed) 
{
    if (parsed[2]) {
        error();
    } else if (parsed[1] == NULL) {
        char *home = "/home";
        int cd = chdir(home);
        if (cd != 0) {
            error();
        }
    } else {
        int cd_2 = chdir(parsed[1]);
        if (cd_2 != 0) {
            error();
        }
    }
}


void pwd(char **parsed)
{
    char pwd[512]; 
    if (getcwd(pwd, sizeof(pwd)) == NULL) {
        exit(0); //change for correct error message
    } else {
        myPrint(pwd); 
        myPrint("\n");
    }
}

int main(int argc, char *argv[]) 
{ 
    int fc; 
    int batch_file; 
    int carrot = 0; 
    if (argc == 2) {
        struct stat buf;
        int fd = stat(argv[1], &buf);
        if (fd == 0) {
            batch_file = open(argv[1], O_RDONLY, 0664);
            dup2(batch_file, STDIN_FILENO);
        } else {
            error();
            exit(0);
        }
    }
    if (argc > 2) {
        error();
        exit(0);
    }
    char cmd_buff[4096];
    char *pinput;
    while (1) {
        int dontrun = 0;
        if (argc == 1) 
            myPrint("myshell> ");
        pinput = fgets(cmd_buff, 4096, stdin); 
        if (!pinput) {
            exit(0);
        } 
        // if (strcmp(pinput, "\n") == 0) {
        //     dontrun = 1; 
        // }
        if (argc == 2) { 
            if (space_checker(pinput) == 0)
                myPrint(pinput);
        } 
        if (strlen(pinput) > 513) {
            error();
            dontrun = 1; 
        } 
        char **semicolonparsed = semicolonparser(pinput); 
        if (!semicolonparsed) {
            error();
        }
        int i=0;
        while (semicolonparsed[i] != NULL) {
            //int newfile = 0;
            int semicolon_null = 0; 
            int *status = malloc(sizeof(int)); 
            *status = 0; 
            for (int j=0; j<strlen(semicolonparsed[i]); j++) {
                if (semicolonparsed[i][j] == '>') {
                    carrot += 1; 
                }
            } 
            char **redirectparsed = redirectparser(semicolonparsed[i], status);
            if (strcmp(semicolonparsed[0], "exit\n") == 0) {
               exit(0);
           }
            char **parsed = parser(redirectparsed[0]);
            if (!parsed[0])
                semicolon_null = 1;
            if (dontrun == 0 && semicolon_null == 0) {
            if (strcmp(parsed[0], "exit") == 0) {
                if (parsed[1] || carrot > 0) {
                    error();
                } else {
                    exit(0);
                }
            } else if (strcmp(parsed[0], "cd") == 0) {
                if (*status == 0) {
                    cd(parsed);
                } else {
                    error();
                } 
            } else if (strcmp(parsed[0], "pwd") == 0) {
                if (*status ==0 && parsed[1] == NULL) {
                    pwd(parsed);
                } else { 
                    error();
                }
            } else { 
                char *instruction = parsed[0];
                char *outfile = redirectparsed[1]; //if outfile is null, raise an error 
                char **list = parsed; 
                // if status is 2 but file doesn't exist, make status ==1; 
                if (*status == 2) {
                    struct stat statuss;
                    int fd = stat(outfile, &statuss); 
                    if (fd == -1) {
                        *status = 1; 
                    }
                }

                if (fork()==0) {
                    int file;
                    //simple redirection
                    if (*status == 1) {  
                        struct stat buffer;
                        int fd = stat(outfile, &buffer); 
                        if (fd == 0) { 
                            error();
                            exit(0); 
                        } else {
                            file = open(outfile, O_WRONLY | O_CREAT, 0666);
                            if (file < 0) { 
                                error();
                                exit(0);
                            }
                            dup2(file, STDOUT_FILENO); //where do i close file?
                            file = close(file); 
                        }
                    }
                    //advanced redirection 
                    if (*status ==2) {  
                        struct stat buff;
                        int fd_adv = stat(outfile, &buff);
                        if (fd_adv == 0) { // if file exists
                            FILE *dummy_file = fopen("dummyfile", "w");
                            if (dummy_file == NULL) {
                                error();
                                exit(0);
                            }
                            FILE *file = fopen(outfile, "r");
                            if (file) {
                                char dummy_line = fgetc(file);
                                while (dummy_line != EOF) {
                                    fputc(dummy_line, dummy_file);
                                    dummy_line = fgetc(file);
                                }
                            }
                            fc = fclose(dummy_file);
                            fclose(file);
                            fc = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, 0666);
                            if (fc < 0) {
                                error();
                                exit(0);
                            }
                            dup2(fc, STDOUT_FILENO);
                            fc = close(fc);
                        } else {
                            //newfile = 1; 
                            file = open(outfile, O_WRONLY | O_CREAT, 0666);
                            if (file < 0) {
                                error();
                                exit(0);
                            }
                            dup2(file, STDOUT_FILENO);
                            file = close(file); 
                            *status = 1; 
                        }
                    }
                    int faultchecker = execvp(instruction, list);
                    if (faultchecker == -1) { 
                        error();
                    }
                exit(0);
                } else {
                    wait(NULL); 
                    if (*status == 2) {
                        FILE *file = fopen(outfile, "a");
                        if (file == NULL) 
                            error();
                        FILE *file2 = fopen("dummyfile", "r");
                        if (file2 == NULL) 
                            error();
                        char character = getc(file2);
                        while (!feof(file2)) {
                            putc(character, file);
                            character = getc(file2);
                            // fread(&character, sizeof(char), 1, file2);
                            // fwrite(&character, sizeof(char), 1, file);
                           }
                        fclose(file);
                        fclose(file2);
                    }
                }
            }
            }
            i++; 
        }
    }
} 