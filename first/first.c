#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>



int memory_reads = 0;
int memory_writes = 0;
int cache_hits = 0;
int cache_misses = 0;
int cacheSets;	//To store the total number of sets in the cache
int linesPerSet;	//To store the number of lines per set

struct line{
	unsigned long int tag;
	int validity;
	int toFill; //if toFill is 1, then this line shouls be filled
};
struct line ** cache;	//The main cache

void load (unsigned long int tag_value, unsigned long int set_index){
	//In this function I will load the tag_value into the set 'set_index' of the global dynamic cache
	
	int indexToFill = 0;	//This would be the index of the line into which I will fill my new information
	for(int j = 0; j<linesPerSet; j++){
	
		if((cache[set_index][j]).toFill == 1){
			indexToFill = j;
			(cache[set_index][j]).toFill = 0;
			if(j == (linesPerSet-1)){
				(cache[set_index][0]).toFill = 1;
			}else{
				(cache[set_index][j+1]).toFill = 1;
			}
			break;
		}
	}

	//fill up information at indexToFill
	
	(cache[set_index][indexToFill]).validity = 1;
	(cache[set_index][indexToFill]).tag = tag_value;
}
//This function is used to find the maximum toFill value in a given set which is helpful for the lru policy
int maxToFill(unsigned long int set_index){
	int max = 0;
	for (int j = 0; j < linesPerSet; j++){
		if((cache[set_index][j]).toFill > max){
			max = (cache[set_index][j]).toFill;
		}
	}
	return max;
}
//This function is used to find the line in set with the smallest toFill value for the lru policy
int smallestToFill(unsigned long int set_index){
       int small = 0;
	for (int j = 0; j < linesPerSet; j++){
		if((cache[set_index][j]).toFill < (cache[set_index][small]).toFill){
			small = j;
		}
	}
	return small;
}

void lruLoad(unsigned long int tag_value, unsigned long int set_index){
	int lineToFill = smallestToFill(set_index);	//This is where I enter the new information
	(cache[set_index][lineToFill]).validity = 1;
	(cache[set_index][lineToFill]).tag = tag_value;
	(cache[set_index][lineToFill]).toFill = (maxToFill(set_index)) + 1;
}

