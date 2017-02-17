#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "traffic.h"

extern struct intersection isection;

/**
 * Populate the car lists by parsing a file where each line has
 * the following structure:
 *
 * <id> <in_direction> <out_direction>
 *
 * Each car is added to the list that corresponds with 
 * its in_direction
 * 
 * Note: this also updates 'inc' on each of the lanes
 */
void parse_schedule(char *file_name) {
    int id;
    struct car *cur_car;
    struct lane *cur_lane;
    enum direction in_dir, out_dir;
    FILE *f = fopen(file_name, "r");

    /* parse file */
    while (fscanf(f, "%d %d %d", &id, (int*)&in_dir, (int*)&out_dir) == 3) {

        /* construct car */
        cur_car = malloc(sizeof(struct car));
        cur_car->id = id;
        cur_car->in_dir = in_dir;
        cur_car->out_dir = out_dir;

        /* append new car to head of corresponding list */
        cur_lane = &isection.lanes[in_dir];
        cur_car->next = cur_lane->in_cars;
        cur_lane->in_cars = cur_car;
        cur_lane->inc++;
    }

    fclose(f);
}

/**
 * TODO: Fill in this function
 *
 * Do all of the work required to prepare the intersection
 * before any cars start coming
 * 
 */
void init_intersection() {
    
    int x;
    //Initialise locks and quadrants.
    for (x = 0; x < 4; x++) {
        
        pthread_mutex_init(&isection.quad[x], NULL);
        
    }
    
    int y;
    //Initialise locks and lanes.
    for (y = 0; y < 4; y++) {
        
        pthread_mutex_init(&isection.lanes[y].lock, NULL);
        pthread_cond_init(&isection.lanes[y].producer_cv, NULL);
        pthread_cond_init(&isection.lanes[y].consumer_cv, NULL);
        
        isection.lanes[y].in_cars = NULL;
        isection.lanes[y].out_cars = NULL;
        isection.lanes[y].inc = 0;
        isection.lanes[y].passed = 0;
        isection.lanes[y].buffer = malloc(sizeof(struct car *) * LANE_LENGTH);
        isection.lanes[y].head = 0;
        isection.lanes[y].tail = 0;
        isection.lanes[y].capacity = LANE_LENGTH;
        isection.lanes[y].in_buf = 0;
        
    }

}

/**
 * TODO: Fill in this function
 *
 * Populates the corresponding lane with cars as room becomes
 * available. Ensure to notify the cross thread as new cars are
 * added to the lane.
 * 
 */
void *car_arrive(void *arg) {
    
    struct lane* l = arg;
    
    while (1) {
        
        pthread_mutex_lock(&l->lock);
        
        // When the process is finished.
        if ((l->inc <= 0) || (l->in_cars == NULL)) {
            pthread_cond_signal(&l->consumer_cv);
            pthread_mutex_unlock(&l->lock);
            return NULL; // Stopping the infinite loop under the correct conditions.
        }
        
        // If the buffer is full.
        while (l->in_buf == l->capacity) {
            pthread_cond_wait(&l->consumer_cv, &l->lock);
        }
        
        // Putting the car in the buffer.
        l->buffer[l->tail] = l->in_cars;
        l->tail++;
        
        if (l->tail >= l->capacity) {
            l->tail = 0;
        }
        
        l->in_buf++;
        l->in_cars = l->in_cars->next;
        
        pthread_cond_signal(&l->producer_cv);
        
        pthread_mutex_unlock(&l->lock);
        
    }
    
}

/**
 * TODO: Fill in this function
 *
 * Moves cars from a single lane across the intersection. Cars
 * crossing the intersection must abide the rules of the road
 * and cross along the correct path. Ensure to notify the
 * arrival thread as room becomes available in the lane.
 *
 * Note: After crossing the intersection the car should be added
 * to the out_cars list of the lane that corresponds to the car's
 * out_dir. Do not free the cars!
 *
 * 
 * Note: For testing purposes, each car which gets to cross the 
 * intersection should print the following three numbers on a 
 * new line, separated by spaces:
 *  - the car's 'in' direction, 'out' direction, and id.
 * 
 * You may add other print statements, but in the end, please 
 * make sure to clear any prints other than the one specified above, 
 * before submitting your final code. 
 */
