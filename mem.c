////////////////////////////////////////////////////////////////////////////////
// Main File:        mem.c
// This File:        mem.c
// Other Files:      -
// Semester:         CS 354 Spring 2019
//
// Author:           Pooja Sivakumar
// Email:            psivakumar2@wisc.edu
// CS Login:         sivakumar
//
/////////////////////////// OTHER SOURCES OF HELP //////////////////////////////
//                   fully acknowledge and credit all sources of help,
//                   other than Instructors and TAs.
//
// Persons:          -
//
//
// Online sources:   -
//
//
////////////////////////////////////////////////////////////////////////////////



#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdio.h>
#include <string.h>
#include "mem.h"




/*
 * This structure serves as the header for each allocated and free block.
 * It also serves as the footer for each free block but only containing size.
 */
typedef struct block_header {
        int size_status;
    /*
    * Size of the block is always a multiple of 8.
    * Size is stored in all block headers and free block footers.
    *
    * Status is stored only in headers using the two least significant bits.
    *   Bit0 => least significant bit, last bit
    *   Bit0 == 0 => free block
    *   Bit0 == 1 => allocated block
    *
    *   Bit1 => second last bit 
    *   Bit1 == 0 => previous block is free
    *   Bit1 == 1 => previous block is allocated
    * 
    * End Mark: 
    *  The end of the available memory is indicated using a size_status of 1.
    * 
    * Examples:
    * 
    * 1. Allocated block of size 24 bytes:
    *    Header:
    *      If the previous block is allocated, size_status should be 27
    *      If the previous block is free, size_status should be 25
    * 
    * 2. Free block of size 24 bytes:
    *    Header:
    *      If the previous block is allocated, size_status should be 26
    *      If the previous block is free, size_status should be 24
    *    Footer:
    *      size_status should be 24
    */
} block_header;         

/* Global variable - DO NOT CHANGE. It should always point to the first block,
 * i.e., the block at the lowest address.
 */

block_header *start_block = NULL;

/* 
 * Function for allocating 'size' bytes of heap memory.
 * Argument size: requested size for the payload
 * Returns address of allocated block on success.
 * Returns NULL on failure.
 * This function should:
 * - Check size - Return NULL if not positive or if larger than heap space.
 * - Determine block size rounding up to a multiple of 8 and possibly adding padding as a result.
 * - Use BEST-FIT PLACEMENT POLICY to find the block closest to the required block size
 * - Use SPLITTING to divide the chosen free block into two if it is too large.
 * - Update header(s) and footer as needed.
 * Tips: Be careful with pointer arithmetic.
 */
void* Alloc_Mem(int size) {
    if (size < 0 ){
		return NULL;
	}
	
	size= size + 4;  // for header
	
		if (size% 8 != 0) {
			size+=( 8 - size%8);
	} //make sure it is a multiple of 8
	
	block_header *temp = NULL;
	temp = start_block; //start at the start
	 //end at the end (which is startblock's size bytes away from start block)
	block_header *bestFit = NULL; //to store the best fit
	int flag = 0;
	int min = -1;
	 // to hold the minimum difference between the free blocks and the requested block size (to find the best block that is closest to the requested block size)
	while (temp->size_status != 1) {
		if ((temp->size_status & 1) == 0) { //if the block is free
			if ((temp->size_status - (temp->size_status & 3)) - size >= 0) {
				if (flag == 0) { //if its the first time we move in the heap
					min = (temp->size_status - (temp->size_status & 3)) - size; //store the difference
					bestFit = temp;
					flag ++;
				}
				else { 
					if ((temp->size_status - (temp->size_status & 3)) - size < min) {
						min = (temp->size_status - (temp->size_status & 3)) - size; //store the smallest difference
						bestFit = temp; //store that address
					}
				}
				}
		}
					temp = (void*)temp + (temp->size_status - (temp->size_status & 3));	 //move the current
	}
				
	if (min == -1) {
		return NULL;
	}

	
	//to split the block
	if (bestFit->size_status - (bestFit->size_status & 3) > size )  {
		int bestFitSize = bestFit->size_status - (bestFit->size_status & 3);
		
		if ((bestFit->size_status & 1) == 0) {
			bestFit->size_status = size + 1; //bestFit gets requested size
		}
		
		//update header and make footer for the latter part of the split
		temp = (void*)bestFit + (bestFit->size_status - (bestFit->size_status & 3));
		temp->size_status = bestFitSize - size + 2;
		block_header *footer = (void*)temp + ((temp->size_status - (temp->size_status & 3)) - 4);
		footer->size_status = (temp->size_status - (temp->size_status & 3));
	}
	
	//if you don't need to split
	else {
		bestFit->size_status = bestFit->size_status + 1; //bestFit is allocated
		temp = (void*)bestFit + (bestFit->size_status - (bestFit->size_status & 3));
		if (temp->size_status != 1)
		temp->size_status += 2; //update header of the next block if allowed
	}
		
	return bestFit + 1;
	
}

