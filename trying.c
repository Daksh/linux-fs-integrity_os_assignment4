#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <openssl/sha.h>
#include "filesys.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include<stdlib.h>
#include<stdio.h> 
#include <string.h>
int main() 
{    
    // declare and initialize string 
    // char str[] = "Geeks";
    // char str2[5];
  	
    // // print string 
    // printf("%s",str); 
    // printf("%d\n",(int)strlen(str) );
    // printf("%d\n",str[5] );
    // printf("%d\n",str[6] );
    printf("%s\n","JH" );
   	// int fd = open("secure.txt", O_CREAT|O_WRONLY, S_IRUSR|S_IWUSR);
    int fd = open("secure.txt",O_RDWR);
    printf("fd = %d\n", fd); 
	assert(fd != -1);

	// FILE* file_;
	// FILE* file_ = fdopen(fd, "w+"); 
    lseek(fd, 0, SEEK_SET);
    char buffer[52]; // 32 bytes for file,  20 bytes for hash , 1 byte for null
    int n;
    // while (!feof(file_)) // to read file 
    // { 
        // fucntion used to read the contents of file 
        while((n = read(fd, buffer, sizeof(buffer))) > 0)
{
		// assert(n==52);

        // fread(buffer, sizeof(buffer), 1, file_); 
		printf(" Bytes read = %d\n",n );
        printf("Buffer is  %s\n",buffer);
	    // printf("Buffer is  %s\n",buffer);
	    // printf("First char %d Second char %d Third char %d\n", buffer[52], buffer[53],buffer[54]);
     //    printf("Size of Buffer %d\n",(int)strlen(buffer));       	    
        char filename[32];
        char root_hash[20];
        for (int i = 0; i<=51; ++i)
        {

        	if(i<=31)
        	{
        		filename[i] = buffer[i];// null character will also get copied
        	}

        	else
        	{
        		root_hash[i-32] = buffer[i];
        	}
        }
        printf("Buffer is  %s\n",buffer);
        printf("Size of Buffer before %d\n",(int)strlen(buffer));       	                            
        filename[32] = '\0';
        // printf("buffer[32] %c\n", buffer[32]);
        printf("Size of Buffer middle %d\n",(int)strlen(buffer));       	                            
        root_hash[20] = '\0';
        printf("Size of Buffer after %d\n",(int)strlen(buffer));       	                            
        //hash1 =  calculate root hash of filename 
        //compare hash1 with root_hash
        //compare(root)
        printf("Size of filename %d\n",(int)strlen(filename) );
        printf("Size of root_hash %d\n",(int)strlen(root_hash));       
        printf(" filename %s\n",filename);
        printf(" hash %s\n", root_hash);
        printf("Buffer is %s\n",buffer);
        printf("Size of Buffer %d\n",(int)strlen(buffer));       	            
    } 	  
    return 0; 
} 