void *car_cross(void *arg) {
    
    struct lane *l = arg;
    
    while (1) {
        
        pthread_mutex_lock(&l->lock);
        
        // When the process is finished.
        if (l->inc <= 0) {
            pthread_mutex_unlock(&l->lock);
            return NULL; // Stopping the infinite loop under the correct conditions.
        }
        
        // While the buffer is empty.
        while (l->in_buf == 0) {
            pthread_cond_wait(&l->producer_cv, &l->lock);
        }
        
        // Get the car from the buffer.
        struct car *cur_car = l->buffer[l->head];
        
        l->head++;
        
        if (l->head >= l->capacity) {
            l->head = 0;
        }
        
        l->in_buf--;
        l->inc--;
        
        pthread_cond_signal(&l->consumer_cv);
        
        pthread_mutex_unlock(&l->lock);
        
        // Print statement for testing purposes.
        printf("in_dir: %d || out_dir: %d || ID: %d\n", cur_car->in_dir, cur_car->out_dir, cur_car->id);
        
        // Find the path.
        int* path = compute_path(cur_car->in_dir, cur_car->out_dir);
        
        int x = 0;
        while (x < (sizeof(path)/sizeof(int))) {
            pthread_mutex_lock(&isection.quad[path[x]]);
            x++;
        }
        
        struct lane* exit = &isection.lanes[cur_car->out_dir];
        
        pthread_mutex_lock(&exit->lock);
        
        cur_car->next = exit->out_cars;
        exit->out_cars = cur_car;
        exit->passed++;
        
        pthread_mutex_unlock(&exit->lock);
        
        int y = 0;
        while (y < (sizeof(path)/sizeof(int))) {
            pthread_mutex_unlock(&isection.quad[path[y]]);
            y++;
        }
        
        // Free up the path for the next loop.
        free(path);
        
    }
    
}

/**
 * TODO: Fill in this function
 *
 * Given a car's in_dir and out_dir return a sorted 
 * list of the quadrants the car will pass through.
 * 
 */
int *compute_path(enum direction in_dir, enum direction out_dir) {
    
    // Check whether the directions are valid or not.
    // If not, then handle with an exit code of 1.
    // Not essential, but good practice.
    
    if (!((in_dir == NORTH || in_dir == SOUTH || in_dir == EAST || in_dir == WEST) && (out_dir == NORTH || out_dir == SOUTH || out_dir == EAST || out_dir == WEST))) {
        fprintf(stderr, "Invalid in_dir.\n");
        exit(1);
    }
    
    int *route = malloc(4 * sizeof(int));
    
    // Using a pattern of the integers 0, 1, 2 & 3 for route allocation.
    // With this quadrant map:
    //
    //                 North
    //         ----------------------
    //        |          |           |
    //        |    1     |     0     |
    //        |          |           |
    //  West   ----------------------   East
    //        |          |           |
    //        |    2     |     3     |
    //        |          |           |
    //         ----------------------
    //                 South
    //
    // Note that the cars are always driving forward in the right lane.
    // Also, the following route arrays produced are sorted into ascending order.
    
    switch (in_dir) {
            
        case NORTH:
            
            switch (out_dir) {
                    
                case NORTH:
                    route[0] = 0;
                    route[1] = 1;
                    route[2] = 2;
                    route[3] = 3;
                    
                case EAST:
                    route[0] = 1;
                    route[1] = 2;
                    route[2] = 3;
                    
                case SOUTH:
                    route[0] = 1;
                    route[1] = 2;
                    
                case WEST:
                    route[0] = 1;
                    
                default:
                    break;
                    
            }
            
        case EAST:
            
            switch (out_dir) {
                    
                case EAST:
                    route[0] = 0;
                    route[1] = 1;
                    route[2] = 2;
                    route[3] = 3;
                    
                case SOUTH:
                    route[0] = 0;
                    route[1] = 1;
                    route[2] = 2;
                    
                case WEST:
                    route[0] = 0;
                    route[1] = 1;
                    
                case NORTH:
                    route[0] = 0;
                    
                default:
                    break;
                    
            }
            
        case SOUTH:
            
            switch (out_dir) {
                    
                case SOUTH:
                    route[0] = 0;
                    route[1] = 1;
                    route[2] = 2;
                    route[3] = 3;
                    
                case WEST:
                    route[0] = 0;
                    route[1] = 1;
                    route[2] = 3;
                    
                case NORTH:
                    route[0] = 0;
                    route[1] = 3;
                    
                case EAST:
                    route[0] = 3;
                    
                default:
                    break;
                    
            }
            
        case WEST:
            
            switch (out_dir) {
                    
                case WEST:
                    route[0] = 0;
                    route[1] = 1;
                    route[2] = 2;
                    route[3] = 3;
                    
                case NORTH:
                    route[0] = 0;
                    route[1] = 2;
                    route[2] = 3;
                    
                case EAST:
                    route[0] = 2;
                    route[1] = 3;
                    
                case SOUTH:
                    route[0] = 2;
                    
                default:
                    break;
                    
            }
            
        default:
            break;
            
    }
    
    return route;

}
