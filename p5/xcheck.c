#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

typedef unsigned char  uchar;
#define BLOCK_SIZE (512)
#define NUM_FUNC 12
#define BOFFSET(block_addr) (mapped_img + (block_addr)*BLOCK_SIZE)
#define T_DIR  1   // Directory
#define T_FILE 2   // File
#define T_DEV  3   // Special device

/*==========fs.h==========*/

// File system super block
struct superblock {
  uint size;         // Size of file system image (blocks)
  uint nblocks;      // Number of data blocks
  uint ninodes;      // Number of inodes.
};

#define NDIRECT 12
#define NINDIRECT (BLOCK_SIZE / sizeof(uint))
#define MAXFILE (NDIRECT + NINDIRECT)

// On-disk inode structure
struct dinode {
  short type;           // File type
  short major;          // Major device number (T_DEV only)
  short minor;          // Minor device number (T_DEV only)
  short nlink;          // Number of links to inode in file system
  uint size;            // Size of file (bytes)
  uint addrs[NDIRECT+1];   // Data block addresses
};

// Inodes per block.
#define IPB           (BLOCK_SIZE / sizeof(struct dinode))

// Block containing inode i
#define IBLOCK(i)     ((i) / IPB + 2)

// Bitmap bits per block
#define BPB           (BLOCK_SIZE*8)

// Block containing bit for block b
#define BBLOCK(b, ninodes) (b/BPB + (ninodes)/IPB + 3)

// Directory is a file containing a sequence of dirent structures.
#define DIRSIZ 14

struct dirent {
  ushort inum;
  char name[DIRSIZ];
};
/*=========================*/


struct superblock* sb;
struct dinode* inodes;
uchar* bitmap;
int num_bitblocks;
uchar* datablocks;
int num_metablocks;
uchar* mapped_img;


/*1. Each inode is either unallocated or one of the valid types (T_FILE, T_DIR, T_DEV). 
 * If not, print ERROR: bad inode.*/
int check_inodes(){
    for (int i = 0; i < sb->ninodes; i++){
        short itype = inodes[i].type;
        if (itype != 0 && itype != T_FILE &&
                itype != T_DIR && itype != T_DEV){
            fprintf(stderr, "ERROR: bad inode.\n");
            return -1;
        }
    }
    return 0;
}

/*For in-use inodes, each address that is used by inode is valid (points to a valid datablock address within the image). If the direct block is used and is invalid, print ERROR: bad direct address in inode.; if the indirect block is in use and is invalid, print ERROR: bad indirect address in inode.*/
int check_addr_valid(){
    for (int i = 0; i < sb->ninodes; i++){
        if (inodes[i].type != 0){
            // check direct blocks
            for (int j = 0; j < NDIRECT; j++){
                uint dir_addr = inodes[i].addrs[j];
                if (dir_addr != 0 && !(dir_addr >= num_metablocks && dir_addr < sb->size)){
                    fprintf(stderr, "ERROR: bad direct address in inode.\n");
                    return -1;
                }
            }
            uint indir_addr = inodes[i].addrs[NDIRECT];
            if (indir_addr != 0 && !(indir_addr >= num_metablocks && indir_addr < sb->size)){
                fprintf(stderr, "ERROR: bad indirect address in inode.\n");
                return -1;
            }
            uint *dir_block = (uint*)BOFFSET(indir_addr);
            if (indir_addr != 0){
                for (int j = 0; j < NINDIRECT; j++){
                    uint dir_addr = dir_block[j];
                    if (dir_addr != 0 && !(dir_addr >= num_metablocks && dir_addr < sb->size)){
                        fprintf(stderr, "ERROR: bad indirect address in inode.\n");
                        return -1;
                    }
                }
            }
        }
    }
    return 0;
}

/*Root directory exists, its inode number is 1, and the parent of the root directory is itself. If not, print ERROR: root directory does not exist.*/
int check_root(){
    struct dinode iroot = inodes[1];
    
    // check root dir exists
    if (!(iroot.type == T_DIR)){
        fprintf(stderr, "ERROR: root directory does not exist.\n");
        return -1;
    }

    // check parent of root dir is itself
    uchar* dir_data = mapped_img + iroot.addrs[0]*BLOCK_SIZE;
    struct dirent *de = (struct dirent *)dir_data;
    de++;
    if (!(strcmp(de->name, "..") == 0 && de->inum == 1)){
        fprintf(stderr, "ERROR: root directory does not exist.\n");
        return -1;
    }
    return 0;    
}



