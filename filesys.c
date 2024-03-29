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

//Maximum Number of Nodes in a level of merkle Tree
#define MNPL 3000

struct merkleNode{
	char hash[20];
	struct merkleNode *leftChild;
	struct merkleNode *rightChild;
};

static struct merkleNode* root[100]; //assuming fd lies in [0,99]
static int secureFSize[100];
static char* fnames[100];
static int filesys_inited = 0;

/* returns 20 bytes unique hash of the buffer (buf) of length (len)
 * in input array sha1.
 */
void get_sha1_hash (const void *buf, int len, const void *sha1)
{
	SHA1 ((unsigned char*)buf, len, (unsigned char*)sha1);
}

struct merkleNode* createMerkleTree(char* fName){
	int fd = open (fName, O_RDONLY, 0);
	assert(fd != -1);

	//Character Array to read blocks (we use blocks of 64 Bytes)
	char blk[64];
	memset (blk, 0, 64);

	//If the File is Empty, return hash 000....0
	int numCan = read (fd, blk, 64);
	assert ( numCan >= 0 );
	if(numCan == 0){ //Empty file case
		struct merkleNode* ret = (struct merkleNode*) malloc( sizeof(struct merkleNode) );
		memset (ret->hash, '0', 20);
		ret -> leftChild = NULL;
		ret -> rightChild = NULL;
		close(fd);
		return ret;
	}

	//Seek to the start of file
	assert(lseek(fd,0,SEEK_SET)==0);

	struct merkleNode* level[MNPL];
	int levelCount = 0; //Number of nodes in a particular level of MT

	//Create all the leaf nodes
	while(read (fd, blk, 64) > 0){
		assert(levelCount < MNPL);

		level[levelCount] = (struct merkleNode*) malloc( sizeof(struct merkleNode) );
		level[levelCount] -> leftChild = NULL;
		level[levelCount] -> rightChild = NULL;
		get_sha1_hash(blk, 64, level[levelCount]->hash);

		memset (blk, 0, 64);
		levelCount++;
	}
	close(fd);

	while(levelCount>1){
		int pCount;
		char blk[40];

		// If there are odd number of nodes, duplicate the last node
		if(levelCount%2){
			assert(levelCount < MNPL);

			level[levelCount] = (struct merkleNode*) malloc( sizeof(struct merkleNode) );
			level[levelCount] -> leftChild = NULL;
			level[levelCount] -> rightChild = NULL;
			for(int i=0; i<20; i++)
				level[levelCount]->hash[i] = level[levelCount-1]->hash[i];

			levelCount++;
		}

		for(pCount = 0; pCount < levelCount/2; pCount++){
			for(int i=0; i<20; i++) blk[i] = level[pCount*2]->hash[i];
			for(int i=0; i<20; i++) blk[20+i] = level[pCount*2+1]->hash[i];

			struct merkleNode *node = (struct merkleNode*) malloc( sizeof(struct merkleNode) );
			get_sha1_hash(blk, 40, node->hash);
			node->leftChild = level[pCount*2];
			node->rightChild = level[pCount*2+1];

			//Re-use the array level, to store parent node references
			level[pCount] = node;
		}

		levelCount = pCount;
	}
	return level[0];
}

//Compare the first 20 bytes of both h1 and h2, if match: return 1; else 0
int hashSame(char* h1, char* h2){
	for(int i=0; i<20; i++)
		if( h1[i]!=h2[i] )
			return 0;
	return 1;
}

void printHash(char* has){
	for(int i=0; i<20; i++)
		printf("%d", has[i]);
	printf("\n");
}

