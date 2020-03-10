#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

// refer: http://man7.org/linux/man-pages/man5/proc.5.html
void showMemUsage(){
    
    FILE *fp;
    char cmdOut[128];

    // Open the command for reading. 
    fp = popen("cat /proc/self/statm", "r");
    if (fp == NULL) {
        printf("Failed to run command 'cat /proc/self/statm'\n" );
        return ;
    }

    // Read the output a line at a time - output it.
    while (fgets(cmdOut, sizeof(cmdOut), fp) != NULL) ;
    pclose(fp);

    char *delim = " ";
    char *token = strtok(cmdOut, delim);
    for(int i=0;i< 6;i++){
        if (token==NULL)    break;

        if (i==0){
            printf("Program size: %s\n", token ); //printing each token
        }else if(i==1){
            printf("Resident set size: %s\n", token );
        }else if(i==2){
            printf("Number of resident shared pages: %s\n", token );
        }else if(i==3){
            printf("text: %s\n", token );
        }else if(i==5){
            printf("data + stack: %s\n", token );
        }
        token = strtok(NULL, " ");
    }
    
}

void showPsaux(){
    pid_t pid =  getpid();
    FILE *fp;
    char cmdOut[128];

    char cmd[128];
    sprintf(cmd, "ps -aux | grep %d", pid); 

    // Open the command for reading. 
    fp = popen(cmd, "r");
    if (fp == NULL) {
        printf("Failed to run command %s'\n", cmd );
        return ;
    }

    // Read the output a line at a time - output it.
    while (fgets(cmdOut, sizeof(cmdOut), fp) != NULL) ;
    pclose(fp);

    printf("ps: %s\n", cmdOut);
}

void createFolderIfNotExist(const char *dir){
    struct stat st = {0};
    if (stat(dir, &st) == -1) {
        mkdir(dir, 0755);
    }
}

void rmFolder(const char *dir){
    char cmd[128];
    sprintf(cmd, "rm -rf %s", dir); 

    system(cmd);
}

void showStatusVm(){
    FILE *fp;
    char cmdOut[128];

    char cmd[128];
    sprintf(cmd, "cat /proc/self/status | grep Vm"); 

    // Open the command for reading. 
    fp = popen(cmd, "r");
    if (fp == NULL) {
        printf("Failed to run command %s'\n", cmd );
        return ;
    }

    // Read the output a line at a time - output it.
    while (fgets(cmdOut, sizeof(cmdOut), fp) != NULL) {
        printf("%s", cmdOut);
    }
    pclose(fp);

    // printf("ps: %s\n", cmdOut);
}