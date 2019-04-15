#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <openssl/sha.h>
#include "filesys.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

struct merkleNode{
	char hash[20];
	struct merkleNode *leftChild;
	struct merkleNode *rightChild;
};

//static struct merkleNode* root[100]; //assuming fd lies in [0,99]
//static char* fnames[100];
//static int filesys_inited = 0;

/* returns 20 bytes unique hash of the buffer (buf) of length (len)
 * in input array sha1.
 */
void get_sha1_hash (const void *buf, int len, const void *sha1)
{
	SHA1 ((unsigned char*)buf, len, (unsigned char*)sha1);
}

void printHash(char* has){
	for(int i=0; i<20; i++)
		printf("%d", has[i]);
	printf("\n");
}

struct merkleNode* createMerkleTree(int fd){
		//printf("Hi, in fn\n" );	
	char blk[64];
	memset (blk, 0, 64);
	//printf("trying to open: %s\n", fnames[fd]);
    printf("fd is %d\n", fd);
    //fd = open (fnames[fd], O_RDONLY, 0);
    //Handling empty file case, and case when read is returning -1
    lseek(fd, 0, SEEK_SET);
    int numCan = read (fd, blk, 64);
    // printf("Able to read %d bytes (max 64)\n", numCan);
    				printf("Hi, before Numcan\n" );
    // assert (numCan >=0 );
    if(numCan == 0){
				printf("inside numCan\n" );    	
        struct merkleNode* ret = (struct merkleNode*) malloc( sizeof(struct merkleNode) );
        				printf("YO\n" );
        memset (ret->hash, 0, 64);
        ret -> leftChild = NULL;
        ret -> rightChild = NULL;
        return ret;
    }
    lseek(fd, 0, SEEK_SET);

	// // printf("tryint to open: %s\n", fnames[fd]);
	// fd = open (fnames[fd], O_RDONLY, 0);
	// //Handling empty file case, and case when read is returning -1
	// lseek(fd, 0, SEEK_SET);
	// int numCan = read (fd, blk, 64);
	// // printf("Able to read %d bytes (max 64)\n", numCan);
	// assert (numCan >=0 );
	// if(numCan == 0){
	// 	struct merkleNode* ret = (struct merkleNode*) malloc( sizeof(struct merkleNode) );
	// 	memset (ret->hash, 0, 64);
	// 	ret -> leftChild = NULL;
	// 	ret -> rightChild = NULL;
	// 	return ret;
	// }
	// lseek(fd, 0, SEEK_SET);
	
	struct merkleNode* level[3000];
	// level = (struct merkleNode*)malloc(sizeof(struct merkleNode));
	int levelCount = 0;
				printf("Hi, YO\n" );	
	//Creating all the leaf nodes				
	while(read (fd, blk, 64) > 0){
				// printf("Hi, in wh1\n" );	
		printf("levelCount %d\n", levelCount);
		assert(levelCount<3000);
		// if(levelCount<5)
		// 	sprintf("blk no. %d's value is %s\n",levelCount,blk);

		level[levelCount] = (struct merkleNode*) malloc( sizeof(struct merkleNode) );
			sprintf("s\n", "i am here");
		level[levelCount] -> leftChild = NULL;
		level[levelCount] -> rightChild = NULL;
		get_sha1_hash(blk, 64, level[levelCount++]->hash);

		memset (blk, 0, 64);
	}
		printf("Hi, out of 1st while\n" );
	//close(fd);

	while(levelCount>1){
		int pCount;
		char blk[40];
		printf("Hi, in 1st while\n" );
		for(pCount = 0; pCount < levelCount/2; pCount++){
			for(int i=0; i<20; i++) blk[i] = level[pCount*2]->hash[i];
			for(int i=0; i<20; i++) blk[20+i] = level[pCount*2+1]->hash[i];

			struct merkleNode *node = (struct merkleNode*) malloc( sizeof(struct merkleNode) );
			get_sha1_hash(blk, 64, node->hash);
			node->leftChild = level[pCount*2];
			node->rightChild = level[pCount*2+1];
			
			level[pCount] = node;
		}
		printf("Hi, after 1st while\n" );
		// if there was a node left, pull it in the level up
		// TODO: Duplicate the last node and add a combined hash
		// this will ensure that we always have a complete BT
		if(levelCount%2){
			level[pCount] = (struct merkleNode*) malloc( sizeof(struct merkleNode) );
			for(int i=0; i<20; i++) level[pCount]->hash[i] = level[pCount*2]->hash[i];
			level[pCount*2]->leftChild = NULL;
			level[pCount*2]->rightChild = NULL;

			pCount++;
		}
		levelCount = pCount;
	}
	return level[0];
}

int main(int argc, char const *argv[])
{
	int count =0;
// struct merkleNode *root
	while(count<3)
	{	printf("c %d\n",count );
		int fd = open ("myfile.txt", O_CREAT|O_RDWR, S_IRUSR|S_IWUSR);
		struct merkleNode *root = createMerkleTree(fd);
		printf("c %d\n",count );		
		printHash(root->hash);
	}

	// int secureFD = open("secure.txt", O_RDWR, S_IRUSR|S_IWUSR);	
	return 0;
}
