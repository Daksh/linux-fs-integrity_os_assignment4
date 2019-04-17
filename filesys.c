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

static struct merkleNode* root[100]; //assuming fd lies in [0,99]
static char* fnames[100];
static int filesys_inited = 0;

struct merkleNode{
	char hash[21];//last byte for '\0'
	struct merkleNode *leftChild;
	struct merkleNode *rightChild;
};

void destroyTree(struct merkleNode* x){
	if(x == NULL)
		return;
	destroyTree(x->leftChild);
	destroyTree(x->rightChild);
	x = NULL;
	free(x);
}

int hashSame(char* h1, char* h2){
	for(int i=0; i<20; i++)
		if( h1[i]!=h2[i] )
			return 0;
	return 1;
}

void merkleTreeTraverse(int fd){
	struct merkleNode* rootNode = root[fd];
	// printf("%x\n", rootNode->hash);
	for(int i=0; i<20; i++)
		printf("%d", rootNode->hash[i]);
	printf("\n");
}

void printHash(char* has){
	for(int i=0; i<20; i++)
		printf("%d", has[i]);
	printf("\n");
}



/* returns 20 bytes unique hash of the buffer (buf) of length (len)
 * in input array sha1.
 */
void get_sha1_hash (const void *buf, int len, const void *sha1)
{
	SHA1 ((unsigned char*)buf, len, (unsigned char*)sha1);
}

struct merkleNode* createMerkleTree(int fd){	
	// printf("%s\n","I am here" );
	char blk[65];
	memset (blk, '0', 64);
    // fd = open (fnames[fd], O_RDONLY, 0);
    // printf(" fd is %d\n",fd );
    //Handling empty file case, and case when read is returning -1
    // int ptr1 = lseek(fd, 0, SEEK_END);    
    // printf("PTR1 is %d\n",ptr1);
    lseek(fd, 0, SEEK_SET);
    // printf("start is  %d\n",ptr );
    int numCan = read (fd, blk, 64);
    // printf("numCan is  %d\n",numCan);
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
		//printf("%s\n","after leaves" );
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
	// printf("%s\n","Last block data" );
	// printf("%s\n",blk);
	// printf("%s\n","Block last Hash value" );
	// printHash(level[levelCount-1]->hash);		
	//close(fd);

