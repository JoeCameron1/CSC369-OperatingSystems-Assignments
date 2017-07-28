#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "pagetable.h"


extern int memsize;

extern int debug;

extern struct frame *coremap;

int clockhand; // Used to look for unreferenced frames. Mimics a clockhand.

bool* referenced; // Referenced Frames (Represented as a BitMap).

/* Page to evict is chosen using the clock algorithm.
 * Returns the page frame number (which is also the index in the coremap)
 * for the page that is to be evicted.
 */

int clock_evict() {
    
    int i = clockhand;
    while (i < memsize) {
        
        if (referenced[i]) {
            referenced[i] = 0;
        } else {
            referenced[i] = 1;
            clockhand = i;
            return i;
        }
        
        i = (i + 1) % memsize;
        
    }
    
    assert(0);
    
}

/* This function is called on each access to a page to update any information
 * needed by the clock algorithm.
 * Input: The page table entry for the page that is being accessed.
 */
void clock_ref(pgtbl_entry_t *p) {
    
    int frame = p->frame >> PAGE_SHIFT;
    referenced[frame] = 1;
    
}

/* Initialize any data structures needed for this replacement
 * algorithm. 
 */
void clock_init() {
    
    clockhand = 0;
    referenced = malloc(sizeof(bool) * memsize);
    memset(referenced, 0, sizeof(bool) * memsize);
    
}
