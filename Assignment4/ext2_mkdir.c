#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "ext2_utils.h"

unsigned char *disk = NULL;

int main(int argc, char *argv[]) {
    
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <image file name> <absolute path on ext2 image>\n", argv[0]);
        exit(1);
    }
    
    if (strcmp("/", argv[2]) == 0) {
        printf("Error: You cannot make the root directory, as the root directory already exists.\n");
        return EEXIST;
    }
    
    char *disk_image_path = argv[1];
    char *target_path = argv[2];
    
    disk = read_disk_image(disk_image_path);
    
    // Ensure that file_path starts with a '/'
    if (!IS_PATH_ABSOLUTE(target_path)) {
        printf("Error: You must specify an absolute path.\n");
        return ENOENT;
    }
    
    struct ext2_inode *i_table = get_inode_table();
    
    // The inode of the current directory
    unsigned int curr_inode_num = NUM(EXT2_ROOT_INO_IDX);
    struct ext2_inode *curr_inode = i_table + EXT2_ROOT_INO_IDX;
    
    char *token = strtok(target_path, "/");
    
    // Current Directory Entry in path
    struct ext2_dir_entry *curr_dir_entry = find_entry(curr_inode, token);
    
    while (token != NULL) {
        char *next_token = strtok(NULL, "/");
        
        if (curr_dir_entry == NULL) {
            
            // Path does not exist
            if (next_token != NULL) {
                printf("Error: The absolute path you specified does not exist.\n");
                return ENOENT;
            }
            
            // Everything good, create new directory
            struct ext2_dir_entry *new_entry =
            create_dir_entry(curr_inode, 0, token, EXT2_FT_DIR);
            
            get_group_descriptor()->bg_used_dirs_count++;
            
            // Create entry for self (.) inside new directory
            struct ext2_inode *new_inode = i_table + INDEX(new_entry->inode);
            create_dir_entry(new_inode, new_entry->inode, ".", EXT2_FT_DIR);
            
            // Create entry for parent directory (..) inside new directory
            create_dir_entry(new_inode, curr_inode_num, "..", EXT2_FT_DIR);
            
            return 0;
        }
        
        // Reached end of path. Target already exists
        else if (next_token == NULL) {
            printf("Error: Directory already exists.\n");
            return EEXIST;
        }
        
        // One or more entries in the path is not a directory.
        else if (curr_dir_entry->file_type != EXT2_FT_DIR) {
            printf("Error: The absolute path contains a non-directory type.\n");
            return ENOENT;
        }
        
        // Everything good, continue traversing path
        token = next_token;
        
        curr_inode_num = curr_dir_entry->inode;
        curr_inode =  i_table + INDEX(curr_inode_num);
        curr_dir_entry = find_entry(curr_inode, token);
    }
    
    // Path does not exist
    return ENOENT;
    
}