	while(levelCount>1){
		int pCount;
		char blk[41];
		for(pCount = 0; pCount < levelCount/2; pCount++){
			for(int i=0; i<20; i++) blk[i] = level[pCount*2]->hash[i];
			for(int i=0; i<20; i++) blk[20+i] = level[pCount*2+1]->hash[i];
			blk[40] = '\0';
			//printf("%s\n","after internal" );
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
	if(level[0]!=NULL)
    {
        destroyTree(level[0]->leftChild);
        destroyTree(level[0]->rightChild);        
    }

	// destroyTree(level[0]->leftChild);
	// destroyTree(level[0]->rightChild);
	//level[0]->leftChild=NULL;
	//level[0]->rightChild=NULL;

	return level[0];
}

//updates secure.txt
int updateSecure(int fd){
	char* fName = fnames[fd];
	// printf("fnames wala %s\n",fName );
// 	char filePath[100];
// if (fcntl(fd, F_GETPATH, filePath) != -1)
// {
// 	printf("%s\n",filePath);
//     // do something with the file path
// }
	char* updatedHash = root[fd]->hash;
	int secureFD = open("secure.txt", O_RDWR, S_IRUSR|S_IWUSR);
	int n;
	char secureBlock[53]; // 32 for fileName + 20 for Root Hash + 1 for '\0'
	lseek(secureFD,0,SEEK_SET);
	while( (n = read(secureFD, secureBlock, 52)) > 0)
	{
		assert(n == 52);
		char filename[33];
		//char hash[21];
		for(int i = 0; i < 32; i++) filename[i] = secureBlock[i];
		
		filename[32]='\0';

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

//Returns the hash of a file existing in secure.txt. If DNE, returns null.
char* checkSecure(int fd) {
	char* fName = fnames[fd];
	int secureFD = open("secure.txt", O_RDWR, S_IRUSR|S_IWUSR);
	int n;
	char secureBlock[53]; // 32 for fileName + 20 for Root Hash
	char *hash = (char*)malloc(21);
	lseek(secureFD,0,SEEK_SET);
	while( (n = read(secureFD, secureBlock, 52) )> 0){
		assert(n == 52);
		char filename[33];
		
		for(int i = 0; i < 32; i++) filename[i] = secureBlock[i];
		for(int j = 0; j < 20; j++) hash[j] = secureBlock[j+32];
		filename[32]='\0';
		hash[20]='\0';
		if( !strcmp(filename,fName) ){
			close(secureFD);
			return hash; //check
		}

	}
	free(hash);
	close(secureFD);
	return NULL;
}


char* getSecureHash(int fd){
	assert(access( "secure.txt", F_OK ) != -1);//secure.txt must exist

	char* fName = fnames[fd];
	char* retHash = (char *)malloc(21);
	int secureFD = open("secure.txt", O_RDWR, S_IRUSR|S_IWUSR);
	//check this
	assert(lseek(secureFD,0,SEEK_CUR)==0);
	int n;
	char secureBlock[53]; // 32 for fileName + 20 for Root Hash
	secureBlock[52] = '\0';	
	// might need lseek(beginning)
	while((n = read(secureFD, secureBlock, sizeof(secureBlock)-1)) > 0){
		assert(n == 52);
		
		char filename[33];
		for(int i = 0; i < 32; i++) filename[i] = secureBlock[i];
		for(int j = 0; j < 20; j++) retHash[j] = secureBlock[j+32];
		filename[32]='\0';
		retHash[20]='\0';
		if( !strcmp(filename,fName) ){
			close(secureFD);
			// printHash(retHash);
			return retHash;
		}
	}
	close(secureFD);
	return NULL;
}

int appendSecure(int fd){
	assert(access( "secure.txt", F_OK ) != -1);//secure.txt must exist
	//check
	char* fName = fnames[fd];
	char* addHash = root[fd]->hash;

	int secureFD = open("secure.txt", O_RDWR, S_IRUSR|S_IWUSR);
	//might be wrong
	assert(lseek(secureFD,0,SEEK_CUR)==0);
	lseek(secureFD, 0, SEEK_END);
	assert(write(secureFD, fName, 32)==32);
	assert(write(secureFD, addHash, 20)==20);
	close(secureFD);
	return 0;
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

	int fd = open(pathname, flags|O_RDWR, mode);
	// printf("%d\n", fd);
	fnames[fd] = (char *)malloc(33);
	//check argument 33
	snprintf(fnames[fd], 33, "%s", pathname);
	// printf("s_open adding fnames[%d]: %s\n", fd, fnames[fd]);

	// printf("Fd before calling merkle in s_open %d\n",fd );
	struct merkleNode* merkleRoot = createMerkleTree(fd);
	root[fd] = merkleRoot;	
	//destroyTree(root[fd]->leftChild);
	//destroyTree(root[fd]->rightChild);
	//root[fd]->leftChild=NULL;
	//root[fd]->rightChild=NULL;
	// printf("%s\n","out" );
	char* secHash = getSecureHash(fd);
	// printf("secHash is \n");
	// printHash(secHash);
	// printf("%s\n","root ka hash");
	// printf("%s\n",root[fd]->hash );
	// printHash(root[fd]->hash);
	// assert(hashSame(secHash,root[fd]->hash));
	// printf("%s\n","hi" );	


	if(secHash == NULL){
		appendSecure(fd);
	} else{
		if(flags & O_TRUNC)
			updateSecure(fd);
		if(!hashSame(secHash,merkleRoot->hash)){
			close(fd);
			for(int x=0; x<20; x++) root[fd]->hash[x] = secHash[x];
			root[fd]->hash[20] ='\0';
			printf("%s\n","hash not same" );
			return -1;
		}
	}
	return fd;
}

// /* Build an in-memory Merkle tree for the file.
//  * Compare the integrity of file with respect to
//  * root hash stored in secure.txt. If the file
//  * doesn't exist, create an entry in secure.txt.
//  * If an existing file is going to be truncated
//  * update the hash in secure.txt.
//  * returns -1 on failing the integrity check.
//  */
// int s_open (const char *pathname, int flags, mode_t mode)
// {
// 	printf("s_open(%s, %d)\n", pathname, flags);
// 	assert (filesys_inited);
// 	int n;
// 	int fd1; //file descriptor for file
// 	int fd2; //file descriptor for secure.txt
// 	char buffer[53]; // 32 for fileName + 20 for Root Hash
// 	int existsInSecure = 0;
// 	/*
// 	If the file exists and secure.txt is there; then check for consistency
// 	If tampered, then return -1 

// 	otrunk -> truncate the file, make file size 0 (something like that)
// 	if not tampered..., then you are going to make the merkle
// 	if file size is 0, then consider the hash to be 0.

// 	if truncate flag is not given, then open call will not change the file
// 	so would not need to do anything with merkle, in that case
// 	*/
// 	int existsInFS = access( pathname, F_OK ) != -1;
// 	fd1 = open(pathname, flags, mode);
// 	fd2 = open("secure.txt", O_RDWR, S_IRUSR|S_IWUSR);

// 	fnames[fd1] = (char *)malloc(32);//IS THIS THE RIGHT PLAce
// 	int i;
// 	for(i=0; pathname[i]!='\0'; i++) fnames[fd1][i] = pathname[i];
// 	fnames[fd1][i] = '\0';

// 	//Step 1: Build in-memory merkle tree
// 	if( existsInFS ) {
// 		printf("existsInFS\n");
// 		root[fd1] = createMerkleTree(fd1);
// 		// merkleTreeTraverse(fd1);
// 		lseek(fd2,0,SEEK_SET);
// 		while((n = read(fd2, buffer, 52)) > 0){
// 			buffer[52]='\0';
// 			char filename[33];
// 			char hash[21];
// 			for(int i = 0; i < 32; i++) 
// 				filename[i]=buffer[i];
// 			for(int j = 0; j < 20; j++) 
// 				hash[j]=buffer[j+32];
// 			filename[32]='\0';
// 			hash[20]='\0';
// 			if( !strcmp(filename,pathname) ){
// 				existsInSecure = 1;
// 				if(!(flags & O_TRUNC)){
// 					//Step 2: Compare Root Hash (if fail return -1)
// 					if(!hashSame(hash, root[fd1]->hash)){
// 						// printf("%s\n", root[fd1]->hash);
// 						printHash(root[fd1]->hash);
// 						// printf("%s\n", hash);
// 						printHash(hash);
// 						printf("hash ki dikkat hai\n");
// 						return -1;
// 					}
// 					return fd1;
// 				}
// 				else{
// 					lseek(fd2, -20, SEEK_CUR);
// 					//Update the root hash in secure.txt
// 					write(fd2, root[fd1]->hash, 20);
// 					lseek(fd2,0,SEEK_END);
// 					return fd1;
// 				}
// 			}
// 		}

// 		//If the file exists in FS, secure.txt must have an entry for it
// 		assert(existsInSecure);

// 	} else {
// 		printf("DNE InFS\n");
// 		root[fd1] = (struct merkleNode*) malloc( sizeof(struct merkleNode) );
// 		// memset(root[fd1]->hash, '0', 64);
// 		root[fd1] = createMerkleTree(fd1);
// 		root[fd1] -> leftChild = NULL;
// 		root[fd1] -> rightChild = NULL;

// 		//Step 3: if file DNE, create entry in secure.txt
// 		char fn[33];
// 		memset(fn, '0', 32);
// 		int i;
// 		for(i=0; pathname[i]!='\0'; i++) fn[i] = pathname[i];
// 		//assert(i<31);
// 		assert(pathname[i]=='\0');
// 		fn[i]=pathname[i];

// 		lseek(fd2, 0, SEEK_END);
// 		write(fd2, fn, 32);
// 		write(fd2, root[fd1]->hash, 20);
// 		return fd1;
// 	}
// 	assert(1==0);
// 	return -1;//SHOULD NOT REACH HERE
// }

/* SEEK_END should always return the file size 
 * updated through the secure file system APIs.

 * from unistd.h; whence values for lseek(2)
	#define	SEEK_SET	0	set file offset to offset
	#define	SEEK_CUR	1	set file offset to current plus offset
	#define	SEEK_END	2	set file offset to EOF plus offset
 */
int s_lseek (int fd, long offset, int whence)
{
	assert(1==0);
	assert(fd<100);
	assert (filesys_inited);
	int ret = lseek (fd, offset, whence);
	//printf("ret: %d\n", ret);
	return ret;
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
	assert(fd<100);
	assert (filesys_inited);

	//printf("Fd before calling merkle %d\n",fd );
	struct merkleNode* prev = createMerkleTree(fd);
	//printf("%s\n","after" );
	if(!hashSame(prev->hash, root[fd]->hash)){
		printHash(prev->hash);
		printHash(root[fd]->hash);
		// printHash(checkSecure(fd));
		return -1;
	}

	//destroyTree(prev);

	//printf("%s\n","after11" );


	int ret = write (fd, buf, count);//CHECK OUTPUT MAYBE
	root[fd] = createMerkleTree(fd);
	//printf("%s\n","after11" );
	//destroyTree(root[fd]->leftChild);
	//destroyTree(root[fd]->rightChild);
	//root[fd]->leftChild=NULL;
	//root[fd]->rightChild=NULL;
	// merkleTreeTraverse(fd);

	// CHANGE SECURE.TXT
	assert(updateSecure(fd)==1);

	return ret;
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

	struct merkleNode* prev = createMerkleTree(fd);
	if(!hashSame(prev->hash, root[fd]->hash)) 
		return -1;
	destroyTree(prev);
	return read (fd, buf, count);
}



/* destroy the in-memory Merkle tree */
int s_close (int fd)
{
	assert(fd<100);
	assert (filesys_inited);
	destroyTree(root[fd]);
	return close (fd);
}


//Check the integrity of all files in secure.txt using fd of secure.txt
//returns 1 if integrity compromised. Else 0
int checkIntegrity(int secureFD)
{
	int n;
	char secureBlock[53]; // 32 for fileName + 20 for Root Hash
	// char *hash = (char*)malloc(21);
	lseek(secureFD,0,SEEK_SET);
	while( (n = read(secureFD, secureBlock, 52) )> 0){
		assert(n == 52);
		char filename[33];
		char hash[21];
		for(int i = 0; i < 32; i++) filename[i] = secureBlock[i];
		for(int j = 0; j < 20; j++) hash[j] = secureBlock[j+32];
		filename[32]='\0';
		hash[20] = '\0';
		//I think should work

		if(access( filename, F_OK ) != -1)
		{
			// if file with given filename exists
			// when do I close this		
			int fd = open(filename,O_RDONLY , 0);
			struct merkleNode* merkleRoot = createMerkleTree(fd);

			// char* secHash = getSecureHash(fd);
			if(!hashSame(hash ,merkleRoot->hash))
			{
				//destroyTree(merkleRoot);
				// integrity for this file compromised
				return 1;
			}
			//destroyTree(merkleRoot);
			//closing now
			// close(fd);
		}
		else
		{

			lseek(secureFD,-52,SEEK_CUR);
			// not sure
			memset(secureBlock,'0',52);// Do check . Resort to copy to delete if this doesn't work
			write(secureFD,secureBlock,52);
		}
		


		// I think I can avoid this
		/*
		fnames[fd] = (char *)malloc(33);
		//check argument 33
		snprintf(fnames[fd], 33, "%s", pathname);
		printf("s_open adding fnames[%d]: %s\n", fd, fnames[fd]);

		*/
		//continue
	}
	return 0; // success
}

/* Check the integrity of all files in secure.txt
 * remove the non-existent files from secure.txt
 * returns 1, if an existing file is tampered
 * return 0 on successful initialization
 */
int filesys_init (void)
{
	assert(filesys_inited == 0);
	//if secure.txt does not exist, CREATE . Anmol is sure
	int fd = open("secure.txt", O_CREAT|O_WRONLY, S_IRUSR|S_IWUSR);
	assert(fd != -1);

	//Checking the integrity of all the files in secure.txt
	int integrity = checkIntegrity(fd);
	filesys_inited = 1;
	close(fd);	
	return integrity;
	// Check the integrity of all the files whos hashes exist in secure.txt - no more

	// if a file DNE, just throw away corresponding entry in secure.txt, (if entry exists)

	// if Integrity of an existing file is compromised, return 1


	// return 0; //on success
}