/* 
 * Function for freeing up a previously allocated block.
 * Argument ptr: address of the block to be freed up.
 * Returns 0 on success.
 * Returns -1 on failure.
 * This function should:
 * - Return -1 if ptr is NULL.
 * - Return -1 if ptr is not a multiple of 8.
 * - Return -1 if ptr is outside of the heap space. //how to do this???
 * - Return -1 if ptr block is already freed.
 * - USE IMMEDIATE COALESCING if one or both of the adjacent neighbors are free.
 * - Update header(s) and footer as needed.
 */                    
int Free_Mem(void *ptr) {         


    if (ptr == NULL) {
		return -1;
	}
	if ((unsigned int)ptr % 8 != 0) {
			return -1;
	}
	
	//to keep track of the current block
	block_header *curr = (block_header*)(ptr - 4);
	int currSize = curr->size_status - (curr->size_status & 3);
	
	//to keep track of the prev block
	block_header *prevFooter = (block_header*)((void *)curr - 4);
	int prevSize = prevFooter->size_status;
	block_header *prev = (block_header *)((void*)curr - prevSize);
	
	//to keep track of the next block
	block_header *next = (block_header*)((void*)curr + currSize);
	int nextSize = next->size_status - (next->size_status & 3);
	block_header *nextFooter = (block_header*)((void*)next + (nextSize - 4));
	
	//if curr is already freed
	if ((curr->size_status & 1) == 0) {
		return -1;
	}

   
    block_header *temp_block = start_block;
    
    while(temp_block -> size_status != 1){
        
        // move through the heap space until it reaches the last block
        temp_block = (void*)temp_block + (temp_block ->size_status) - (temp_block->size_status&3);
        
    }
    //if passed pointer is outside heap range, error
    if ((block_header*) ptr >= temp_block){

        return -1;
    }
	
	//if both prev and next are free
	if ((prev->size_status & 1) == 0 && (next->size_status & 1) == 0) {
		if ((prev->size_status & 2) == 1) {
			//if prev's prev is allocated
			prev->size_status = prevSize + currSize + nextSize + 2;
		}
		else {
			prev->size_status = prevSize + currSize + nextSize;
		}
		//update next's header 
		nextFooter->size_status = prevSize + currSize + nextSize;
		block_header *temp = (void*)nextFooter + 4;
		//if not at end
		
		return 0;
	}
	//if only prev is free
	else if ((prev->size_status & 1) == 0) {
		//if prev's prev is allocated
		if ((prev->size_status & 2) == 1) {
			prev->size_status = prevSize + currSize + 2;
		}
		else {
			prev->size_status = prevSize + currSize;
		}
		//create a footer
		block_header * currFooter = (void *)prev + ((prev->size_status - (prev->size_status & 3)) - 4);
		currFooter->size_status = (prev->size_status - (prev->size_status & 3));
		//if curr is not the last block, update next's header
		if (next->size_status != 1)
		next->size_status -=2;
		return 0;
	}
	//if next only is free
	else if ((next->size_status & 1) == 0) 
	{
		curr->size_status = currSize + nextSize + 2;
		//create a footer
		block_header* currFooter = (void*)curr + (currSize + nextSize - 4);
		currFooter->size_status = currSize + nextSize;
		block_header * temp = (void*)currFooter + 4;
		//if next block is not end
		
	}
	else {
			curr->size_status --;
			return 0;
	}
	return -1;
	
}

/*
 * Function used to initialize the memory allocator.
 * Intended to be called ONLY once by a program.
 * Argument sizeOfRegion: the size of the heap space to be allocated.
 * Returns 0 on success.
 * Returns -1 on failure.
 */                    
