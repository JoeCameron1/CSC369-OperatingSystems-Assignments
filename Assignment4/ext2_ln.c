#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "ext2_utils.h"

unsigned char *disk = NULL;

int main(int argc, char *argv[]) {
    
    if (argc < 4 || argc > 5) {
        fprintf(stderr, "Usage: %s <image file name> [-s] <target> <link name>\n", argv[0]);
        exit(1);
    }
    
    unsigned char target_file_type = EXT2_FT_REG_FILE;
    char *disk_image_path = argv[1];
    char *source_path;
    char *link_path;
    
    // Check if flag for symoblic link is present
    if (argc == 5) {
        char *flag = argv[2];
        
        if (flag[1] != 's') {
            fprintf(stderr, "Usage: %s <image file name> [-s] <target> <link name>\n", argv[0]);
            exit(1);
        }
        target_file_type = EXT2_FT_SYMLINK;
        source_path = argv[3];
        link_path = argv[4];
    }
    else {
        source_path = argv[2];
        link_path = argv[3];
    }
    
    disk = read_disk_image(disk_image_path);
    
    struct ext2_dir_entry *src_dir_entry = find_dir_entry(source_path);
    
    if (src_dir_entry == NULL) {
        return ENOENT;
    }
    
    // Don't allow hard links to directories
    if (src_dir_entry->file_type == EXT2_FT_DIR &&
        target_file_type != EXT2_FT_SYMLINK) {
        
        return EISDIR;
    }
    
    // Use src file name if no name has been provided for target
    unsigned int src_file_name_len = src_dir_entry->name_len + 1;
    char src_file_name[src_file_name_len];
    
    strncpy(src_file_name, src_dir_entry->name, src_file_name_len);
    src_file_name[src_file_name_len - 1] = '\0';
    
    struct ext2_dir_entry *link;
    
    // Symbolic link
    if (target_file_type == EXT2_FT_SYMLINK) {
        link = ln_create_target_file(link_path, src_file_name, 0, target_file_type);
        
        copy_symlink_path(link, source_path);
    }
    
    // Hard link
    else {
        link = ln_create_target_file(link_path, src_file_name,
                                     src_dir_entry->inode, target_file_type);
    }
    
    return 0;
    
}
