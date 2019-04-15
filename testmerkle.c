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
	printf("\n");
}

struct merkleNode* createMerkleTree(int fd){
// char* fName = fnames[fd];
// fd = open (fName, O_RDONLY, 0);

int end = lseek(fd, 0, SEEK_END);
char* buf = (char *)malloc(end);
assert(lseek(fd,0,SEEK_SET)==0);
struct merkleNode* ret = (struct merkleNode*) malloc( sizeof(struct merkleNode) );
get_sha1_hash(buf,end,ret->hash);
ret->leftChild = NULL;
ret->rightChild = NULL;

close(fd);
return ret;
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