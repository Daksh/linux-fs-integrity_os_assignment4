#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <openssl/sha.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

struct merkleNode{
	char hash[20];
	struct merkleNode *leftChild;
	struct merkleNode *rightChild;
};

static struct merkleNode* root[100]; //assuming fd lies in [0,99]
static char* fnames[100];
static int filesys_inited = 0;

void get_sha1_hash (char *buf, int len, char *sha1)
{
	assert(len>=20);
	for(int i=0; i<20; i++)
		sha1[i] = buf[i];
}

struct merkleNode* createMerkleTree(int fd){
	char blk[64];
	memset (blk, 0, 64);
	printf("In func%d\n", fd);
	//struct merkleNode* level[1024*1024];//4 MB
	struct merkleNode* level[5000];
	int levelCount = 0;

	//Creating all the leaf nodes
	while(read (fd, blk, 64) > 0){
		assert(levelCount<5000);

		level[levelCount] = (struct merkleNode*) malloc( sizeof(struct merkleNode) );
		get_sha1_hash(blk, 64, level[levelCount++]->hash);

		memset (blk, 0, 64);
	}
	printf("In func%d\n", fd);
	while(levelCount>1){
		int pCount;
		char blk[40];

		for(pCount = 0; pCount < levelCount/2; pCount++){
			for(int i=0; i<20; i++) blk[i] = level[pCount*2]->hash[i];
			for(int i=0; i<20; i++) blk[20+i] = level[pCount*2+1]->hash[i];

			struct merkleNode *node = (struct merkleNode*) malloc( sizeof(struct merkleNode) );
			get_sha1_hash(blk, 64, node->hash);
			node->leftChild = level[pCount*2];
			node->rightChild = level[pCount*2+1];
			
			level[pCount] = node;
		}

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

int main(){
	int fd = open ("merkleTest.txt", O_RDONLY, 0);
	printf("Got FD: %d\n", fd);
    root[fd]=createMerkleTree(fd);
    printf("%s\n", root[fd]->hash);

	return 0;
}