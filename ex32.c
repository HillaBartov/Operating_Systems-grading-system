//#Hilla Bartov 315636779 LATE-SUBMISSION
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <sys/wait.h>
#include <time.h>


void compile(char *path, char *line2, char *line3, int results_fd, char *firstPath);

void run(char *line2, char *line3, char folder[256], int results_fd, char *firstPath);

void compare(char *resultPath, char *line3, char folder[256], int results_fd, char *firstPath);

void wrightToResults(int status, char folder[256], int results_fd);


int main(int argc, char *argv[]) {
    char firstPath[PATH_MAX];
    extern int errno;

    char conf[450], *confTxt = argv[1], *line1, *line2, *line3;
    int fd;
    struct stat stat_conf;
    stat(confTxt, &stat_conf);
    if ((fd = open(confTxt, O_RDONLY)) == -1) {
        if (write(STDERR_FILENO, "Input/output File not exist\n", 25) == -1) {
            exit(-1);
        }
        exit(-1);
    }
    if (read(fd, conf, stat_conf.st_size) != stat_conf.st_size) {
        if (write(STDERR_FILENO, "read(): Error in system call\n", 25) == -1) {
            exit(-1);
        }
        exit(-1);
    }
    close(fd);
    line1 = strtok(conf, "\n");
    line2 = strtok(NULL, "\n");
    line3 = strtok(NULL, "\n");
    //save first path for the results.csv and the comp.out
    getcwd(firstPath, sizeof(firstPath));
    char resultsPath[PATH_MAX];
    strcpy(resultsPath, firstPath);
    //create the results file
    strcat(resultsPath, "/results.csv");
    int results_fd;
    if ((results_fd = open(resultsPath, O_CREAT | O_TRUNC | O_WRONLY, S_IRWXU)) == -1) {
        if (write(STDERR_FILENO, "Can not open file\n", 25) == -1) {
            exit(-1);
        }
        exit(-1);
    }
    //comple c files
    compile(line1, line2, line3, results_fd, firstPath);
    close(results_fd);
    return 0;
}

void compile(char *path, char *line2, char *line3, int results_fd, char *firstPath) {
    //current directory
    char cwd[PATH_MAX];
    getcwd(cwd, sizeof(cwd));
    printf("%s\n", cwd);
    DIR *dpParent, *dpChild;
    struct dirent *pdParent, *pdChild;
    int status = 0;
    int isCExist = 0;

    if ((dpParent = opendir(path)) == NULL) {
        if (write(STDERR_FILENO, "Not a valid directory\n", 25) == -1) {
            exit(-1);
        }
        exit(-1);
    }
    if (chdir(path) == -1) {
        if (write(STDERR_FILENO, "Error: No such file or directory\n", 50) == -1) {
            exit(-1);
        }
        exit(-1);
    }

    getcwd(cwd, sizeof(cwd));
    printf("%s\n", cwd);
    //go over first directory folders
    while ((pdParent = readdir(dpParent)) != NULL) {
        if (pdParent->d_type == DT_DIR &&
            strcmp(pdParent->d_name, ".") != 0 &&
            strcmp(pdParent->d_name, "..") != 0) {
            if ((dpChild = opendir(pdParent->d_name)) == NULL) {
                if (write(STDERR_FILENO, "Not a valid directory\n", 25) == -1) {
                    continue;
                }
                continue;
            }
            //enter dir and make the files in it
            if (chdir(pdParent->d_name) == -1) {
                if (write(STDERR_FILENO, "Error: No such file or directory\n", 50) == -1) {
                    continue;
                }
                continue;
            }
            getcwd(cwd, sizeof(cwd));
            printf("%s\n", cwd);
            isCExist = 0;
            //go over inner die files
            while ((pdChild = readdir(dpChild)) != NULL) {
                //check for a c file
                if (pdChild->d_type != DT_DIR && pdChild->d_name[strlen(pdChild->d_name) - 1] == 'c' &&
                    pdChild->d_name[strlen(pdChild->d_name) - 2] == '.') {
                    printf("%s\n", pdChild->d_name);
                    //creat a child for the gcc process
                    pid_t pid = fork();
                    if (pid == 0) {
                        char *argv[] = {"gcc", pdChild->d_name, NULL};
                        if ((execvp("gcc", argv) == -1)) {
                            if (write(STDERR_FILENO, "execvp(): Error in system call\n", 50) == -1) {
                                continue;
                            }
                            continue;
                        }
                    } else if (pid < 0) {
                        if (write(STDERR_FILENO, "fork(): Error in system call\n", 50) == -1) {
                            continue;
                        }
                        continue;
                    } else if (pid > 0) {
                        waitpid(pid, &status, 0);
                        //when compilation error is occurred, print to results
                        if (WIFEXITED(status)) {
                            int exit_status = WEXITSTATUS(status);
                            if (exit_status == EXIT_FAILURE) {
                                char name[256];
                                strcpy(name, pdParent->d_name);
                                wrightToResults(-1, name, results_fd);
                                //close(results_fd);
                            } else {
                                //run the executable file
                                run(line2, line3, pdParent->d_name, results_fd, firstPath);
                            }
                        }
                    }
                    //there is one c file at most
                    isCExist = 1;
                    break;
                }
            }
            closedir(dpChild);
            if (isCExist == 0) {
                //have no c file, print to results
                wrightToResults(0, pdParent->d_name, results_fd);
            }
            if (chdir("..") == -1) {
                if (write(STDERR_FILENO, "Error: No such file or directory\n", 50) == -1) {
                    exit(-1);
                }
                exit(-1);
            }
        }
    }
    closedir(dpParent);
}