int check_dir(){
    
    for (int i = 0; i < sb->ninodes; i++){
        struct dinode inode = inodes[i];
        short itype = inode.type;
        if (itype == T_DIR){
            uchar* dir_data = mapped_img + inode.addrs[0]*BLOCK_SIZE;
            struct dirent *de_cur = (struct dirent *)dir_data;
            if (!(strcmp(de_cur->name, ".") == 0 && de_cur->inum == i)){
                fprintf(stderr, "ERROR: directory not properly formatted.\n");
                return -1;
            }
            struct dirent *de_par = de_cur + 1;
            if (strcmp(de_par->name, "..") != 0){
                fprintf(stderr, "ERROR: directory not properly formatted.\n");
                return -1;
            }
        }
    }
    return 0;
}


int check_block_addr_in_use(uint addr){
     uint index_block = BBLOCK(addr, sb->ninodes);
     uchar *bitmap = mapped_img + index_block*BLOCK_SIZE;
     uint offset_block = addr % BPB;
     
     uint index_byte = offset_block/8;
     
     uint offset_byte = offset_block%8;
     if ((bitmap[index_byte] & (0x1 << offset_byte)) != 0){
         return 1;
     }
     return 0;
}

int check_bitmap_marked(){
    for (int i = 0; i < sb->ninodes; i++){
        struct dinode inode = inodes[i];
        short itype = inode.type;
        if (itype != 0){
            //if the inode is in use.
            for (int j = 0; j < NDIRECT; j++){
                uint dir_addr = inodes[i].addrs[j];
                if (dir_addr != 0 && check_block_addr_in_use(dir_addr)== 0){
                    fprintf(stderr, "ERROR: address used by inode but marked free in bitmap.\n");
                    return -1;
                }
            }
            uint indir_addr = inodes[i].addrs[NDIRECT];
            if (indir_addr != 0 && check_block_addr_in_use(indir_addr)== 0){
                fprintf(stderr, "ERROR: address used by inode but marked free in bitmap.\n");
                return -1;
            }
            uint *dir_block = (uint*)BOFFSET(indir_addr);
            if (indir_addr != 0){
                for (int j = 0; j < NINDIRECT; j++){
                   uint dir_addr = dir_block[j];
                   if (dir_addr != 0 && check_block_addr_in_use(dir_addr)== 0){
                       fprintf(stderr, "ERROR: address used by inode but marked free in bitmap.\n");
                       return -1;
                   }

                }
            }

        }
    }
    return 0;
}

int check_no_extra_bit(){
    uchar real_bitmap[num_bitblocks*BLOCK_SIZE];
    memcpy(real_bitmap, bitmap, num_bitblocks*BLOCK_SIZE);
    for (int i = 0; i < sb->ninodes; i++){
        struct dinode inode = inodes[i];
        short itype = inode.type;
        if (itype != 0){
            //if the inode is in use.
            for (int j = 0; j < NDIRECT; j++){
                uint dir_addr = inodes[i].addrs[j];
                if (dir_addr != 0){
                   uint byte_idx = dir_addr / 8, byte_off = dir_addr % 8;
                   real_bitmap[byte_idx] = real_bitmap[byte_idx] & ~(0x1 << byte_off);
                }
            }
            uint indir_addr = inodes[i].addrs[NDIRECT];
            uint *dir_block = (uint*)BOFFSET(indir_addr);
            if (indir_addr != 0){
                uint byte_idx = indir_addr / 8, byte_off = indir_addr % 8;
                real_bitmap[byte_idx] = real_bitmap[byte_idx] & ~(0x1 << byte_off);

                for (int j = 0; j < NINDIRECT; j++){
                   uint dir_addr = dir_block[j];
                   if (dir_addr != 0){
                       byte_idx = dir_addr / 8;
                       byte_off = dir_addr % 8;
                       real_bitmap[byte_idx] = real_bitmap[byte_idx] & ~(0x1 << byte_off);
                   }
                }
            }

        }
    }

    int half_idx = num_metablocks / 8, half_offset = num_metablocks % 8;
    for (int i = half_offset; i < 8; i++){
        if ((real_bitmap[half_idx] & (0x1 << half_offset)) != 0){
            fprintf(stderr, "ERROR: bitmap marks block in use but it is not in use.\n");
            return -1;
        }
    }

    for (int i = half_idx+1; i < num_bitblocks*BLOCK_SIZE; i++){
        if (real_bitmap[i] != 0){
            fprintf(stderr, "ERROR: bitmap marks block in use but it is not in use.\n");
            return -1;
        }
    }
    return 0;
}

