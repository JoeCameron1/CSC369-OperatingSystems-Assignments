#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include "pagetable.h"


extern int memsize;

extern int debug;

extern struct frame *coremap;

/* Page to evict is chosen using the optimal (aka MIN) algorithm. 
 * Returns the page frame number (which is also the index in the coremap)
 * for the page that is to be evicted.
 */

int opt_evict() {
	
    int page_to_be_evicted;
    
    int current = -1;
    int outdated;
    
    int switch_class = 0;
    int class = 0;
    
    int x = 0;
    while (x < memsize) {
        if ((((coremap[x].pte)->frame) >> 30) == class) {
            if (current == -1) {
                page_to_be_evicted = x;
                outdated = coremap[x].placeholder;
                coremap[x].placeholder = 0;
                switch_class = 1;
            } else {
                coremap[page_to_be_evicted].placeholder = outdated - 1;
                page_to_be_evicted = x;
                outdated = coremap[x].placeholder;
                coremap[x].placeholder = 0;
            }
        }
        if (coremap[x].placeholder > 1) {
            coremap[x].placeholder = coremap[x].placeholder - 1;
        }
        coremap[x].ceiling = coremap[x].ceiling - 1;
        x = x + 1;
    }
    
    class = 2;
    
    int i;
    for (i = 0; i < 3; i++) {
        
        if (switch_class == 0) {
            
            int y = 0;
            while (y < memsize) {
                if ((((coremap[y].pte)->frame) >> 30) == class) {
                    if (current == -1) {
                        page_to_be_evicted = y;
                        outdated = coremap[y].placeholder;
                        coremap[y].placeholder = 0;
                        switch_class = 1;
                    } else {
                        coremap[page_to_be_evicted].placeholder = outdated - 1;
                        page_to_be_evicted = y;
                        outdated = coremap[y].placeholder;
                        coremap[y].placeholder = 0;
                    }
                }
                y = y + 1;
            }
            
            if (class == 2) {
                class = 1;
            } else {
                class = 3;
            }
            
        }
        
    }
    
    return page_to_be_evicted;
    
}

/* This function is called on each access to a page to update any information
 * needed by the opt algorithm.
 * Input: The page table entry for the page that is being accessed.
 */
void opt_ref(pgtbl_entry_t *p) {
    
    if ((((p-> frame) >> 29) & 1) == 1) {
        
        int i = 0;
        while (i < memsize) {
            if(coremap[i].pte == p) {
                if (coremap[i].placeholder == 0) {
                    coremap[i].placeholder = coremap[i].ceiling;
                }
            }
            coremap[i].ceiling = coremap[i].ceiling + 1;
            i = i + 1;
        }
        
    }
    
}

/* Initializes any data structures needed for this
 * replacement algorithm.
 */
void opt_init() {
    
    int i = 0;
    while (i < memsize) {
        coremap[i].placeholder = 0;
        coremap[i].ceiling = 1;
        i = i + 1;
    }
    
}

