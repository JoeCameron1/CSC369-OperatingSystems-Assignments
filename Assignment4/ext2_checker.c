#include <stdio.h>
#include <stdlib.h>
#include "ext2_utils.h"

#define FILE_TYPE(I_MODE) ((I_MODE >> 12))
#define IS_DIR(I_MODE) ((I_MODE >> 12) == (EXT2_S_IFDIR >> 12))

unsigned char *disk = NULL;

/*
 * Ensure that the superblock and block group counters for free inodes
 * are consistent with the inode bitmap.
 *
 * Return the sum of the absolute differences between the bitmap and the
 * superblock and block group counters.
 */
unsigned int fix_free_inodes_count() {
    
    unsigned char *inode_bitmap = get_inode_bitmap();
    int bitmap_size = get_inodes_count();
    
    int num_low_bits = 0;
    
    int i = 0;
    while (i < bitmap_size) {
        unsigned char curr_byte = inode_bitmap[i];
        int curr_bit = 0;
        while (curr_bit < 8) {
            if (!IS_IN_USE(curr_byte, curr_bit)) {
                num_low_bits++;
            }
            curr_bit = curr_bit + 1;
        }
        i = i + 1;
    }
    
    int num_free_inodes = num_low_bits;
    
    struct ext2_super_block *sb = (struct ext2_super_block *)(disk + EXT2_BLOCK_SIZE);
    struct ext2_group_desc *gd = get_group_descriptor();
    
    int delta_with_super_blk = abs(num_free_inodes - (int) sb->s_free_inodes_count);
    
    int delta_with_grp_desc = abs(num_free_inodes - (int) gd->bg_free_inodes_count);
    
    if (delta_with_super_blk != 0) {
        sb->s_free_inodes_count = num_free_inodes;
        printf("Fixed: superblock's free inodes counter was off by %d compared to the bitmap\n", delta_with_super_blk);
    }
    
    if (delta_with_grp_desc != 0) {
        gd->bg_free_inodes_count = num_free_inodes;
        printf("Fixed: block group's free inodes counter was off by %d compared to the bitmap\n", delta_with_grp_desc);
    }
    
    return delta_with_super_blk + delta_with_grp_desc;
}

/*
 * Ensure that the superblock and block group counters for free blocks
 * are consistent with the block bitmap.
 *
 * Return the sum of the absolute differences between the bitmap and the
 * superblock and block group counters.
 */
unsigned int fix_free_blocks_count() {
    
    unsigned char *block_bitmap = get_block_bitmap();
    int bitmap_size = get_blocks_count();
    
    int num_low_bits = 0;
    
    int i = 0;
    while (i < bitmap_size) {
        unsigned char curr_byte = block_bitmap[i];
        int curr_bit = 0;
        while (curr_bit < 8) {
            if (!IS_IN_USE(curr_byte, curr_bit)) {
                num_low_bits++;
            }
            curr_bit = curr_bit + 1;
        }
        i = i + 1;
    }
    
    int num_free_blocks = num_low_bits;
    
    struct ext2_super_block *sb = (struct ext2_super_block *)(disk + EXT2_BLOCK_SIZE);
    struct ext2_group_desc *gd = get_group_descriptor();
    
    int delta_with_super_blk = abs(num_free_blocks - (int) sb->s_free_blocks_count);
    
    int delta_with_grp_desc = abs(num_free_blocks - (int) gd->bg_free_blocks_count);
    
    if (delta_with_super_blk != 0) {
        sb->s_free_blocks_count = num_free_blocks;
        printf("Fixed: superblock's free blocks counter was off by %d compared to the bitmap\n", delta_with_super_blk);
    }
    
    if (delta_with_grp_desc != 0) {
        gd->bg_free_blocks_count = num_free_blocks;
        printf("Fixed: block group's free blocks counter was off by %d compared to the bitmap\n", delta_with_grp_desc);
    }
    
    return delta_with_super_blk + delta_with_grp_desc;
}

/*
 * Return the equivalent file_type for a directory entry,
 * based on the given i_mode.
 */
unsigned char dir_entry_file_type(unsigned short i_mode) {
    
    if (FILE_TYPE(i_mode) == FILE_TYPE(EXT2_S_IFDIR)) {
        return EXT2_FT_DIR;
    } else if (FILE_TYPE(i_mode) == FILE_TYPE(EXT2_S_IFREG)) {
        return EXT2_FT_REG_FILE;
    } else if (FILE_TYPE(i_mode) == FILE_TYPE(EXT2_S_IFLNK)) {
        return EXT2_FT_SYMLINK;
    } else {
        return EXT2_FT_UNKNOWN;
    }
    
}

/*
 * Ensure that the given directory entry's file_type matches the i_mode
 * of its inode.
 * If not, fix the direcotry entry's file_type to match the i_mode.
 *
 * Return 0, if they were already consistent.
 * Return 1, otherwise.
 */
int fix_file_type_mismatch(struct ext2_dir_entry *entry) {
    int fixed_quantity = 0;
    struct ext2_inode *inode = get_inode_table() + INDEX(entry->inode);
    
    unsigned char expected = dir_entry_file_type(inode->i_mode);
    
    if (entry->file_type != expected) {
        entry->file_type = expected;
        fixed_quantity = fixed_quantity + 1;
        printf("Fixed: Entry type vs inode mismatch: inode [%d]\n", entry->inode);
    }
    
    return fixed_quantity;
}

/*
 * Ensure that the given inode is marked as 'in-use' in the inode bitmap.
 * If not, mark it as 'in-use'.
 *
 * Return 0 if it was already marked 'in-use'.
 * Return 1 otherwise.
 */