int check_dir_dup(){
    uchar temp_bitmap[num_bitblocks*BLOCK_SIZE];
    for (int i = 0; i < sb->ninodes; i++){
        struct dinode inode = inodes[i];
        short itype = inode.type;
        if (itype != 0){
            //if the inode is in use.
            for (int j = 0; j < NDIRECT+1; j++){
                uint dir_addr = inodes[i].addrs[j];
                if (dir_addr != 0){
                    uint byte_index = dir_addr/8;
                    uint byte_off = dir_addr%8;

                    if((temp_bitmap[byte_index] & (0x1 << byte_off)) != 0){
                        fprintf(stderr, "ERROR: direct address used more than once.\n");
                        return -1;
                    }
                    temp_bitmap[byte_index] = temp_bitmap[byte_index] | (0x1 << byte_off);
                }
            }
        }
    }
    return 0;
}

int check_indir_dup(){
    uchar temp_bitmap[num_bitblocks*BLOCK_SIZE];
    memset(temp_bitmap, 0, num_bitblocks*BLOCK_SIZE);
    for (int i = 0; i < sb->ninodes; i++){
        struct dinode inode = inodes[i];
        short itype = inode.type;
        if (itype != 0){
            uint indir_addr = inode.addrs[NDIRECT];
            if (indir_addr != 0){
                uint *dir_block = (uint*)BOFFSET(indir_addr);
                for (int j = 0; j < NINDIRECT; j++){
                    uint dir_addr = dir_block[j];
                    if (dir_addr != 0){
                         uint byte_index = dir_addr/8;
                         uint byte_off = dir_addr%8;

                         if((temp_bitmap[byte_index] & (0x1 << byte_off)) != 0){
                             fprintf(stderr, "ERROR: indirect address used more than once.\n");
                             return -1;
                         }
                         temp_bitmap[byte_index] = temp_bitmap[byte_index] | (0x1 << byte_off);
                   }

                }
            }

        }
    }
    return 0;
}

int check_inode_referred(){
    uchar inode_bitmap[sb->ninodes / 8];
    for (int i = 0; i < sb->ninodes; i++){
        if (inodes[i].type != 0){
            inode_bitmap[i/8] = inode_bitmap[i/8] | (0x1 << i%8);
        }
    }
    for (int i = 0; i < sb->ninodes; i++){
        struct dinode inode = inodes[i];
        if (inode.type == T_DIR){
            // traverse direct blocks
            for (int j = 0; j < NDIRECT; j++){
                uint dir_addr = inode.addrs[j];
                if (dir_addr != 0){
                    uchar *dir_offset = BOFFSET(dir_addr);
                    
                    for (struct dirent* de = (struct dirent*)dir_offset; 
                            de < (struct dirent*)(dir_offset + BLOCK_SIZE);
                            de++){
                        if (de->inum == 0)
                            continue;
                        int byte_idx = de->inum / 8, byte_off = de->inum % 8;
                        inode_bitmap[byte_idx] = inode_bitmap[byte_idx] & ~(0x1 << byte_off);
                    }
                }
            }
            uint indir_addr = inode.addrs[NDIRECT];
            if(indir_addr != 0){
                uint *dir_block = (uint*)BOFFSET(indir_addr);
                for (int j = 0; j < NINDIRECT; j++){
                    uint dir_addr = dir_block[j];
                    if (dir_addr != 0){
                        uchar *dir_offset = BOFFSET(dir_addr);
                        
                        for (struct dirent* de = (struct dirent*)dir_offset; 
                                de < (struct dirent*)(dir_offset + BLOCK_SIZE);
                                de++){
                            if (de->inum == 0)
                                continue;
                            int byte_idx = de->inum / 8, byte_off = de->inum % 8;
                            inode_bitmap[byte_idx] = inode_bitmap[byte_idx] & ~(0x1 << byte_off);
                        }
                   }

                }
            }
        }
    }
    for (int i = 0; i < sb->ninodes / 8; i++){
        if (inode_bitmap[i] != 0){
            fprintf(stderr, "ERROR: inode marked use but not found in a directory.\n");
            return -1;
        }
    }
    return 0;
}


