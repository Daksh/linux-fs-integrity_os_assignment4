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
	printf("\n");
}

struct merkleNode* createMerkleTree(int fd){	
	char blk[65];
	memset (blk, '0', 64);
    //fd = open (fnames[fd], O_RDONLY, 0);
    //Handling empty file case, and case when read is returning -1
    lseek(fd, 0, SEEK_SET);
    int numCan = read (fd, blk, 64);
    assert (numCan >=0 );
    if(numCan == 0){    	
        struct merkleNode* ret = (struct merkleNode*) malloc( sizeof(struct merkleNode) );
        blk[64] = '\0';
		get_sha1_hash(blk, 64, ret->hash);        
        ret->hash[20] = '\0';
        ret -> leftChild = NULL;
        ret -> rightChild = NULL;
        return ret;
    }
    //read successful
	int endpointer = lseek(fd,0,SEEK_END);
    lseek(fd, 0, SEEK_SET);
	memset (blk, '0', 64);	
	struct merkleNode* level[3000];
	int levelCount = 0;

	while((read (fd, blk, 64)) > 0){
		blk[64] = '\0';
		assert(levelCount<3000);
		level[levelCount] = (struct merkleNode*) malloc( sizeof(struct merkleNode) );
		level[levelCount] -> leftChild = NULL;
		level[levelCount] -> rightChild = NULL;

		get_sha1_hash(blk, 64, level[levelCount++]->hash);

		level[levelCount-1]->hash[20] = '\0';
	
		int current = lseek(fd,0,SEEK_CUR);
		if(current != endpointer)
		{			
			memset (blk, '0', 64);
		}
	}
	printf("%s\n","Last block data" );
	printf("%s\n",blk);
	printf("%s\n","Block last Hash value" );
	printHash(level[levelCount-1]->hash);		
	//close(fd);

	while(levelCount>1){
		int pCount;
		char blk[41];
		for(pCount = 0; pCount < levelCount/2; pCount++){
			for(int i=0; i<20; i++) blk[i] = level[pCount*2]->hash[i];
			for(int i=0; i<20; i++) blk[20+i] = level[pCount*2+1]->hash[i];
			blk[40] = '\0';
			struct merkleNode *node = (struct merkleNode*) malloc( sizeof(struct merkleNode) );
			get_sha1_hash(blk, 40, node->hash);
			node->hash[20] = '\0';
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
	char pathname[] =  "file2.txt";
	while(count<3)
	{	printf("c %d\n",count );
		int fd = open (pathname, O_CREAT|O_RDONLY, S_IRUSR|S_IWUSR);
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