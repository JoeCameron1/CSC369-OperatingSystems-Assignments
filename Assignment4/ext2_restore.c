#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "ext2_utils.h"

unsigned char *disk = NULL;

/*
 * Returns the directory entry for the directory that contains
 * the given file.
 */
struct ext2_dir_entry *find_container_directory(char *file_path) {
    
    // Ensure that file_path starts with a '/'
    if (!IS_PATH_ABSOLUTE(file_path)) {
        exit(ENOENT);
    }
    
    // Length of path (including null byte)
    int path_len = strlen(file_path) + 1;
    
    char tokenized_path[path_len];
    strncpy(tokenized_path, file_path, path_len);
    
    // The name of the current dir / file / link in the path
    char *token = strtok(tokenized_path, "/");
    
    unsigned int root_inode_num = NUM(EXT2_ROOT_INO_IDX);
    
    // Container (or 'parent') directory of curr_dir_entry
    struct ext2_dir_entry *container_dir =
    find_entry_in_inode(root_inode_num, ".");
    
    // The current directory entry in path
    struct ext2_dir_entry *curr_dir_entry =
    find_entry_in_inode(container_dir->inode, token);
    
    while (token != NULL) {
        char *next_token = strtok(NULL, "/");
        
        // Reached end of path
        if (next_token == NULL) {
            return container_dir;
        }
        
        // 'file_path' refers to a non-existent entry
        else if (curr_dir_entry == NULL) {
            exit(ENOENT);
        }
        
        // One or more entries in th path is not a directory
        else if (curr_dir_entry->file_type != EXT2_FT_DIR) {
            exit(ENOENT);
        }
        
        token = next_token;
        container_dir = curr_dir_entry;
        curr_dir_entry = find_entry_in_inode(curr_dir_entry->inode, token);
    }
    
    return container_dir;
}

/*
 * Returns TRUE if the given inode and all of the data blocks it points to,
 * is not marked as 'in-use' in the corresponding bitmaps.
 *
 * Returns FALSE, otherwise.
 */
int is_inode_and_data_blocks_free(unsigned int inode_num) {
    if (is_inode_in_use(inode_num)) {
        return FALSE;
    }
    
    struct ext2_inode *inode = get_inode_table() + INDEX(inode_num);
    
    // Check if all of the direct data blocks pointed by this inode is unused
    int n;
    for (n = 0; n < 12; n++) {
        int block_num = inode->i_block[n];
        
        if (block_num == 0) {
            return TRUE;
        }
        else if (is_block_in_use(block_num)) {
            return FALSE;
        }
    }
    
    int indirect_block_num = inode->i_block[n];
    
    if (indirect_block_num != 0) {
        
        // Check if the indirect block is free
        if (is_block_in_use(indirect_block_num)) {
            return FALSE;
        }
        
        // Check if the direct blocks pointed by the indirect block are free
        
        int max_indirect_blocks = EXT2_BLOCK_SIZE / sizeof(unsigned int);
        int blocks_checked = 0;
        
        // Position within indirect block
        unsigned int *pos = (unsigned int *)
        BLOCK_START(disk, indirect_block_num);
        
        // Free 2nd level blocks pointed by the indirect block
        unsigned int direct_block_num = *(pos++);
        
        while (blocks_checked < max_indirect_blocks
               && direct_block_num != 0) {
            
            if (is_block_in_use(direct_block_num)) {
                return FALSE;
            }
            direct_block_num = *(pos++);
            blocks_checked++;
        }
    }
    
    // None of the blocks (and the inode itself) have been reallocated.
    // All blocks are 'safe' to be reclaimed
    return TRUE;
}

/*
 * Reacquires the given inode and all of its data blocks, if they are not
 * already in use.
 *
 * Returns EXIT_SUCCESS if they were successfully acquired.
 * Returns ENOENT, otherwise.
 */
int reclaim_inode_and_data_blocks(unsigned int inode_num) {
    
    // Return error if the inode and all of its
    // pointed blocks are not free
    if (!is_inode_and_data_blocks_free(inode_num)) {
        return ENOENT;
    }
    
    struct ext2_inode *inode = get_inode_table() + INDEX(inode_num);
    
    // Reclaim the inode
    set_inode_in_use(inode_num);
    inode->i_links_count = 1;
    inode->i_dtime = 0;
    
    // Reclaim all the direct data blocks pointed by this inode
    int n;
    for (n = 0; n < 12; n++) {
        int block_num = inode->i_block[n];
        
        if (block_num == 0) {
            return 0;
        }
        
        set_block_in_use(block_num);
    }
    
    int indirect_block_num = inode->i_block[n];
    
    if (indirect_block_num != 0) {
        
        set_block_in_use(indirect_block_num);
        
        // Check if the direct blocks pointed by the indirect block are free
        
        int max_indirect_blocks = EXT2_BLOCK_SIZE / sizeof(unsigned int);
        int blocks_claimed = 0;
        
        // Position within indirect block
        unsigned int *pos = (unsigned int *)
        BLOCK_START(disk, indirect_block_num);
        
        // Free 2nd level blocks pointed by the indirect block
        unsigned int direct_block_num = *(pos++);
        
        while (blocks_claimed < max_indirect_blocks && direct_block_num != 0) {
            
            set_block_in_use(direct_block_num);
            direct_block_num = *(pos++);
            blocks_claimed++;
        }
    }
    
    return 0;
}

