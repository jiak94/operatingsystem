#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "fs.h"
#include "fs_util.h"
#include "disk.h"

char inodeMap[MAX_INODE / 8];
char blockMap[MAX_BLOCK / 8];
Inode inode[MAX_INODE];
SuperBlock superBlock;
Dentry dentry[MAX_INODE];
Dentry curDir;
IndirectBlock indirectBlockArray[MAX_INODE];
int curDirBlock;

int fs_mount(char *name)
{
		int numInodeBlock =  (sizeof(Inode)*MAX_INODE)/ BLOCK_SIZE;
		int i, index, inode_index = 0;

		// load superblock, inodeMap, blockMap and inodes into the memory
		if(disk_mount(name) == 1) {
				disk_read(0, (char*) &superBlock);
				disk_read(1, inodeMap);
				disk_read(2, blockMap);
				disk_read(3, (char*) &dentry);
				disk_read(4, (char*) &indirectBlockArray);
				//printf("size of indirectArray: %lu\n", sizeof(indirectBlockArray));
				//printf("size of dentry: %lu\n", sizeof(dentry));
				for(i = 0; i < numInodeBlock; i++)
				{
						index = i+5;
						disk_read(index, (char*) (inode+inode_index));
						inode_index += (BLOCK_SIZE / sizeof(Inode));
				}
				// root directory
				curDirBlock = inode[0].directBlock[0];
				disk_read(curDirBlock, (char*)&curDir);

		} else {
		// Init file system superblock, inodeMap and blockMap
				superBlock.freeBlockCount = MAX_BLOCK - (1+1+1+1+1+numInodeBlock);
				superBlock.freeInodeCount = MAX_INODE;

				//Init inodeMap
				for(i = 0; i < MAX_INODE / 8; i++)
				{
						set_bit(inodeMap, i, 0);
				}
				//Init blockMap
				for(i = 0; i < MAX_BLOCK / 8; i++)
				{
						if(i < (1+1+1+1+1+numInodeBlock)) set_bit(blockMap, i, 1);
						else set_bit(blockMap, i, 0);
				}
				//Init root dir
				int rootInode = get_free_inode();
				curDirBlock = get_free_block();

				inode[rootInode].type =directory;
				inode[rootInode].owner = 0;
				inode[rootInode].group = 0;
				gettimeofday(&(inode[rootInode].created), NULL);
				gettimeofday(&(inode[rootInode].lastAccess), NULL);
				inode[rootInode].size = 1;
				inode[rootInode].blockCount = 1;
				inode[rootInode].directBlock[0] = curDirBlock;

				curDir.numEntry = 1;
				strncpy(curDir.dentry[0].name, ".", 1);
				curDir.dentry[0].name[1] = '\0';
				curDir.dentry[0].inode = rootInode;
				disk_write(curDirBlock, (char*)&curDir);

				dentry[rootInode] = curDir;
				//printf("size of indirectArray: %lu\n", sizeof(indirectBlockArray));
				//printf("size of denry: %lu\n", sizeof(dentry));
		}

		//printf("size of Inode: %lu\n", sizeof(Inode));
		return 0;
}

int fs_umount(char *name)
{
		int numInodeBlock =  (sizeof(Inode)*MAX_INODE )/ BLOCK_SIZE;
		int i, index, inode_index = 0;
		disk_write(0, (char*) &superBlock);
		disk_write(1, inodeMap);
		disk_write(2, blockMap);
		disk_write(3, (char*) &dentry);
		disk_write(4, (char*) &indirectBlockArray);
		for(i = 0; i < numInodeBlock; i++)
		{
				index = i+5;
				disk_write(index, (char*) (inode+inode_index));
				inode_index += (BLOCK_SIZE / sizeof(Inode));
		}
		// current directory
		disk_write(curDirBlock, (char*)&curDir);

		disk_umount(name);
}

int search_cur_dir(char *name)
{
		// return inode. If not exist, return -1
		int i;

		for(i = 0; i < curDir.numEntry; i++)
		{
				if(command(name, curDir.dentry[i].name)) return curDir.dentry[i].inode;
		}
		return -1;
}

