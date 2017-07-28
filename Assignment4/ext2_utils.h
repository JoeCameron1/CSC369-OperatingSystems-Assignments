#include <string.h>
#include "ext2.h"

#define IS_PATH_ABSOLUTE(PATH) ((PATH[0] == '/'))
#define TRUE 1
#define FALSE 0
#define NUM_DISK_BLKS(CURR, DELTA) (((CURR * 512) + (DELTA)) / 512)
#define INDEX(NUM) ((NUM) - 1)
#define NUM(INDEX) ((INDEX) + 1)
#define BLOCK_START(DISK, BLOCK_NUM) ((DISK) + (BLOCK_NUM) * EXT2_BLOCK_SIZE)
#define BLOCK_END(BLOCK_PTR) ((BLOCK_PTR) + EXT2_BLOCK_SIZE)
#define IS_IN_USE(BYTE, BIT) ((BYTE) & (1 << BIT))

// **************************************************************************
// General-Use Functions

unsigned char *read_disk_image(char *path);
struct ext2_group_desc *get_group_descriptor();
struct ext2_inode *get_inode_table();
unsigned char *get_block_bitmap();
unsigned char *get_inode_bitmap();
int get_blocks_count();
int get_inodes_count();
int allocate_block();
void set_inode_in_use(unsigned int inode_num);
void set_block_in_use(unsigned int block_num);
int is_inode_in_use(unsigned int inode_num);
int is_block_in_use(unsigned int block_num);
void unlink_inode(unsigned int inode_num);
int get_name_len(char *name);
char *get_file_name(char *path);
int get_actual_dir_entry_len(struct ext2_dir_entry *entry);
struct ext2_dir_entry *find_entry(struct ext2_inode *dir_inode, char *name);
struct ext2_dir_entry *find_entry_in_inode(unsigned int inode_num, char *name);
struct ext2_dir_entry *create_dir_entry(struct ext2_inode *dir_inode, unsigned int link_inode, char *name, unsigned char file_type);

// **************************************************************************

// **************************************************************************
// Functions only used in the ext2_cp file.

int is_regular_file(const char *path);
FILE *open_file(char *path);
struct ext2_dir_entry *cp_create_target_file(char *path, char *name);
void copy_sourcefile_data(struct ext2_inode *inode, FILE *src);

// **************************************************************************
// Functions only used in the ext2_ln file.

int path_terminator_valid(char *path, struct ext2_dir_entry *entry);
struct ext2_dir_entry *find_dir_entry(char *path);
struct ext2_dir_entry *ln_create_target_file(char *path, char *name, unsigned int link_inode, unsigned char file_type);
void copy_symlink_path(struct ext2_dir_entry *dir_entry, char *path);

// **************************************************************************
