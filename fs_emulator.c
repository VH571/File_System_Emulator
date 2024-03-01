#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdint.h>
#define MAX_NODES 1024
typedef struct{
    uint32_t inode;
    char type;
}InodeStruct;


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
    InodeStruct inodes[MAX_NODES];
    size_t inodeCount = 0;
    FILE *fp = fopen("inodes_list", "rb");
    if (fp == NULL){
        fprintf(stderr, "ERR: FILE OPEN FAILD\n");
        exit(1);
    }
    InodeStruct temp;
    while(fread(&temp, sizeof(InodeStruct), 1, fp) == 1){
        if((temp.inode >= 0 && temp.inode <= MAX_NODES) && (temp.type == 'd' || temp.type == 'f')){
            fprintf(stdout, "Sucessful inode entry: { inode: %u, type: %c }\n", temp.inode, temp.type);
            inodes[inodeCount++] = temp;
        }
        else{
        }
        if (inodeCount >= MAX_NODES) {
            break;
        }
    }
    fclose(fp);




    return 0;
}