int create_large_file(char *name, int size) {
	if (size > 70656)
	{
		printf("File too large!\n");
	}
	int i;
	int inodeNum = search_cur_dir(name);
	if (inodeNum >= 0) {
		printf("File create failed:  %s exist.\n", name);
		return -1;
	}

	if(curDir.numEntry + 1 >= (BLOCK_SIZE / sizeof(DirectoryEntry))) {
		printf("File create failed: directory is full!\n");
		return -1;
	}

	int numBlock = size / BLOCK_SIZE;
	if(size % BLOCK_SIZE > 0) numBlock++;
	int indirect = numBlock - 10;

	if(numBlock > superBlock.freeBlockCount) {
		printf("File create failed: not enough space\n");
		return -1;
	}

	if(superBlock.freeInodeCount < 1) {
		printf("File create failed: not enough inode\n");
		return -1;
	}

	char *tmp = (char*) malloc(sizeof(int) * size+1);
	rand_string(tmp, size);
	printf("rand_string = %s\n", tmp);

	inodeNum = get_free_inode();
	if(inodeNum < 0) {
		printf("File_create error: not enough inode.\n");
		return -1;
	}

	inode[inodeNum].type = file;
	inode[inodeNum].owner = 1;
	inode[inodeNum].group = 2;
	gettimeofday(&(inode[inodeNum].created), NULL);
	gettimeofday(&(inode[inodeNum].lastAccess), NULL);
	inode[inodeNum].size = size;
	inode[inodeNum].blockCount = numBlock;
	inode[inodeNum].indirectBlock = indirect;

	strncpy(curDir.dentry[curDir.numEntry].name, name, strlen(name));
	curDir.dentry[curDir.numEntry].name[strlen(name)] = '\0';
	curDir.dentry[curDir.numEntry].inode = inodeNum;
	printf("curdir %s, name %s\n", curDir.dentry[curDir.numEntry].name, name);
	curDir.numEntry++;

	indirectBlockArray[inodeNum].inode = inodeNum;

	for(i = 0; i < 10; i++)
	{
		int block = get_free_block();
		if(block == -1) {
			printf("File_create error: get_free_block failed\n");
			return -1;
		}
		inode[inodeNum].directBlock[i] = block;
		disk_write(block, tmp+(i*BLOCK_SIZE));
	}

	for (; i < numBlock; i++)
	{
		int block = get_free_block();
		if (block == -1)
		{
			printf("File_create error: get_free_block failed\n");
			return -1;
		}
		indirectBlockArray[inodeNum].indirectBlock[i-10] = block;
		disk_write(block, tmp+(i*BLOCK_SIZE));
	}
	printf("file created: %s, inode %d, size %d\n", name, inodeNum, size);

	free(tmp);

	dentry[curDir.dentry[0].inode] = curDir;
	return 0;
}

int file_create(char *name, int size)
{
		int i;

		if(size >= SMALL_FILE) {
			//printf("Do not support files larger than %d bytes yet.\n", SMALL_FILE);
			//return -1;
			return create_large_file(name, size);
		}

		int inodeNum = search_cur_dir(name);
		if(inodeNum >= 0) {
				printf("File create failed:  %s exist.\n", name);
				return -1;
		}

		if(curDir.numEntry + 1 >= (BLOCK_SIZE / sizeof(DirectoryEntry))) {
				printf("File create failed: directory is full!\n");
				return -1;
		}

		int numBlock = size / BLOCK_SIZE;
		if(size % BLOCK_SIZE > 0) numBlock++;

		if(numBlock > superBlock.freeBlockCount) {
				printf("File create failed: not enough space\n");
				return -1;
		}

		if(superBlock.freeInodeCount < 1) {
				printf("File create failed: not enough inode\n");
				return -1;
		}

		char *tmp = (char*) malloc(sizeof(int) * size+1);

		rand_string(tmp, size);
		printf("rand_string = %s\n", tmp);

		// get inode and fill it
		inodeNum = get_free_inode();
		if(inodeNum < 0) {
				printf("File_create error: not enough inode.\n");
				return -1;
		}

		inode[inodeNum].type = file;
		inode[inodeNum].owner = 1;
		inode[inodeNum].group = 2;
		gettimeofday(&(inode[inodeNum].created), NULL);
		gettimeofday(&(inode[inodeNum].lastAccess), NULL);
		inode[inodeNum].size = size;
		inode[inodeNum].blockCount = numBlock;

		// add a new file into the current directory entry
		strncpy(curDir.dentry[curDir.numEntry].name, name, strlen(name));
		curDir.dentry[curDir.numEntry].name[strlen(name)] = '\0';
		curDir.dentry[curDir.numEntry].inode = inodeNum;
		printf("curdir %s, name %s\n", curDir.dentry[curDir.numEntry].name, name);
		curDir.numEntry++;

		// get data blocks
		for(i = 0; i < numBlock; i++)
		{
				int block = get_free_block();
				if(block == -1) {
						printf("File_create error: get_free_block failed\n");
						return -1;
				}
				inode[inodeNum].directBlock[i] = block;
				disk_write(block, tmp+(i*BLOCK_SIZE));
		}

		printf("file created: %s, inode %d, size %d\n", name, inodeNum, size);

		free(tmp);

		dentry[curDir.dentry[0].inode] = curDir;
		return 0;
}

