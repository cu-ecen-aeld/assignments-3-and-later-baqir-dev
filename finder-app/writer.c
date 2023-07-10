#include <stdio.h>
#include <errno.h>
#include <syslog.h>
#include <string.h>

int main(int argc, char **argv){
    const char* pchWriteFile = NULL;
    const char* pchWriteStr = NULL;
    if(argc != 3){
        printf("Invalid arguments\n");
        printf("Total number or arguments should be 2.\n");
        printf("The order of arguments should be:\n1) File Directory Path\n2) String to be written in the specified file\n");
        return 1;
    }
    pchWriteFile = argv[1];
    pchWriteStr = argv[2];
    openlog(NULL,0,LOG_USER);
    FILE* poFile = fopen(pchWriteFile, "w");
    if(poFile == NULL){
        printf("Failed to open file\n");
        syslog(LOG_ERR, "Error: Failed to open file. FileName: %s Error: %s", pchWriteFile, strerror(errno));
        return 1;
    }
    syslog(LOG_DEBUG, "Writing %s to %s", pchWriteStr, pchWriteFile);
    fputs(pchWriteStr, poFile);
    fclose(poFile);
    return 0;
}
