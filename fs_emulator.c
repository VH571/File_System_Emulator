#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdint.h>
#include <string.h>
#define MAX_INODES 1024
#define MAX_NAME_LEN 32

typedef struct{
    uint32_t inode;
    char type;
} __attribute__((packed)) InodeStruct;

typedef struct {
    uint32_t inode;
    char name[MAX_NAME_LEN + 1];
} __attribute__((packed)) DirectoryStruct; 

int findDirectory(char *name,InodeStruct inodes[], DirectoryStruct directoryInfo[], int size);
int findFile(char *name,InodeStruct inodes[], DirectoryStruct directoryInfo[], int size);
void writeFile(DirectoryStruct directoryInfo[],int *directorySize,int currDirectory, int *inodeCount, char *name); 
void writeDirectory(DirectoryStruct directoryInfo[],int *directorySize,int currDirectory, int *inodeCount, char *name);
void readDirectory(int dir, DirectoryStruct directoryInfo[], int *finalSize);
void printDirecotry(DirectoryStruct directoryInfo[], int size);
void updateInodes_list(char type, int *inodeCount, InodeStruct inodes[]);


void printInodes(InodeStruct inodes[], int inodeCount) {
    printf("Inode List:\n");
    for (int i = 0; i < inodeCount; ++i) {
        printf("Inode %u: Type %c\n", inodes[i].inode, inodes[i].type);
    }
}

void printDirectoryEntries(DirectoryStruct directoryEntries[], int entryCount) {
    printf("Directory Entries:\n");
    for (int i = 0; i < entryCount; ++i) {
        printf("Entry %d - Inode: %u, Name: %s\n", i + 1, directoryEntries[i].inode, directoryEntries[i].name);
    }
}