/*
Takes a fd (file descriptor)
gets the corresponding fileName from fnames array; fnames[fd]
and the new hash from root array; root[fd]->hash 

Opens secure.txt and rewrites the hash in corresponding fileName

returns 1 on success
0 on failure 
*/
int updateSecure(int fd){
	assert(access( "secure.txt", F_OK ) != -1);//secure.txt must exist

	char* fName = fnames[fd];
	char* updatedHash = root[fd]->hash;

	int secureFD = open("secure.txt", O_RDWR, S_IRUSR|S_IWUSR);
	assert(lseek(secureFD,0,SEEK_CUR)==0);
	int n;
	char secureBlock[52]; // 32 for fileName + 20 for Root Hash

	while((n = read(secureFD, secureBlock, sizeof(secureBlock))) > 0){
		assert(n == 52);
		
		char filename[32];
		char hash[20];
		for(int i = 0; i < 32; i++) filename[i] = secureBlock[i];
		for(int j = 0; j < 20; j++) hash[j] = secureBlock[j+32];

		if( !strcmp(filename,fName) ){
			lseek(secureFD, -20, SEEK_CUR);

			//Update the root hash in secure.txt
			write(secureFD, updatedHash, 20);
			close(secureFD);
			return 1;
		}
	}
	close(secureFD);
	return 0;
}

char* getSecureHash(int fd){
	assert(access( "secure.txt", F_OK ) != -1);//secure.txt must exist

	char* fName = fnames[fd];
	char* retHash = (char *)malloc(20);

	int secureFD = open("secure.txt", O_RDWR, S_IRUSR|S_IWUSR);
	assert(lseek(secureFD,0,SEEK_CUR)==0);
	int n;
	char secureBlock[52]; // 32 for fileName + 20 for Root Hash

	while((n = read(secureFD, secureBlock, sizeof(secureBlock))) > 0){
		assert(n == 52);
		
		char filename[32];
		for(int i = 0; i < 32; i++) filename[i] = secureBlock[i];
		for(int j = 0; j < 20; j++) retHash[j] = secureBlock[j+32];

		if( !strcmp(filename,fName) ){
			close(secureFD);
			return retHash;
		}
	}
	close(secureFD);
	return NULL;
}

int appendSecure(int fd){
	assert(access( "secure.txt", F_OK ) != -1);//secure.txt must exist

	char* fName = fnames[fd];
	char* addHash = root[fd]->hash;

	int secureFD = open("secure.txt", O_RDWR, S_IRUSR|S_IWUSR);
	assert(lseek(secureFD,0,SEEK_CUR)==0);
	lseek(secureFD, 0, SEEK_END);
	assert(write(secureFD, fName, 32)==32);
	assert(write(secureFD, addHash, 20)==20);
	close(secureFD);
	return 0;
}

void s_updateSize(int fd){
	int currentPosition = lseek(fd, 0, SEEK_CUR);

	secureFSize[fd] = lseek(fd, 0, SEEK_END);

	lseek(fd, currentPosition, SEEK_SET);
}

/* Build an in-memory Merkle tree for the file.
 * Compare the integrity of file with respect to
 * root hash stored in secure.txt. If the file
 * doesn't exist, create an entry in secure.txt.
 * If an existing file is going to be truncated
 * update the hash in secure.txt.
 * returns -1 on failing the integrity check.
 */
int s_open (const char *pathname, int flags, mode_t mode)
{
	assert (filesys_inited);

	int fd = open(pathname, flags, mode);
	fnames[fd] = (char *)malloc(32);
	snprintf(fnames[fd], 32, "%s", pathname);
	// printf("s_open adding fnames[%d]: %s\n", fd, fnames[fd]);

	struct merkleNode* merkleRoot = createMerkleTree(fnames[fd]);
	char* secHash = getSecureHash(fd);

	root[fd] = merkleRoot;

	if(secHash == NULL){
		appendSecure(fd);
	} else{
		if(flags & O_TRUNC)
			updateSecure(fd);
		if(!hashSame(secHash,merkleRoot->hash)){
			close(fd);
			for(int x=0; x<20; x++) root[fd]->hash[x] = secHash[x];
			return -1;
		}
	}

	s_updateSize(fd);
	return fd;
}

/* SEEK_END should always return the file size 
 * updated through the secure file system APIs.

 * from unistd.h; whence values for lseek(2)
	#define	SEEK_SET	0	set file offset to offset
	#define	SEEK_CUR	1	set file offset to current plus offset
	#define	SEEK_END	2	set file offset to EOF plus offset
 */
