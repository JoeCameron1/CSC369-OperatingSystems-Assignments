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

// Linked list to represent the lru queue.
typedef struct linked_list_node {
    int frame; // Frame number.
    struct linked_list_node* next_node; // Pointer to next node in linked-list.
} node;

node* head_of_lru;
node* tail_of_lru;

bool* referenced; // BitMap that records referenced frames.

/* Page to evict is chosen using the accurate LRU algorithm.
 * Returns the page frame number (which is also the index in the coremap)
 * for the page that is to be evicted.
 */

int lru_evict() {
	
    assert(head_of_lru != NULL); // Check the queue isn't empty.
    
    int frame = head_of_lru->frame; // Recover frame number from head of lru list.
    
    if (head_of_lru == tail_of_lru) { // If only one element in the list.
        tail_of_lru = NULL;
    }
    
    assert(referenced[frame] == 1); // Check for errors.
    
    referenced[frame] = 0; // Note unreferenced frame.
    
    node* new_head_of_lru = head_of_lru->next_node;
    free(head_of_lru);
    head_of_lru = new_head_of_lru;
    
    return frame;
    
}

/* This function is called on each access to a page to update any information
 * needed by the lru algorithm.
 * Input: The page table entry for the page that is being accessed.
 */
void lru_ref(pgtbl_entry_t *p) {
    
    int frame = p->frame >> PAGE_SHIFT;
    
    if (referenced[frame] == 1) { // If referenced.
        
        node* new_node = (node*)malloc(sizeof(node));
        new_node->frame = frame;
        new_node->next_node = NULL;
        
        tail_of_lru->next_node = new_node;
        tail_of_lru = new_node;
        
        node* p = head_of_lru;
        node* previous_node = NULL;
        
        while (p->frame != frame) {
            previous_node = p;
            p = p->next_node;
        }
        
        if (previous_node != NULL) {
            previous_node->next_node = p->next_node;
            free(p);
        } else {
            head_of_lru = p->next_node;
            free(p);
            if (head_of_lru == NULL) {
                tail_of_lru = NULL;
            }
        }
        
    } else {
        
        referenced[frame] = 1; // Note referenced frame.
        
        node* new_node = (node*)malloc(sizeof(node));
        new_node->frame = frame;
        new_node->next_node = NULL;
        
        if (tail_of_lru == NULL) {
            tail_of_lru = new_node;
            head_of_lru = tail_of_lru;
        } else {
            tail_of_lru->next_node = new_node;
            tail_of_lru = new_node;
        }
        
    }
    
}


/* Initialize any data structures needed for this 
 * replacement algorithm 
 */
void lru_init() {
    
    head_of_lru = NULL;
    tail_of_lru = NULL;
    referenced = malloc(sizeof(bool) * memsize);
    memset(referenced, 0, sizeof(bool) * memsize);
    
}