int main(int argc,char *argv[]){
    // Check CMD_Line arg
    if (argc != 2){
        fprintf(stderr, "ERR: NO DIR INPUT\n");
        exit(1);
    }
    //Check DIR
    DIR *dir = opendir(argv[1]);
    if(dir){
        closedir(dir);
    }else{
        closedir(dir);
        fprintf(stderr, "ERR: NO DIR EXIST\n");
        exit(1);
    }
    //Change DIR
    if (chdir(argv[1]) == -1){
        fprintf(stderr, "ERR: FAILD TO CHANGE-DIR\n");
        exit(1);
    }

    //READ INODES_LIST
    InodeStruct inodes[MAX_INODES];
    int inodeCount = 0;
    FILE *fp = fopen("inodes_list", "rb");
    if (fp == NULL){
        fprintf(stderr, "ERR: FILE OPEN FAILD\n");
        exit(1);
    }
    InodeStruct temp;
    while(fread(&temp, sizeof(InodeStruct), 1, fp) == 1){

        if (temp.inode < MAX_INODES && (temp.type == 'd' || temp.type == 'f')) {
            inodes[inodeCount] = temp; 
            inodeCount++;
        }
    }
    //Verify Starting Directory
    if (inodes[0].type != 'd') {
        fprintf(stderr, "ERR: Root directory (inode 0) is not a directory or missing.\n");
        fclose(fp);
        exit(1);
    }
    int currDirectory = 0;

    int currDirectorySize = 0;
    DirectoryStruct currDirectoryInfo[MAX_INODES];
    readDirectory(currDirectory, currDirectoryInfo, &currDirectorySize);

    fclose(fp);
    char line[128];
    while ((fprintf(stdout, ">> ") ) && (fgets(line, sizeof(line), stdin))) {
        line[strcspn(line, "\n")] = 0;
        if(strncmp(line, "exit", 4) == 0) {
            break;
        } else if (strncmp(line, "cd ", 3) == 0) {
            if(findDirectory(line + 3, inodes, currDirectoryInfo, currDirectorySize) != -1){
                currDirectory = findDirectory(line+3, inodes, currDirectoryInfo, currDirectorySize);
                readDirectory(currDirectory, currDirectoryInfo, &currDirectorySize);
            }else{
                printf("No Directory Found\n");
            }

        } else if (strncmp(line, "ls", 2) == 0) {
            printDirecotry(currDirectoryInfo, currDirectorySize);

        } else if (strncmp(line, "mkdir ", 6) == 0) {
            if((findDirectory(line+6, inodes, currDirectoryInfo, currDirectorySize) == -1)){

                writeDirectory(currDirectoryInfo, &currDirectorySize, currDirectory, &inodeCount, line+6);
                updateInodes_list('d', &inodeCount, inodes);
            }else{
                printf("Directory already exists: %s\n", line+6);
            }

        } else if (strncmp(line, "touch ", 6) == 0) {
            if((findFile(line+6, inodes, currDirectoryInfo, currDirectorySize) == -1)){
                writeFile(currDirectoryInfo, &currDirectorySize, currDirectory, &inodeCount, line+6);
                updateInodes_list('f', &inodeCount, inodes);
            }else{
                printf("File already exists: %s\n", line+6);
            }

        } else {
            printf("Unknown command or incorrect format.\n");
        }

    }
    return 0;
}
void writeDirectory(DirectoryStruct directoryInfo[],int *directorySize,int currDirectory, int *inodeCount, char *name){
    if (*directorySize >= MAX_INODES - 1) {
        fprintf(stderr, "Maximum directory size reached.\n");
        return;
    }
    DirectoryStruct newDirectory = {.inode = *inodeCount};
    strncpy(newDirectory.name, name, MAX_NAME_LEN);
    newDirectory.name[MAX_NAME_LEN] = '\0';

    directoryInfo[*directorySize] = newDirectory;
    (*directorySize)++;

    DirectoryStruct newDirectoryinfo[2];
    DirectoryStruct addedDir = {.inode = *inodeCount}; 
    strncpy(addedDir.name, ".", MAX_NAME_LEN);
    DirectoryStruct parentDir = {.inode = currDirectory};
    strncpy(parentDir.name, "..", MAX_NAME_LEN);
    newDirectoryinfo[0] = addedDir;
    newDirectoryinfo[1] = parentDir;
    char currFileOpen[5];

    //Update current direcotryFile
    sprintf(currFileOpen,"%d",currDirectory);
    FILE *fp = fopen(currFileOpen, "wb");
    if (fp == NULL) {
        perror("ERR: FILE OPEN FAILED");
        exit(1);
    }
    for (int i = 0; i < *directorySize; i++) {
        if (fwrite(&directoryInfo[i].inode, sizeof(directoryInfo[i].inode), 1, fp) != 1) {
            perror("Failed to write inode to file");
            fclose(fp);
            exit(1);
        }
        if (fwrite(directoryInfo[i].name, sizeof(char), MAX_NAME_LEN, fp) != MAX_NAME_LEN) {
            perror("Failed to write name to file");
            fclose(fp);
            exit(1);
        }
    }
    fclose(fp);
    //Create new File with "." & ".." initialized
    sprintf(currFileOpen,"%d",*inodeCount);
    fp = fopen(currFileOpen, "wb");
    if (fp == NULL) {
        perror("ERR: FILE OPEN FAILED");
        exit(1);
    }
    for (int i = 0; i < 2; i++) {
        if (fwrite(&newDirectoryinfo[i].inode, sizeof(newDirectoryinfo[i].inode), 1, fp) != 1) {
            perror("Failed to write inode to file");
            fclose(fp);
            exit(1);
        }
        if (fwrite(directoryInfo[i].name, sizeof(char), MAX_NAME_LEN, fp) != MAX_NAME_LEN) {
            perror("Failed to write name to file");
            fclose(fp);
            exit(1);
        }
    }
    
    fclose(fp);
}