/*
 * Unhides the given `entry` in the direcotry block, by adjusting
 * the record lengths (rec_len) of its previous entry and itself.
 */
void unhide_deleted_entry(struct ext2_dir_entry *entry,
                          struct ext2_dir_entry *prev_entry) {
    
    int distance = ((unsigned char *) entry) - ((unsigned char *) prev_entry);
    
    // Make the restored entry point to the next valid entry
    entry->rec_len = prev_entry->rec_len - distance;
    
    // Make previous entry point to the restored entry
    prev_entry->rec_len = distance;
}

/*
 * Restores the file with the given `name` that is contained within the
 * directory with the given inode.
 *
 * Returns: EXIT_SUCCESS, if the file was successfully restored
 *                EEXIST, if the file / direcotry already exists
 *                ENOENT, if the file cannot be restored
 *                EISDIR, if the 'file' is a deleted directory
 */
int restore_file(unsigned int dir_inode_num, char *name) {
    
    struct ext2_inode *dir_inode = get_inode_table() + INDEX(dir_inode_num);
    int name_length = get_name_len(name);
    
    // Check if the file already exists
    if (find_entry(dir_inode, name) != NULL) {
        return EEXIST;
    }
    
    // Iterate over data blocks in search for the matching directory entry
    int n;
    for (n = 0; n < 12 && (dir_inode->i_block)[n] != 0; n++) {
        
        int block_num = (dir_inode->i_block)[n];
        
        unsigned char *block_start = BLOCK_START(disk, block_num);
        unsigned char *block_end = BLOCK_END(block_start);
        
        // Current position within this block
        unsigned char *pos = block_start;
        
        /* Note: The first entry in a block cannot be restored since its inode
         *       is set to 0 while deletion
         */
        
        struct ext2_dir_entry *last_valid_entry= (struct ext2_dir_entry *)pos;
        
        while (pos < block_end) {
            unsigned char *gap_start =
            pos + get_actual_dir_entry_len(last_valid_entry);
            
            unsigned char *gap_end = pos + last_valid_entry->rec_len;
            
            pos = gap_start;
            
            while (pos < gap_end) {
                
                // Potential deleted entry
                struct ext2_dir_entry *entry = (struct ext2_dir_entry *) pos;
                
                // Try to skip to the next entry if this inode is zeroed out
                if (entry->inode == 0) {
                    
                    if (entry->name_len == 0) {
                        
                        // No way to find the next entry in the gap, jump to
                        // the end of the gap
                        if (entry->rec_len == 0) {
                            pos = gap_end;
                        }
                        else {
                            pos += entry->rec_len;
                        }
                    }
                    else {
                        pos += get_actual_dir_entry_len(entry);
                    }
                }
                else {
                    // Check if this is the file we are trying to recover
                    if (name_length == entry->name_len &&
                        strncmp(name, entry->name, name_length) == 0) {
                        
                        // Do not allow restore of directories
                        if (entry->file_type == EXT2_FT_DIR) {
                            return EISDIR;
                        }
                        
                        // Found the correct entry, reclaim inode and
                        // data blocks, if possible
                        int result = reclaim_inode_and_data_blocks(entry->inode);
                        
                        if (result == 0) {
                            // Adjust directory entry pointers to 'unhide' the
                            // deleted entry
                            unhide_deleted_entry(entry, last_valid_entry);
                        }
                        
                        return result;
                    }
                    
                    // Skip to the next entry
                    else {
                        pos += get_actual_dir_entry_len(entry);
                    }
                }
            }
            
            // Sorry for the ugly expression. This is the best I could do
            // while trying to keep the variable names explicit enough
            last_valid_entry = (struct ext2_dir_entry *)
            (((unsigned char *) last_valid_entry) +
             last_valid_entry->rec_len);
        }
    }
    
    // No matching dir entry was found
    return ENOENT;
}

/*
 * Restores the file with the given `path`.
 *
 * Returns: EXIT_SUCCESS, if the file was successfully restored
 *                EEXIST, if the file / direcotry already exists
 *                ENOENT, if the file cannot be restored
 *                EISDIR, if the 'file' is a deleted directory
 */
int restore(char *path) {
    // The directory which contains the file to delete
    struct ext2_dir_entry *container_dir = find_container_directory(path);
    
    // Don't accept directory paths
    if (path[strlen(path) - 1] == '/') {
        return EISDIR;
    }
    
    char *file_name = get_file_name(path);
    
    // No filname specified implies, it refers to the current dir
    if (file_name == NULL) {
        file_name = ".";
    }
    
    return restore_file(container_dir->inode, file_name);
}


int main(int argc, char *argv[]) {
    
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <image file name> <absolute path on ext2 image>\n", argv[0]);
        exit(1);
    }
    
    char *disk_image_path = argv[1];
    char *file_path = argv[2];
    
    disk = read_disk_image(disk_image_path);
    
    return restore(file_path);
    
}