int file_cat(char *name)
{
		//printf("Not implemented yet.\n");
		int i;
		int inodeNum = search_cur_dir(name);
		if (inodeNum < 0) {
			printf("Error: No such file in the directory\n");
			return -1;
		}

		int numBlock = inode[inodeNum].blockCount;
		int indirectBlock = inode[inodeNum].indirectBlock;


		char *string;
		string = malloc(102400);
		if (numBlock > 10)
		{
			for(i = 0; i < numBlock; i++) {
				disk_read(inode[inodeNum].directBlock[i], string);
				printf("%s\n", string);
			}

			for (int i = 0; i < indirectBlock; i++)
			{
				disk_read(indirectBlockArray[inodeNum].indirectBlock[i], string);
				printf("%s\n", string);
			}
		}
		else {
			for(i = 0; i < numBlock; i++) {
				disk_read(inode[inodeNum].directBlock[i], string);
				printf("%s\n", string);
			}
		}
		
		gettimeofday(&(inode[inodeNum].lastAccess), NULL);
		free(string);
		//printf("\n");
		return 1;
}

int file_read(char *name, int offset, int size)
{
		//printf("Not implemented yet.\n");
		int i;
		int inodeNum = search_cur_dir(name);
		if (inodeNum < 0) {
			printf("Error: No such file in the directory\n");
			return -1;
		}

		int numBlock = inode[inodeNum].size / BLOCK_SIZE;

		if (inode[inodeNum].size % BLOCK_SIZE > 0)
		{
			numBlock = numBlock + 1;
		}
		//printf("numBlock = %d\n", numBlock);

		char *string;
		char *temp;
		string = malloc(102400);
		temp = malloc(102400);
		for(i = 0; i < numBlock; i++) {
			if (i < 10)
			{
				disk_read(inode[inodeNum].directBlock[i], temp);
			}
			else {
				disk_read(indirectBlockArray[inodeNum].indirectBlock[i - 10], temp);
			}
			
			//printf("%s\n", string);
			strcat(string, temp);
		}
		int length = strlen(string);
		if (size + offset > length)
		{
			size = length - offset;
		}
		char *substring;
		substring = malloc(102400);
		printf("%s\n", strncpy(substring, string+offset, size));
		gettimeofday(&(inode[inodeNum].lastAccess), NULL);

		free(string);
		free(temp);
		free(substring);
		//printf("\n");
		return 1;
}

int file_write(char *name, int offset, int size, char *buf)
{
		//printf("Not implemented yet.\n");
		char *writeString;
		writeString = malloc(102400);
		if (size < strlen(buf))
		{
			strncpy(writeString, buf, size);
		}
		else {
			strncpy(writeString, buf, strlen(buf));
		}

		int i;
		int inodeNum = search_cur_dir(name);
		if (inodeNum < 0) {
			printf("Error: No such file in the directory\n");
			return -1;
		}

		int numBlock = (inode[inodeNum].size + size) / BLOCK_SIZE;

		if ((inode[inodeNum].size + size) % BLOCK_SIZE > 0)
		{
			numBlock = numBlock + 1;
		}

		char *string;
		char *temp;
		temp = malloc(102400);
		string = malloc(102400);
		for(i = 0; i < numBlock; i++) {
			if (i < 10)
			{
				disk_read(inode[inodeNum].directBlock[i], temp);
			}
			else  {
				disk_read(indirectBlockArray[inodeNum].indirectBlock[i - 10], temp);
			}
			strcat(string, temp);
		}
		printf("%lu\n", strlen(string));

		char *newString, *secondPart;
		newString = malloc(102400);
		secondPart = malloc(102400);

		//first part of original
		strncpy(newString, string, offset);

		//string to write
		strcat(newString, writeString);

		//get second part
		strncpy(secondPart, string + offset, strlen(string) - offset);

		//paste to the new string
		strcat(newString, secondPart);

		printf("%lu\n", strlen(newString));

		for (int i = 0; i < numBlock; i++)
		{
			if (i < 10)
			{
				disk_write(inode[inodeNum].directBlock[i], newString+(i*BLOCK_SIZE));
			}
			else {
				disk_write(indirectBlockArray[inodeNum].indirectBlock[i - 10], newString+(i*BLOCK_SIZE));
			}
			
		}
		gettimeofday(&(inode[inodeNum].lastAccess), NULL);

		free(string);
		free(temp);
		free(newString);
		free(writeString);
		free(secondPart);
		//printf("\n");
		return 1;
}