void writeFile(DirectoryStruct directoryInfo[],int *directorySize,int currDirectory, int *inodeCount, char *name){
    if (*directorySize >= MAX_INODES - 1) {
        fprintf(stderr, "Maximum directory size reached.\n");
        return;
    }
    DirectoryStruct newDirectory = {.inode = *inodeCount};
    strncpy(newDirectory.name, name, MAX_NAME_LEN);
    newDirectory.name[MAX_NAME_LEN] = '\0';

    directoryInfo[*directorySize] = newDirectory;
    (*directorySize)++;

    char currFileOpen[5];

    sprintf(currFileOpen,"%d",currDirectory);
    FILE *fp = fopen(currFileOpen, "wb");
    if (fp == NULL) {
        perror("ERR: FILE OPEN FAILED");
        exit(1);
    }
    for (int i = 0; i < *directorySize; i++) {
        if (fwrite(&directoryInfo[i].inode, sizeof(directoryInfo[i].inode), 1, fp) != 1) {
            perror("Failed to write inode to file");
            fclose(fp);
            exit(1);
        }
        if (fwrite(directoryInfo[i].name, sizeof(char), MAX_NAME_LEN, fp) != MAX_NAME_LEN) {
            perror("Failed to write name to file");
            fclose(fp);
            exit(1);
        }
    }
    fclose(fp);
    sprintf(currFileOpen,"%d",*inodeCount);
    fp = fopen(currFileOpen, "w");
    if (fp == NULL) {
        perror("ERR: FILE OPEN FAILED");
        exit(1);
    }
    size_t written = fwrite(name,sizeof(char),strlen(name),fp);
    if(written != strlen(name)){
        perror("Failed to write the string to file");
        fclose(fp);
        exit(EXIT_FAILURE);
    }
    fclose(fp);

}

void updateInodes_list(char type, int *inodeCount, InodeStruct inodes[]) {
    InodeStruct newInode = {*inodeCount, type};
    FILE *fp = fopen("inodes_list", "wb");
    if (fp == NULL) {
        perror("ERR: FILE OPEN FAILED");
        exit(1);
    }

    // Check if there is space for a new inode
    if (*inodeCount < MAX_INODES) {
        // Append newInode to the inodes array
        inodes[*inodeCount] = newInode;
        (*inodeCount)++;
    } else {
        fprintf(stderr, "Maximum number of inodes reached.\n");
        fclose(fp);
        exit(1);
    }

    // Write the inodes array to the file
    if (fwrite(inodes, sizeof(InodeStruct), *inodeCount, fp) != *inodeCount) {
        perror("Failed to write inodes to file");
        fclose(fp);
        exit(1);
    }

    fclose(fp);

}
void readDirectory(int dir, DirectoryStruct directoryInfo[], int *finalSize){ 
    char name[5];
    sprintf(name, "%d", dir);
    FILE *fp = fopen(name, "rb");
    if (fp == NULL){
        fprintf(stderr, "ERR: FILE OPEN FAILD\n");
        exit(1);
    }
    DirectoryStruct temp;
    int idx = 0;
    while(fread(&temp, sizeof(temp.inode), 1, fp) == 1){
        if(fread(temp.name, sizeof(char)*32, 1, fp) == 1){
            directoryInfo[idx] = temp;
        }
        idx++;
    }
    *finalSize = idx;
    fclose(fp);
}
void printDirecotry(DirectoryStruct directoryInfo[], int size){
    for(int i = 0; i< size; i++){
        printf("%u %s\n", directoryInfo[i].inode, directoryInfo[i].name);
    }
}

int findDirectory(char *name,InodeStruct inodes[], DirectoryStruct directoryInfo[], int size) {
    for (int i = 0; i < size; i++) {
        //fprintf(stdout, "%d\n",inodes[directoryInfo[i].inode].type == 'd' && strcmp(directoryInfo[i].name, name) == 0);
        if (inodes[directoryInfo[i].inode].type == 'd' && strcmp(directoryInfo[i].name, name) == 0) {
            return directoryInfo[i].inode;
        }
    }
    return -1;
}

int findFile(char *name,InodeStruct inodes[], DirectoryStruct directoryInfo[], int size) {
    for (int i = 0; i < size; i++) {
        if (inodes[directoryInfo[i].inode].type == 'f' && strcmp(directoryInfo[i].name, name) == 0) {
            return directoryInfo[i].inode;
        }
    }
    return -1;
}



