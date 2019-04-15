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
	char hash[21];//last byte for '\0'
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
		// printf("%c", has[i]);	
	printf("\n");
}

struct merkleNode* createMerkleTree(int fd){
		//printf("Hi, in fn\n" );	
	char blk[65];
	memset (blk, 0, 64);
	//printf("trying to open: %s\n", fnames[fd]);
    printf("fd is %d\n", fd);
    //fd = open (fnames[fd], O_RDONLY, 0);
    //Handling empty file case, and case when read is returning -1
    lseek(fd, 0, SEEK_SET);
    int numCan = read (fd, blk, 64);
    // printf("Able to read %d bytes (max 64)\n", numCan);
    // printf("Hi, before Numcan\n" );
    // assert (numCan >=0 );
    if(numCan == 0){
		// printf("inside numCan\n" );    	
        struct merkleNode* ret = (struct merkleNode*) malloc( sizeof(struct merkleNode) );
		// printf("YO\n" );
        memset (ret->hash, 0, 64);
        ret -> leftChild = NULL;
        ret -> rightChild = NULL;
        return ret;
    }
    //read successful

    lseek(fd, 0, SEEK_SET);
	
	struct merkleNode* level[3000];
	int levelCount = 0;
	// printf("Hi, YO\n" );	
	//Creating all the leaf nodes
	memset (blk, '0', 64);
	// blk[64]='\0';
	// printf("Blk after memset %s\n",blk );
	// printf("%s\n", "Printing done");
	// int rval;
	// while((rval =read (fd, blk, 64)) > 0){
	// 	// printf("Hi, in wh1\n" );	
	// 	// printf("levelCount = %d\n", levelCount);
	// 	while(rval != 64){
	// 		//printf("rvaal: %i",rval);
	// 		blk[rval] = '0';
	// 		rval++;
	// 	}

		// int rval;
		while((read (fd, blk, 64)) > 0){
		// printf("Hi, in wh1\n" );	
		// printf("levelCount = %d\n", levelCount);
		// while(rval != 64){
		// 	//printf("rvaal: %i",rval);
		// 	blk[rval] = '0';
		// 	rval++;
		// }

		blk[64] = '\0';
		assert(levelCount<3000);
		// if(levelCount<5)
		// 	sprintf("blk no. %d's value is %s\n",levelCount,blk);
		// printf("%s\n","before" );
		level[levelCount] = (struct merkleNode*) malloc( sizeof(struct merkleNode) );
		// printf("%s\n","after" );
		level[levelCount] -> leftChild = NULL;
		level[levelCount] -> rightChild = NULL;
		get_sha1_hash(blk, 64, level[levelCount++]->hash);
		level[levelCount-1]->hash[20] = '\0';
		if(levelCount==1)
		{
			printf("%s\n",blk);
			// printf("%s\n",level[levelCount-1]->hash);
			printf("%s\n","Block1 1 Hash value" );
			printHash(level[levelCount-1]->hash);
		}

		// Check this
		// printf("Terminating hash\n" );

		// printf("Done\n");
		//maybe correct
		memset (blk, 0, 64);
	}

		printf("%s\n",blk);
		// printf("%s\n",level[levelCount-1]->hash);
		printf("%s\n","Block last Hash value" );
		printHash(level[levelCount-1]->hash);

		// printf("Hi, out of 1st while\n" );		
	//close(fd);

	while(levelCount>1){
		int pCount;
		char blk[41];
		// printf("Hi, in 1st while\n" );
		for(pCount = 0; pCount < levelCount/2; pCount++){
			for(int i=0; i<20; i++) blk[i] = level[pCount*2]->hash[i];
			for(int i=0; i<20; i++) blk[20+i] = level[pCount*2+1]->hash[i];
			blk[40] = '\0';
			struct merkleNode *node = (struct merkleNode*) malloc( sizeof(struct merkleNode) );
			get_sha1_hash(blk, 64, node->hash);
			node->hash[20] = '\0';
			node->leftChild = level[pCount*2];
			node->rightChild = level[pCount*2+1];
			level[pCount] = node;
		}
		// printf("Hi, after 1st while\n" );
		// if there was a node left, pull it in the level up
		// TODO: Duplicate the last node and add a combined hash
		// this will ensure that we always have a complete BT
		if(levelCount%2){
			level[pCount] = (struct merkleNode*) malloc( sizeof(struct merkleNode) );
			for(int i=0; i<20; i++) level[pCount]->hash[i] = level[pCount*2]->hash[i];
			level[pCount]->hash[20] = '\0';				
			level[pCount*2]->leftChild = NULL;
			level[pCount*2]->rightChild = NULL;
			pCount++;
		}
		levelCount = pCount;
	}
	return level[0];
}

void destroyTree(struct merkleNode* x){
	if(x == NULL)
		return;
	destroyTree(x->leftChild);
	destroyTree(x->rightChild);
	free(x);
}

int main(int argc, char const *argv[])
{
	int count =0;
// struct merkleNode *root
	while(count<3)
	{	printf("c %d\n",count );
		int fd = open ("file2.txt", O_CREAT|O_RDONLY, S_IRUSR|S_IWUSR);
		struct merkleNode *root = createMerkleTree(fd);
		printf("File number %d hash value \n", count+1);
		printHash(root->hash);
		destroyTree(root);
		count++;
		close(fd);
/*		printf("%s\n",root->hash);*/
	}

	// int secureFD = open("secure.txt", O_RDWR, S_IRUSR|S_IWUSR);	
	return 0;
}