int check_inode_marked(){
    uchar inode_bitmap[sb->ninodes / 8];
    for (int i = 0; i < sb->ninodes; i++){
        struct dinode inode = inodes[i];
        if (inode.type == T_DIR){
            // traverse direct blocks
            for (int j = 0; j < NDIRECT; j++){
                uint dir_addr = inode.addrs[j];
                if (dir_addr != 0){
                    uchar *dir_offset = BOFFSET(dir_addr);
                    
                    for (struct dirent* de = (struct dirent*)dir_offset; 
                            de < (struct dirent*)(dir_offset + BLOCK_SIZE);
                            de++){
                        if (de->inum == 0)
                            continue;
                        int byte_idx = de->inum / 8, byte_off = de->inum % 8;
                        inode_bitmap[byte_idx] = inode_bitmap[byte_idx] | (0x1 << byte_off);
                    }
                }
            }
            uint indir_addr = inode.addrs[NDIRECT];
            if(indir_addr != 0){
                uint *dir_block = (uint*)BOFFSET(indir_addr);
                for (int j = 0; j < NINDIRECT; j++){
                    uint dir_addr = dir_block[j];
                    if (dir_addr != 0){
                        uchar *dir_offset = BOFFSET(dir_addr);
                        
                        for (struct dirent* de = (struct dirent*)dir_offset; 
                                de < (struct dirent*)(dir_offset + BLOCK_SIZE);
                                de++){
                            if (de->inum == 0)
                                continue;
                            int byte_idx = de->inum / 8, byte_off = de->inum % 8;
                            inode_bitmap[byte_idx] = inode_bitmap[byte_idx] | (0x1 << byte_off);
                        }
                   }

                }
            }
        }
    }
    for (int i = 0; i < sb->ninodes; i++){
        if (inodes[i].type != 0){
            inode_bitmap[i/8] = inode_bitmap[i/8] & ~(0x1 << i%8);
        }
    }
    for (int i = 0; i < sb->ninodes / 8; i++){
        if (inode_bitmap[i] != 0){
            fprintf(stderr, "ERROR: inode referred to in directory but marked free.\n");
            return -1;
        }
    }
    return 0;
}

int check_file_ref_counts(){
    int inode_refmap[sb->ninodes];
    memset(inode_refmap, 0, sb->ninodes* sizeof(int));
    for (int i = 0; i< sb->ninodes; i++){
        if (inodes[i].type == T_FILE)
            inode_refmap[i] = inodes[i].nlink;
    }
    for (int i = 0; i < sb->ninodes; i++){
        struct dinode inode = inodes[i];
        if (inode.type == T_DIR){
            // traverse direct blocks
            for (int j = 0; j < NDIRECT; j++){
                uint dir_addr = inode.addrs[j];
                if (dir_addr != 0){
                    uchar *dir_offset = BOFFSET(dir_addr);
                    
                    for (struct dirent* de = (struct dirent*)dir_offset; 
                            de < (struct dirent*)(dir_offset + BLOCK_SIZE);
                            de++){
                        if (de->inum == 0 || inodes[de->inum].type != T_FILE)
                            continue;
                        inode_refmap[de->inum]--;
                        if (inode_refmap[de->inum] < 0){
                            fprintf(stderr, "ERROR: bad reference count for file.\n");
                            return -1;
                        }
                    }
                }
            }
            uint indir_addr = inode.addrs[NDIRECT];
            if(indir_addr != 0){
                uint *dir_block = (uint*)BOFFSET(indir_addr);
                for (int j = 0; j < NINDIRECT; j++){
                    uint dir_addr = dir_block[j];
                    if (dir_addr != 0){
                        uchar *dir_offset = BOFFSET(dir_addr);
                        
                        for (struct dirent* de = (struct dirent*)dir_offset; 
                                de < (struct dirent*)(dir_offset + BLOCK_SIZE);
                                de++){
                            if (de->inum == 0 || inodes[de->inum].type != T_FILE)
                                continue;
                            inode_refmap[de->inum]--;
                            if (inode_refmap[de->inum] < 0){
                                fprintf(stderr, "ERROR: bad reference count for file.\n");
                                return -1;
                            }
                        }
                   }
                }
            }
        }
    }
    for (int i = 0; i < sb->ninodes; i++){
        if (inode_refmap[i] != 0){
            fprintf(stderr, "ERROR: bad reference count for file.\n");
            return -1;
        }
    }
    return 0;
}


