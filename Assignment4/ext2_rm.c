#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "ext2_utils.h"

unsigned char *disk = NULL;

/*
 * Returns the directory entry for the directory that contains the file
 * with the given `file_path`.
 */
struct ext2_dir_entry *find_container_directory(char *file_path) {
    
    // Length of path (including null byte)
    int path_len = strlen(file_path) + 1;
    
    char tokenized_path[path_len];
    strncpy(tokenized_path, file_path, path_len);
    
    // The name of the current dir / file / link in the path
    char *token = strtok(tokenized_path, "/");
    
    unsigned int root_inode_num = NUM(EXT2_ROOT_INO_IDX);
    
    // Container (or 'parent') directory of curr_dir_entry
    struct ext2_dir_entry *container_dir = find_entry_in_inode(root_inode_num, ".");
    
    // The current directory entry in path
    struct ext2_dir_entry *curr_dir_entry = find_entry_in_inode(container_dir->inode, token);
    
    while (token != NULL) {
        char *next_token = strtok(NULL, "/");
        
        // 'file_path' refers to a non-existent entry
        if (curr_dir_entry == NULL) {
            printf("Error: No such file to remove.\n");
            exit(ENOENT);
        }
        
        // Reached end of path
        else if (next_token == NULL) {
            return container_dir;
        }
        
        // One or more entries in th path is not a directory
        else if (curr_dir_entry->file_type != EXT2_FT_DIR) {
            printf("Error: The absolute path contains files.\n");
            exit(ENOENT);
        }
        
        token = next_token;
        container_dir = curr_dir_entry;
        curr_dir_entry = find_entry_in_inode(curr_dir_entry->inode, token);
    }
    
    return container_dir;
}

/*
 * Deletes (i.e. hides) the directory entry for the file with the
 * given `name` that resides inside the directory with inode `dir_inode_num`.
 *
 * Returns EXIT_SUCCESS, if the file was successfully deleted.
 *               ENOENT, if the file does not exist.
 *               EISDIR, if the given `path` refers to a directory.
 */
int delete_file_entry(unsigned int dir_inode_num, char *name) {
    
    struct ext2_inode *dir_inode = get_inode_table() + INDEX(dir_inode_num);
    
    int name_length = get_name_len(name);
    
    // Iterate over data blocks in search for the matching directory entry
    int n;
    for (n = 0; n < 12 && (dir_inode->i_block)[n] != 0; n++) {
        
        int block_num = (dir_inode->i_block)[n];
        
        unsigned char *block_start = BLOCK_START(disk, block_num);
        unsigned char *block_end = BLOCK_END(block_start);
        
        // Current position within this block
        unsigned char *pos = block_start;
        
        // The previous entry within the block
        struct ext2_dir_entry *prev_entry = NULL;
        
        while (pos != block_end) {
            struct ext2_dir_entry *entry = (struct ext2_dir_entry *) pos;
            
            // Dir entry is in use
            if (entry->inode != 0) {
                
                // Check if length of file names match
                if (name_length == ((int) entry->name_len)) {
                    
                    // File names match
                    if (strncmp(name, entry->name, name_length) == 0) {
                        
                        // Do not allow deletion of directories
                        if (entry->file_type == EXT2_FT_DIR) {
                            printf("Error: Cannot delete a directory.\n");
                            return EISDIR;
                        }
                        
                        // Delete the entry
                        // Unlink inode (and free blocks if inode has no more links)
                        unlink_inode(entry->inode);
                        
                        // Set inode to 0 if 'entry' is the first entry in the block
                        if (prev_entry == NULL) {
                            entry->inode = 0;
                        }
                        
                        // Otherwise, make the previous entry point to wherever the deleted entry
                        // was pointing to
                        else {
                            prev_entry->rec_len += entry->rec_len;
                        }
                        
                        return 0;
                    }
                }
                pos += entry->rec_len;
            }
            
            // Dir entry is not in use. Move to next entry
            else if (entry->rec_len > 0) {
                pos += entry->rec_len;
            }
            
            // Dir entry is blank / zeroed out / has invalid rec_len
            else {
                // Skip to end of block
                pos = block_end;
            }
            
            prev_entry = entry;
        }
    }
    
    // No matching dir entry was found
    printf("Error: No such file to remove.\n");
    return ENOENT;
}


int main(int argc, char *argv[]) {
    
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <image file name> <absolute path on ext2 image>\n", argv[0]);
        exit(1);
    }
    
    char *disk_image_path = argv[1];
    char *file_path = argv[2];
    
    disk = read_disk_image(disk_image_path);
    
    // Ensure that file_path starts with a '/'
    if (!IS_PATH_ABSOLUTE(file_path)) {
        printf("Error: Please give an absolute path. Perhaps include '/' at the start of the path.\n");
        return ENOENT;
    }
    
    // The directory which contains the file to delete
    struct ext2_dir_entry *container_dir = find_container_directory(file_path);
    
    // Don't accept directory paths
    if (file_path[strlen(file_path) - 1] == '/') {
        printf("Error: Absolute path must end with a valid filename.\n");
        return EISDIR;
    }
    
    char *file_name = get_file_name(file_path);
    
    // No filname specified implies, it refers to the current dir
    if (file_name == NULL) {
        file_name = ".";
    }
    
    return delete_file_entry(container_dir->inode, file_name);
    
}