int file_stat(char *name)
{
		char timebuf[28];
		int inodeNum = search_cur_dir(name);
		if(inodeNum < 0) {
				printf("file cat error: file is not exist.\n");
				return -1;
		}

		printf("Inode = %d\n", inodeNum);
		if(inode[inodeNum].type == file) printf("type = file\n");
		else printf("type = directory\n");
		printf("owner = %d\n", inode[inodeNum].owner);
		printf("group = %d\n", inode[inodeNum].group);
		printf("size = %d\n", inode[inodeNum].size);
		printf("num of block = %d\n", inode[inodeNum].blockCount);
		format_timeval(&(inode[inodeNum].created), timebuf, 28);
		printf("Created time = %s\n", timebuf);
		format_timeval(&(inode[inodeNum].lastAccess), timebuf, 28);
		printf("Last accessed time = %s\n", timebuf);
}

int file_remove(char *name)
{
		//printf("Not implemented yet.\n");
		int inodeNum = search_cur_dir(name);
		if (inodeNum < 0)
		{
			printf("Error: no such file\n");
			return -1;
		}

        if (curDir.numEntry == 0) {
            printf("Error: directory is empty");
            return -1;
        }
        int numBlock = inode[inodeNum].size / BLOCK_SIZE;

        superBlock.freeBlockCount += numBlock;
        superBlock.freeInodeCount++;

        //inode[inodeNum] = {0};
        //curDir.dentry[curDir.numEntry] = {0};
        inode[inodeNum].owner = 0;
        inode[inodeNum].group = 0;
        gettimeofday(&(inode[inodeNum].created), NULL);
        gettimeofday(&(inode[inodeNum].lastAccess), NULL);

        inode[inodeNum].size = 0;
        inode[inodeNum].blockCount = 0;


        memset(&curDir.dentry[curDir.numEntry].name[0], 0,sizeof(curDir.dentry[curDir.numEntry].name));
        curDir.dentry[curDir.numEntry].inode = -1;

        inodeMap[inodeNum] = 0;


        curDir.numEntry--;

        dentry[curDir.dentry[0].inode] = curDir;
        return 0;
}

int dir_make(char* name)
{
		//printf("Not implemented yet.\n");
        int inodeNum = search_cur_dir(name);

        if (inodeNum >= 0) {
             printf("Already Exists\n");
             return -1;
        }

        inodeNum = get_free_inode();
        if (inodeNum < 0) {
            printf("No enough inode. \n");
            return -1;
        }

        inode[inodeNum].type = directory;
        inode[inodeNum].owner = 1;
        inode[inodeNum].group = 2;
        inode[inodeNum].size = 1;
        gettimeofday(&(inode[inodeNum].created), NULL);
        gettimeofday(&(inode[inodeNum].lastAccess), NULL);

        dentry[inodeNum].numEntry = 2;
        strncpy(dentry[inodeNum].dentry[0].name, ".", strlen("."));
        dentry[inodeNum].dentry[0].inode = inodeNum;

        strncpy(dentry[inodeNum].dentry[1].name, "..", strlen(".."));
        dentry[inodeNum].dentry[1].inode = curDir.dentry[0].inode;

        strncpy(curDir.dentry[curDir.numEntry].name, name, strlen(name));
        curDir.dentry[curDir.numEntry].name[strlen(name)] = '\0';
        curDir.dentry[curDir.numEntry].inode = inodeNum;
        printf("curdir %s, name %s\n", curDir.dentry[curDir.numEntry].name, name);
        
        curDir.numEntry++;

        dentry[curDir.dentry[0].inode] = curDir;
        return 0;
}