int Init_Mem(int sizeOfRegion) {         
    int pagesize;
    int padsize;
    int fd;
    int alloc_size;
    void* space_ptr;
    block_header* end_mark;
    static int allocated_once = 0;
  
    if (0 != allocated_once) {
        fprintf(stderr, 
        "Error:mem.c: Init_Mem has allocated space during a previous call\n");
        return -1;
    }
    if (sizeOfRegion <= 0) {
        fprintf(stderr, "Error:mem.c: Requested block size is not positive\n");
        return -1;
    }

    // Get the pagesize
    pagesize = getpagesize();

    // Calculate padsize as the padding required to round up sizeOfRegion 
    // to a multiple of pagesize
    padsize = sizeOfRegion % pagesize;
    padsize = (pagesize - padsize) % pagesize;

    alloc_size = sizeOfRegion + padsize;

    // Using mmap to allocate memory
    fd = open("/dev/zero", O_RDWR);
    if (-1 == fd) {
        fprintf(stderr, "Error:mem.c: Cannot open /dev/zero\n");
        return -1;
    }
    space_ptr = mmap(NULL, alloc_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, 
                    fd, 0);
    if (MAP_FAILED == space_ptr) {
        fprintf(stderr, "Error:mem.c: mmap cannot allocate space\n");
        allocated_once = 0;
        return -1;
    }
  
    allocated_once = 1;

    // for double word alignment and end mark
    alloc_size -= 8;

    // To begin with there is only one big free block
    // initialize heap so that start block meets 
    // double word alignement requirement
    start_block = (block_header*) space_ptr + 1;
    end_mark = (block_header*)((void*)start_block + alloc_size);
  
    // Setting up the header
    start_block->size_status = alloc_size;

    // Marking the previous block as used
    start_block->size_status += 2;

    // Setting up the end mark and marking it as used
    end_mark->size_status = 1;

    // Setting up the footer
    block_header *footer = (block_header*) ((char*)start_block + alloc_size - 4);
    footer->size_status = alloc_size;
  
    return 0;
}         
                 
/* 
 * Function to be used for DEBUGGING to help you visualize your heap structure.
 * Prints out a list of all the blocks including this information:
 * No.      : serial number of the block 
 * Status   : free/used (allocated)
 * Prev     : status of previous block free/used (allocated)
 * t_Begin  : address of the first byte in the block (where the header starts) 
 * t_End    : address of the last byte in the block 
 * t_Size   : size of the block as stored in the block header
 */                     
void Dump_Mem() {         
    int counter;
    char status[5];
    char p_status[5];
    char *t_begin = NULL;
    char *t_end = NULL;
    int t_size;

    block_header *current = start_block;
    counter = 1;

    int used_size = 0;
    int free_size = 0;
    int is_used = -1;

    fprintf(stdout, "************************************Block list***\
                    ********************************\n");
    fprintf(stdout, "No.\tStatus\tPrev\tt_Begin\t\tt_End\t\tt_Size\n");
    fprintf(stdout, "-------------------------------------------------\
                    --------------------------------\n");
  
    while (current->size_status != 1) {
        t_begin = (char*)current;
        t_size = current->size_status;
    
        if (t_size & 1) {
            // LSB = 1 => used block
            strcpy(status, "used");
            is_used = 1;
            t_size = t_size - 1;
        } else {
            strcpy(status, "Free");
            is_used = 0;
        }

        if (t_size & 2) {
            strcpy(p_status, "used");
            t_size = t_size - 2;
        } else {
            strcpy(p_status, "Free");
        }

        if (is_used) 
            used_size += t_size;
        else 
            free_size += t_size;

        t_end = t_begin + t_size - 1;
    
        fprintf(stdout, "%d\t%s\t%s\t0x%08lx\t0x%08lx\t%d\n", counter, status, 
        p_status, (unsigned long int)t_begin, (unsigned long int)t_end, t_size);
    
        current = (block_header*)((char*)current + t_size);
        counter = counter + 1;
    }

    fprintf(stdout, "---------------------------------------------------\
                    ------------------------------\n");
    fprintf(stdout, "***************************************************\
                    ******************************\n");
    fprintf(stdout, "Total used size = %d\n", used_size);
    fprintf(stdout, "Total free size = %d\n", free_size);
    fprintf(stdout, "Total size = %d\n", used_size + free_size);
    fprintf(stdout, "***************************************************\
                    ******************************\n");
    fflush(stdout);

    return;
}         