int s_lseek (int fd, long offset, int whence)
{
	assert(fd<100);
	assert (filesys_inited);

	if(whence==SEEK_END)
		return secureFSize[fd];
	return lseek (fd, offset, whence);
}

/* read the blocks that needs to be updated
 * check the integrity of the blocks
 * modify the blocks
 * update the in-memory Merkle tree and root in secure.txt
 * returns -1 on failing the integrity check.
 * Finally, write modified blocks of the file
 */
ssize_t s_write (int fd, const void *buf, size_t count)
{
	assert (fd<100);
	assert (filesys_inited);

	struct merkleNode* merkleRoot = createMerkleTree(fnames[fd]);
	char* secHash = getSecureHash(fd);

	assert (hashSame(secHash,root[fd]->hash));
	assert (secHash != NULL);

	root[fd] = merkleRoot;

	if(!hashSame(secHash,merkleRoot->hash)){
		for(int x=0; x<20; x++) root[fd]->hash[x] = secHash[x];
		return -1;
	}

	assert(write (fd, buf, count)==count);
	root[fd] = createMerkleTree(fnames[fd]);
	assert (hashSame(root[fd]->hash,createMerkleTree(fnames[fd])->hash));
	assert(updateSecure(fd)==1);

	secHash = getSecureHash(fd);
	assert (hashSame(secHash,root[fd]->hash));

	s_updateSize(fd);
	return count;
}

/* check the integrity of blocks containing the 
 * requested data.
 * returns -1 on failing the integrity check.
 */
ssize_t s_read (int fd, void *buf, size_t count)
{
	assert (filesys_inited);
	assert(fd<100);
	//Step 1: Compute the BLOCKS of the file that need to be read
	//Step 2: Read the blocks
	//Step 3: Check Integrity (if fail return -1)

	struct merkleNode* merkleRoot = createMerkleTree(fnames[fd]);

	char* secHash = getSecureHash(fd);
	assert (hashSame(secHash,root[fd]->hash));
	
	if(!hashSame(merkleRoot->hash, secHash)) 
		return -1;

	return read (fd, buf, count);
}

void destroyTree(struct merkleNode* x){
	if(x == NULL)
		return;
	// destroyTree(x->leftChild);
	// destroyTree(x->rightChild);
	free(x);
}

/* destroy the in-memory Merkle tree */
int s_close (int fd)
{
	assert(fd<100);
	assert (filesys_inited);
	destroyTree(root[fd]);
	return close (fd);
}

/* Check the integrity of all files in secure.txt
 * remove the non-existent files from secure.txt
 * returns 1, if an existing file is tampered
 * return 0 on successful initialization
 */
int filesys_init (void)
{
	assert(filesys_inited == 0);

	system ("touch secure.txt"); //if secure.txt does not exist, CREATE
	system ("mv secure.txt secure_temp.txt");
	int fdTo = open("secure.txt", O_CREAT|O_WRONLY, S_IRUSR|S_IWUSR);
	int fdFrom = open("secure_temp.txt", O_RDONLY, S_IRUSR|S_IWUSR);
	assert(fdTo != -1);
	assert(fdFrom != -1);

	int n, integrityLost = 0;
	char secureBlock[52]; // 32 for fileName + 20 for Root Hash
	while((n = read(fdFrom, secureBlock, sizeof(secureBlock))) > 0){
		assert(n == 52);
		
		char filename[32];
		char hash[20];
		for(int i = 0; i < 32; i++) filename[i] = secureBlock[i];
		for(int j = 0; j < 20; j++) hash[j] = secureBlock[j+32];

		//only keep those entries in secure.txt, who's file exists
		if(access(filename, F_OK ) != -1){
			assert(write(fdTo, secureBlock, 52)==52);

			//file exists, but it has a different hash, than the one in secure.txt
			if(!(hashSame(hash,createMerkleTree(filename)->hash)))
				integrityLost = 1;
		}
	}
	close(fdFrom);
	close(fdTo);
	system ("rm secure_temp.txt");
	if(integrityLost)
		return 1; 

	filesys_inited = 1;
	return 0; //on success
}