int main (int argc, char** argv){
	

	//making sure I have correct number of inputs
	if(!(argc == 6)){
		printf("error\n");
		return 0;
	}
	//printf("Number of command line inputs: %d\n", argc);
	//reading in all the inputs
	int cache_size = atoi(argv[1]);
        int block_size = atoi(argv[2]);
	char * cache_policy = argv[3];
	char * associativity = argv[4];
	char * tracefile = argv[5];
	
	int cacheLines = cache_size/block_size; //total number of lines in the cache
	//printf("the number of lines: %d\n", cacheLines);

	//making sure cache_policy is valid 
	if ( (strcmp(cache_policy, "fifo") != 0) && (strcmp(cache_policy, "lru") != 0)){
		printf("error\n");
		return 0;
	}
	//printf("cache policy is correct\n");
	//making sure trace file is valid
	FILE * file = fopen(tracefile,"r");

	if(file == NULL){
		printf("error\n");
		return 0;
	}
	//printf("file is present\n");
	//making sure associativity is valid and determining structure of cache
	
	if ( strcmp(associativity,"direct") == 0){
	       cacheSets = cacheLines;
       		linesPerSet = 1;
 	}else if(strcmp(associativity, "assoc") == 0){
		cacheSets = 1;
		linesPerSet = cacheLines;
	}else{
		int l = strlen(associativity);
		if (l < 7){
			printf("error\n"); return 0;
		}
		char copy[l+1];
		strcpy(copy, associativity);
		copy[6] = '\0';
		if (strcmp("assoc:", copy)!=0){
			printf("error\n"); return 0;
		}
		char n[l-6+1];
		for (int i = 0; i < l-5; i++){
			n[i] = associativity[i+6];
		}
		linesPerSet = atoi(n);
		cacheSets = cacheLines/linesPerSet;
	}
	//printf("lines per set: %d AND total sets: %d\n", linesPerSet, cacheSets);
	//END OF CHECKING AND DETERMINING DIMENSION OF CACHE
	
	//CREATING THE CACHE IN DYNAMIC MEMORY
	//The cache would be an array of 'caheSets' pointers to Lines
	
	cache = (struct line **) malloc (sizeof(struct line *) * cacheSets);
	//printf("cache partly created");
	//fflush(stdout);
	//each set would have be an array of 'linesPerSet' lines. 
	for (int i = 0; i < cacheSets; i++){
		cache[i] = (struct line *)malloc(sizeof(struct line) * linesPerSet);
	}
	//go through the lines and set validity and tag as zero
	
	for (int i = 0; i < cacheSets; i++){
		for(int j = 0; j < linesPerSet; j++){
			(cache[i][j]).tag = 0;
			(cache[i][j]).validity = 0;
			if (strcmp(cache_policy, "fifo") == 0){
				if (j ==0){
					(cache[i][j]).toFill = 1;
				}else{
					(cache[i][j]).toFill = 0;
				}
			}else{
				(cache[i][j]).toFill = j;
			}
		}
	}
	//printf("cache created and initialized");
	//fflush(stdout);
	//cache has been set up and initialized.
	//determining the number of bits for block offset, set, and tag
	int b = (int)(log(block_size)/log(2));
	int s = (int)(log(cacheSets)/log(2));
	int t = 48 - s - b;
	
	//Now, I will read the file line by line
	
	char c; // to store the first character of each line
	//char * hex_address = (char *) malloc((sizeof (char))*80); //to store the address in each line
	char hex_address [100];
	unsigned long int decimal_add; //The above address in unsigned decimal form
	//char *ptr = (char *) malloc ((sizeof (char))*100) ;	//to help convert string to long
	char *ptr ;//= (char *) malloc ((sizeof (char))*100) ;	//to help convert string to long
	//char ptr [100] ;	//to help convert string to long
	unsigned long int set_index;	//The set that the address belongs to
	unsigned long int tag_value;	//The tag of the address
	int check=0;	//To help determine if the data is already present in the cache or not
	int found=0;	//To store the line where we find the tag (if found)
	//printf("reached while loop");
	//fflush(stdout);
	while(1){

		fscanf(file,"%c",&c);
	//	printf("%c ", c);
	//	fflush(stdout);

		if( (c != 'R') && (c != 'W') ){
			break;
		}
		fscanf(file," 0x%s\n", hex_address);
	//	printf("%s\n", hex_address);
	//	fflush(stdout);

		decimal_add = (unsigned long int)strtol(hex_address, &ptr, 16);
		set_index = (decimal_add>>b)&((unsigned long int)(pow(2,s)-1));
		tag_value = (decimal_add>>(b+s))&((unsigned long int)(pow(2,t)-1));

		check = 0; 	//indicating this address is not in the cache
		found = 0;
		//searching if it is already in the cache:
		for (int j = 0; j < linesPerSet; j++){
			if ((cache[set_index][j]).validity == 0){
				continue;
			}

			if((cache[set_index][j]).tag == tag_value){
				check = 1;
				found = j;
				break;
			} 
		}

		//At this point if check is 1 then we found the value;
		if (check){
			if(c == 'R'){
				cache_hits++;
			}else{
				cache_hits++;
				memory_writes++;
			}
			if(strcmp(cache_policy, "lru") == 0){
				(cache[set_index][found]).toFill = (maxToFill(set_index))+1;
			}
			continue;
		}else{
			if(c == 'R'){
				cache_misses++;
				memory_reads++;
			}else{
				cache_misses++;
				memory_reads++;
				memory_writes++;
			}
			//If not found in the cache, the data should be loaded from memory into the cache
			if (strcmp(cache_policy, "lru") == 0){
				lruLoad(tag_value, set_index);
			}else{
				load(tag_value, set_index);
			}
		}
		
	}

	fclose(file);

	//free the cache
	for (int i = 0; i < cacheSets; i++){
		free(cache[i]);
	}
	free(cache);

	//Print output
	printf("Memory reads: %d\n",memory_reads);
	
	printf("Memory writes: %d\n",memory_writes);

	printf("Cache hits: %d\n",cache_hits);

	printf("Cache misses: %d\n",cache_misses);
}
	