int check_dir_counts(){
    int inode_refmap[sb->ninodes];
    memset(inode_refmap, 0, sb->ninodes* sizeof(int));
    for (int i = 0; i < sb->ninodes; i++){
        struct dinode inode = inodes[i];
        if (inode.type == T_DIR){
            // traverse direct blocks
            for (int j = 0; j < NDIRECT; j++){
                uint dir_addr = inode.addrs[j];
                if (dir_addr != 0){
                    uchar *dir_offset = BOFFSET(dir_addr);
                    
                    for (struct dirent* de = (struct dirent*)dir_offset + 2; 
                            de < (struct dirent*)(dir_offset + BLOCK_SIZE);
                            de++){
                        if (de->inum == 0 || inodes[de->inum].type != T_DIR)
                            continue;
                        inode_refmap[de->inum]++;
                        if (inode_refmap[de->inum] > 1){
                            fprintf(stderr, "ERROR: directory appears more than once in file system.\n");
                            return -1;
                        }
                    }
                }
            }
            uint indir_addr = inode.addrs[NDIRECT];
            if(indir_addr != 0){
                uint *dir_block = (uint*)BOFFSET(indir_addr);
                for (int j = 0; j < NINDIRECT; j++){
                    uint dir_addr = dir_block[j];
                    if (dir_addr != 0){
                        uchar *dir_offset = BOFFSET(dir_addr);
                        for (struct dirent* de = (struct dirent*)dir_offset + 2; 
                                de < (struct dirent*)(dir_offset + BLOCK_SIZE);
                                de++){
                            if (de->inum == 0 || inodes[de->inum].type != T_DIR)
                                continue;
                            inode_refmap[de->inum]++;
                            if (inode_refmap[de->inum] > 1){
                                fprintf(stderr, "ERROR: directory appears more than once in file system.\n");
                                return -1;
                            }
                        }
                   }
                }
            }
        }
    }
    for (int i = 0; i < sb->ninodes; i++){
        if (inode_refmap[i] > 1){
            fprintf(stderr, "ERROR: directory appears more than once in file system.\n");
            return -1;
        }
    }
    return 0;
}

int
main(int argc, char *argv[])
{
    if (argc == 1){ 
        fprintf(stderr, "Usage: xcheck <file_system_image> \n"); 
        exit(1); 
    } 
    char *img_path = argv[1];
    int img_fd;
    if((img_fd = open(img_path, O_RDONLY)) == -1)
    {
        fprintf(stderr, "image not found.\n");
        exit(1);
    }
    size_t size;
    struct stat s;
    if(fstat(img_fd, &s) < 0)
        exit(1);
    size = s.st_size;
   
    if((mapped_img = mmap(0, size, PROT_READ, MAP_PRIVATE, img_fd, 0)) < 0)
        exit(1);

    // set super block pointer 
    sb = (struct superblock *)BOFFSET(1);
    inodes = (struct dinode *)BOFFSET(2); 
    bitmap = (uchar *)BOFFSET(sb->ninodes / IPB + 3);
    num_bitblocks = sb->size/(BLOCK_SIZE*8) + 1;
    datablocks = (uchar *)BOFFSET(sb->ninodes / IPB + 3 + num_bitblocks);
    num_metablocks = sb->ninodes / IPB + 3 + num_bitblocks;
    
    int (*check_funcs[NUM_FUNC])() = {check_inodes, check_addr_valid, check_root, check_dir,  check_bitmap_marked, 
        check_no_extra_bit, check_dir_dup, check_indir_dup, check_inode_referred, check_inode_marked,
        check_file_ref_counts, check_dir_counts };
    for(int i = 0;i < NUM_FUNC; i++)
    {
        if(check_funcs[i]() < 0 )
            exit(1);
    }
    
    // no problems detected
    exit(0);
}