int dir_remove(char *name)
{
	int inodeNum = search_cur_dir(name);
    if (inodeNum < 0) {
       	printf("Error: no such directory\n");
        return -1;
    }

    if (curDir.numEntry == 0) {
        printf("Error: current directory is empty\n");
        return -1;
    }

    if (dentry[inodeNum].numEntry > 2)
    {
    	printf("Error: the directory is not empty\n");
    	return -1;
    }

    superBlock.freeInodeCount++;

    inode[inodeNum].owner = 0;
    inode[inodeNum].group = 0;
    gettimeofday(&(inode[inodeNum].created), NULL);
    gettimeofday(&(inode[inodeNum].lastAccess), NULL);

    inode[inodeNum].size = 0;
    inode[inodeNum].blockCount = 0;


    memset(&curDir.dentry[curDir.numEntry].name[0], 0,sizeof(curDir.dentry[curDir.numEntry].name));
    curDir.dentry[curDir.numEntry].inode = -1;

    inodeMap[inodeNum] = 0;


    curDir.numEntry--;
    dentry[curDir.dentry[0].inode] = curDir;
}

int dir_change(char* name)
{
		//printf("Not implemented yet.\n");
		int inodeNum = search_cur_dir(name);
		if (name < 0)
		{
			printf("Error: no Such directory\n");
			return -1;
		}
		if (inode[inodeNum].type != directory)
		{
			printf("Error: no such directory\n");
			return -1;
		}
		gettimeofday(&(inode[inodeNum].lastAccess), NULL);
		//printf("InodeNum: %d\n", inodeNum);
		//printf("%d\n", dentry[inodeNum].numEntry);
		curDir = dentry[inodeNum];
}

int ls()
{
		int i;
		for(i = 0; i < curDir.numEntry; i++)
		{
				int n = curDir.dentry[i].inode;
				if(inode[n].type == file) printf("type: file, ");
				else printf("type: dir, ");
				printf("name \"%s\", inode %d, size %d byte\n", curDir.dentry[i].name, curDir.dentry[i].inode, inode[n].size);
		}

		return 0;
}

int fs_stat()
{
		printf("File System Status: \n");
		printf("# of free blocks: %d (%d bytes), # of free inodes: %d\n", superBlock.freeBlockCount, superBlock.freeBlockCount*512, superBlock.freeInodeCount);
}

int execute_command(char *comm, char *arg1, char *arg2, char *arg3, char *arg4, int numArg)
{
		if(command(comm, "create")) {
				if(numArg < 2) {
						printf("error: create <filename> <size>\n");
						return -1;
				}
				return file_create(arg1, atoi(arg2)); // (filename, size)
		} else if(command(comm, "cat")) {
				if(numArg < 1) {
						printf("error: cat <filename>\n");
						return -1;
				}
				return file_cat(arg1); // file_cat(filename)
		} else if(command(comm, "write")) {
				if(numArg < 4) {
						printf("error: write <filename> <offset> <size> <buf>\n");
						return -1;
				}
				return file_write(arg1, atoi(arg2), atoi(arg3), arg4); // file_write(filename, offset, size, buf);
		}	else if(command(comm, "read")) {
				if(numArg < 3) {
						printf("error: read <filename> <offset> <size>\n");
						return -1;
				}
				return file_read(arg1, atoi(arg2), atoi(arg3)); // file_read(filename, offset, size);
		} else if(command(comm, "rm")) {
				if(numArg < 1) {
						printf("error: rm <filename>\n");
						return -1;
				}
				return file_remove(arg1); //(filename)
		} else if(command(comm, "mkdir")) {
				if(numArg < 1) {
						printf("error: mkdir <dirname>\n");
						return -1;
				}
				return dir_make(arg1); // (dirname)
		} else if(command(comm, "rmdir")) {
				if(numArg < 1) {
						printf("error: rmdir <dirname>\n");
						return -1;
				}
				return dir_remove(arg1); // (dirname)
		} else if(command(comm, "cd")) {
				if(numArg < 1) {
						printf("error: cd <dirname>\n");
						return -1;
				}
				return dir_change(arg1); // (dirname)
		} else if(command(comm, "ls"))  {
				return ls();
		} else if(command(comm, "stat")) {
				if(numArg < 1) {
						printf("error: stat <filename>\n");
						return -1;
				}
				return file_stat(arg1); //(filename)
		} else if(command(comm, "df")) {
				return fs_stat();
		} else {
				fprintf(stderr, "%s: command not found.\n", comm);
				return -1;
		}
		return 0;
}