void run(char *line2, char *line3, char folder[256], int results_fd, char *firstPath) {
    //new file descriptor for input
    int in_fd;
    if ((in_fd = open(line2, O_RDONLY)) == -1) {
        if (write(STDERR_FILENO, "Input/output File not exist\n", 25) == -1) {
            return;
        }
        return;
    }
    //check timeout
    time_t start, end;
    struct dirent *pdParent;
    DIR *dpParent;

    //get current path
    char cwd[PATH_MAX];
    getcwd(cwd, sizeof(cwd));
    printf("%s\n", cwd);

    if ((dpParent = opendir(cwd)) == NULL) {
        if (write(STDERR_FILENO, "Not a valid directory\n", 25) == -1) {
            return;
        }
        return;
    }
    //new file descriptor for output
    int out_fd;
    //check for the executable file
    while ((pdParent = readdir(dpParent)) != NULL) {
        if (strcmp(pdParent->d_name, "a.out") == 0) {
            //create new file for the result
            strcat(cwd, "/check_output.txt");
            if ((out_fd = open(cwd, O_CREAT | O_TRUNC | O_WRONLY, S_IRWXU)) == -1) {
                if (write(STDERR_FILENO, "Can not open file\n", 25) == -1) {
                    return;
                }
                return;
            }
            //count executable time
            time(&start);
            pid_t pid = fork();
            if (pid == 0) {
                char *argv[] = {"./a.out", NULL};
                //input for the executable file will be taken from line 2
                dup2(in_fd, 0);
                //output from the executable file will be written to the "/check_output.txt" file
                dup2(out_fd, 1);
                if ((execvp("./a.out", argv) == -1)) {
                    if (write(STDERR_FILENO, "execvp(): Error in system call\n", 50) == -1) {
                        return;
                    }
                    return;
                }
            } else if (pid < 0) {
                if (write(STDERR_FILENO, "fork(): Error in system call\n", 50) == -1) {
                    return;
                }
                return;
            } else if (pid > 0) {
                wait(NULL);
                time(&end);
                //check for timeout
                if (difftime(end, start) > 3) {
                    //when timeout occurred
                    wrightToResults(4, folder, results_fd);
                    if (remove("check_output.txt") != 0) {
                        if (write(STDERR_FILENO, "Can not remove file\n", 50) == -1) {
                            return;
                        }
                        return;
                    }
                } else {
                    //compare the output with the given one in line 3
                    compare(cwd, line3, folder, results_fd, firstPath);
                }
                if (remove("a.out") != 0) {
                    if (write(STDERR_FILENO, "Can not remove file\n", 50) == -1) {
                        return;
                    }
                }
                close(in_fd);
                closedir(dpParent);
            }
        }
    }
}

void compare(char *resultPath, char *line3, char folder[256], int results_fd, char *firstPath) {
    int exit_status = 0, status = 0;;
    //compare files
    pid_t pidOut = fork();
    if (pidOut == 0) {
        //move to the first directory to run the comp.out file
        if (chdir(firstPath) == -1) {
            if (write(STDERR_FILENO, "Error: No such file or directory\n", 50) == -1) {
                return;
            }
            return;
        }
        char *argv[] = {"./comp.out", line3, resultPath, NULL};
        if ((execv("./comp.out", argv) == -1)) {
            if (write(STDERR_FILENO, "execvp(): Error in system call\n", 50) == -1) {
                return;
            }
            return;
        }
    } else if (pidOut < 0) {
        if (write(STDERR_FILENO, "fork(): Error in system call\n", 50) == -1) {
            return;
        }
        return;
    } else if (pidOut > 0) {
        waitpid(pidOut, &status, 0);
        //write the return value to the comparison result file
        if (WIFEXITED(status)) {
            exit_status = WEXITSTATUS(status);
            wrightToResults(exit_status, folder, results_fd);
            if (remove("check_output.txt") != 0) {
                if (write(STDERR_FILENO, "Can not remove file\n", 50) == -1) {
                    return;
                }
            }
        }
    }
}

void wrightToResults(int status, char folder[256], int results_fd) {
    switch (status) {
        //compilation error
        case -1:
            strcat(folder, ",COMPILATION_ERROR,10\n");
            break;
            //noo c file
        case 0:
            strcat(folder, ",NO_C_FILE,0\n");
            break;
            //excellent
        case 1:
            strcat(folder, ",EXCELLENT,100\n");
            break;
            //wrong
        case 2:
            strcat(folder, ",WRONG,50\n");
            break;
            //similar
        case 3:
            strcat(folder, ",SIMILAR,75\n");
            break;
            //timeout
        case 4:
            strcat(folder, ",TIMEOUT,20\n");
            break;
        default:
            break;
    }
    if (write(results_fd, folder, strlen(folder)) == -1) {
        return;
    }
}