int fix_inode_allocation_inconsistency(unsigned int inode_num) {
    
    int fixed_quantity = 0;
    
    if (!is_inode_in_use(inode_num)) {
        set_inode_in_use(inode_num);
        fixed_quantity = fixed_quantity + 1;
        printf("Fixed: inode [%d] not marked as in-use\n", inode_num);
    }
    
    return fixed_quantity;
    
}

/*
 * Zero out the deletion time of this inode, if it is not already zeroed out.
 *
 * Returns 0 if it was already zeroed out.
 * Returns 1 otherwise.
 */
int fix_inode_deletion_time(unsigned int inode_num) {
    
    int fixed_quantity = 0;
    
    struct ext2_inode *inode = get_inode_table() + INDEX(inode_num);
    
    if (inode->i_dtime) {
        inode->i_dtime = 0;
        fixed_quantity = fixed_quantity + 1;
        printf("Fixed: valid inode marked for deletion: [%d]\n", inode_num);
    }
    
    return fixed_quantity;
    
}

/*
 * Checks if all data blocks pointed by the given inode is marked as in-use.
 * If not, it marks the appropriate ones as in-use and updates the
 * superblock and block group counters for free blocks.
 *
 * Returns the total number of blocks that it marked as 'in-use'.
 */
int fix_data_block_allocation(unsigned int inode_num) {
    int n, num_fixed = 0;
    
    struct ext2_inode *inode = get_inode_table() + INDEX(inode_num);
    
    for (n = 0; n < 12; n++) {
        unsigned int block_num = inode->i_block[n];
        
        if (block_num != 0 && !is_block_in_use(block_num)) {
            set_block_in_use(block_num);
            num_fixed++;
        }
    }
    
    unsigned int indirect_block_num = inode->i_block[n];
    
    if (indirect_block_num != 0) {
        
        if (!is_block_in_use(indirect_block_num)) {
            set_block_in_use(indirect_block_num);
            num_fixed++;
        }
        
        int max_indirect_blocks = EXT2_BLOCK_SIZE / sizeof(unsigned int);
        
        unsigned int *position = (unsigned int *) BLOCK_START(disk, indirect_block_num);
        
        unsigned int *block_end = position + max_indirect_blocks;
        
        unsigned int direct_block_num = *(position++);
        
        while (position < block_end) {
            if ((direct_block_num != 0) && (!is_block_in_use(direct_block_num))) {
                set_block_in_use(direct_block_num);
                num_fixed++;
            }
            direct_block_num = *(position++);
        }
    }
    
    if (num_fixed) {
        printf("Fixed: %d in-use data blocks not marked in data bitmap for inode: [%d]\n", num_fixed, inode_num);
    }
    
    return num_fixed;
}

/*
 * Recursively checks the file system for inconsistencies, and takes
 * appropriate measures to fix them.
 *
 * Returns the total number of inconsistencies fixed.
 */
int fix_dir_entries_recursively(struct ext2_dir_entry *entry, int is_root) {
    
    struct ext2_inode *inode = get_inode_table() + INDEX(entry->inode);
    
    int num_fixed = fix_file_type_mismatch(entry) + fix_inode_allocation_inconsistency(entry->inode) + fix_inode_deletion_time(entry->inode) + fix_data_block_allocation(entry->inode);
    
    // If entry is not a directory, then return.
    if (entry->file_type != EXT2_FT_DIR) {
        return num_fixed;
    }
    
    // Stop recursing if this dir entry is not the root, and refers to itself
    // or its parent
    else if (!is_root && (strncmp(entry->name, ".", strlen(".")) == 0 || strncmp(entry->name, "..", strlen("..")) == 0)) {
        return num_fixed;
    }
    
    int n = 0;
    while ((n < 12) && ((inode->i_block)[n] != 0)) {
        
        int block_num = (inode->i_block)[n];
        
        unsigned char *block_start = BLOCK_START(disk, block_num);
        unsigned char *block_end = BLOCK_END(block_start);
        unsigned char *position = block_start;
        
        while (position != block_end) {
            struct ext2_dir_entry *curr_entry = (struct ext2_dir_entry *) position;
            
            if (curr_entry->inode != 0) {
                num_fixed += fix_dir_entries_recursively(curr_entry, FALSE);
            }
            
            position = position + curr_entry->rec_len;
        }
        n = n + 1;
    }
    
    return num_fixed;
}


int main(int argc, char *argv[]) {
    
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <image file name>\n", argv[0]);
        exit(1);
    }
    
    char *disk_image_path = argv[1];
    
    disk = read_disk_image(disk_image_path);
    
    unsigned int fixed_inconsistencies = 0;
    
    struct ext2_inode *root_dir_inode = get_inode_table() + EXT2_ROOT_INO_IDX;
    
    // If the root inode doesn't have a type of 'directory', then fix it.
    if (!IS_DIR(root_dir_inode->i_mode)) {
        root_dir_inode->i_mode = root_dir_inode->i_mode | EXT2_S_IFDIR;
        fixed_inconsistencies = fixed_inconsistencies + 1;
        printf("Fixed: Root inode not marked as directory\n");
    }
    
    struct ext2_dir_entry *root_entry = (struct ext2_dir_entry *)
    BLOCK_START(disk, root_dir_inode->i_block[0]);
    
    // This is the total number of inconsistencies fixed.
    fixed_inconsistencies = fixed_inconsistencies + fix_free_inodes_count() + fix_free_blocks_count() + fix_dir_entries_recursively(root_entry, TRUE);
    
    if (fixed_inconsistencies) {
        printf("%d file system inconsistencies repaired!\n", fixed_inconsistencies);
    } else {
        printf("No file system inconsistencies detected!\n");
    }
    
    return 0;
    
}
