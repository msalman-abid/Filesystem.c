#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>

/*
 *   ___ ___ ___ ___ ___ ___ ___ ___ ___ ___ ___ 
 *  |   |   |   |   |                       |   |
 *  | 0 | 1 | 2 | 3 |     .....             |127|
 *  |___|___|___|___|_______________________|___|
 *  |   \    <-----  data blocks ------>
 *  |     \
 *  |       \
 *  |         \
 *  |           \
 *  |             \
 *  |               \
 *  |                 \
 *  |                   \
 *  |                     \
 *  |                       \
 *  |                         \
 *  |                           \
 *  |                             \
 *  |                               \
 *  |                                 \
 *  |                                   \
 *  |                                     \
 *  |                                       \
 *  |                                         \
 *  |                                           \
 *  |     <--- super block --->                   \
 *  |______________________________________________|
 *  |               |      |      |        |       |
 *  |        free   |      |      |        |       |
 *  |       block   |inode0|inode1|   .... |inode15|
 *  |        list   |      |      |        |       |
 *  |_______________|______|______|________|_______|
 *
 *
 */


#define FILENAME_MAXLEN 8  // including the NULL char
#define BLOCK_SIZE 1024 //1KB block
#define DATA_BLOCKS 127
#define MAX_INODES 16
/* 
 * inode 
 */

typedef struct inode {
  int  dir;  // boolean value. 1 if it's a directory.
  char name[FILENAME_MAXLEN];
  int  size;  // actual file/directory size in bytes.
  int  blockptrs [8];  // direct pointers to blocks containing file's content.
  int  used;  // boolean value. 1 if the entry is in use.
  int  rsvd;  // reserved for future use
} inode;


/* 
 * directory entry
 */

typedef struct dirent {
  char name[FILENAME_MAXLEN];
  int  namelen;  // length of entry name
  int  inode;  // this entry inode index
} dirent;


typedef union block{
    char data[BLOCK_SIZE];
    dirent dirent_ptr[64];
    // char *data;
    // dirent *dirent_ptr; //Allocate entire block (64 dirents)
} block;

// typedef struct dir_block{
//     dirent entries[64];
// } dir_block;

typedef struct super_block {
  char free_list[1+DATA_BLOCKS]; // '0' = occupied, '1' = free
  inode inode_table[MAX_INODES];

} super_block;

typedef struct disk {
//   super_block sup_block;
//   block data_blocks[DATA_BLOCKS]; //pointers to disk blocks
  char fs_array[128*1024];
  super_block* sup_block;
  block* data_blocks;

} disk;

/*
 * functions
 */
// create file
// copy file
// remove/delete file
// move a file
// list file info
// create directory
// remove a directory
#define LETTER_START 97
#define LETTER_END 122


char* find_lastname(char* path)
{
    /* 
     * Extract the leaf item from the file path. 
     */

    if (strcmp("/", path) == 0)
        return path;

    int last_sep = -1;
    for (int i = 0; i < strlen(path); i++)
    {
        if (path[i] == '/')
            last_sep = i;
    }

    return &(path[last_sep+1]);
}

int reset_parent_dir(block* parent_dir_block ,char* child_name)
{
    int return_val = -1;
    for (int i = 0; i < 64; i++)
    {
        if (strcmp(parent_dir_block->dirent_ptr[i].name, child_name) == 0)
        {
            strcpy(parent_dir_block->dirent_ptr[i].name,"");
            parent_dir_block->dirent_ptr[i].namelen = 0;
            return_val = parent_dir_block->dirent_ptr[i].inode;
            parent_dir_block->dirent_ptr[i].inode = -1;
            
            break;
        } 
    }
    return return_val;
}

int traverse_path(disk* m_disk, char* path)
{
    /* Find last (valid) folder in path and return its inode.
     */

    int dir_inode = -1;
    int last_sep = 0;

    if (path[0] != '/')
    {
        printf("Invalid Path: Does not start from root\n");
        return dir_inode;
    }

    path++; //skip root char
    dir_inode = 0; //root inode
    char buffer[256] = "";

    //get relevant (existing path)
    for (int i = 0; i < strlen(path); i++)
    {
        if (path[i] == '/')
            last_sep = i;
    }
    
    //cut last file/directory away
    strncpy(buffer,path, last_sep);

    if (last_sep == 0)
        return dir_inode; //return root
    
    char* token = NULL;
    const char* delim = "/";

    token = strtok(buffer, delim);
    

    while (token!=NULL)
    {
        int found = 0;
        inode* inode_d = &(m_disk->sup_block->inode_table[dir_inode]);
        block* tmp_block = &(m_disk->data_blocks[inode_d->blockptrs[0]]);

        //check if this dir exists in current dir (64 entries)
        for (int i = 0; i < 64; i++)
        {
            if (strcmp(token,tmp_block->dirent_ptr[i].name) == 0)
            {
                
                dir_inode = tmp_block->dirent_ptr[i].inode;
                found = 1;
                break;
            }

        }

        if (found == 0)
        {
            printf("Directory %s not found in current directory\n\n", token);
            dir_inode = -1;
            break;
        }
        
        token = strtok(NULL, delim);

    }
    
    return dir_inode;
}

void update_parent_size(disk *m_disk, char* path, int m_size)
{
    /*
     * Recursively traverse from child to root,
     * adding size to each directory in the path.
     */

    int parent_inode = -1;
    int last_sep = -1;
    char buffer[256];
    
    parent_inode = traverse_path(m_disk, path);
    inode* parent_inode_ptr = &(m_disk->sup_block->inode_table[parent_inode]);

    parent_inode_ptr->size += m_size;

    //get relevant (existing path)
    for (int i = 0; i < strlen(path); i++)
    {
        if (path[i] == '/')
            last_sep = i;
    }

    if (last_sep == 0)
        return;

    strncpy(buffer, path, last_sep);
    strcat(buffer, "\0");


    update_parent_size(m_disk, buffer, m_size);

}

int chk_file_exists(disk *m_disk, char* path)
{
    int parent_inode = traverse_path(m_disk, path);
    inode* parent_inode_ptr = &(m_disk->sup_block->inode_table[parent_inode]);
    block* parent_dir_block = &(m_disk->data_blocks[parent_inode_ptr->blockptrs[0]]);


    for (int i = 0; i < 64; i++)
    {
        if (strcmp(parent_dir_block->dirent_ptr[i].name, 
            find_lastname(path)) == 0)
        {
            inode* tmp = &(m_disk->sup_block
            ->inode_table[parent_dir_block->dirent_ptr[i].inode]);
            
            if (tmp->dir)
                return -1;
            
            //File exists
            return 1; 
        } 
    }

    return 0;
}

int populate_file(disk *m_disk, inode *f_inode, int size)
{
    /*
     * Allocate data blocks to inode, and write the specified 
     * number of bytes in the block.  
     */

    //check disk free space
    int free_blocks = 0;
    int blocks_needed = (int) ceil(size/1024);

    for (int i = 1; i < DATA_BLOCKS; i++)
    {
        if (m_disk->sup_block->free_list[i] == '1')
            free_blocks += 1;
    }
    
    if ((free_blocks - blocks_needed) < 0)
    {
        printf("Not enough space on disk\n");
        return -1;
    }
    

    int total_written = 0;
    int block_num = 0;
    int current_letter = LETTER_START;
    block* curr_block = NULL;

    while(total_written < size)
    {
        if (block_num == 8)
        {
            printf("Max size allocated\n");
            return -1;
        }
        
        //allocate new block
        for (int i = 1; i < (DATA_BLOCKS); i++)
        {
            if (m_disk->sup_block->free_list[i] == '1')
            {
                m_disk->sup_block->free_list[i] = '0';

                curr_block = &(m_disk->data_blocks[i]);
                f_inode->blockptrs[block_num] = i; 
                block_num++; //next free blockptr
                break;
            }
        }

        if (curr_block == NULL)
        {    
            printf("No free block for file");
            return -1;
        }


        for (int i = 0; i < 1024; i++)
        {
            curr_block->data[i] = current_letter;

            if (++total_written == size)
            {   
                curr_block->data[i+1] = '\0'; //add null char at EOF
                break;
            }
            current_letter = (current_letter + 1 == LETTER_END ? 
                LETTER_START: current_letter + 1);
        }

    } return 0;
}

int allocate_folder(disk* m_disk, inode* m_inode)
{
    /*
     * Allocate a single datablock to a folder inode,
     * containing an array of 64 dirents.
     */

    int ret_val = -1;
    

    //find free block 
    for (int i = 1; i < (DATA_BLOCKS); i++)
    {
        if (m_disk->sup_block->free_list[i] == '1')
        {
            m_disk->sup_block->free_list[i] = '0';
            m_inode->blockptrs[0] = i; //store direct ptr in inode
            ret_val = i; //data block where folder's data is stored
            break;
        }
    }
    if (ret_val < 0)
        printf("No free block for folder\n");
    else
    {
        for (int i = 0; i < 64; i++)
        {
            m_disk->data_blocks[ret_val].dirent_ptr[i].inode = -1; //mark dirent array entry as unused
        }
    }
    

    return ret_val;
    
}

int delete_file(disk* m_disk, char* f_name)
{
    /* 
    *  Delete file from disk. Reset parent direntry,
    *  free data blocks, and mark inode as free.
    *  Return free inode number. 
    */

    int file_inode = -1;
    
    //parent directory inode number
    int d_inode_idx = traverse_path(m_disk, f_name);

    inode* parent_inode = &(m_disk->sup_block->inode_table[d_inode_idx]);
    block *tmp_block = &(m_disk->data_blocks[parent_inode->blockptrs[0]]);
    
    char tmp_name[FILENAME_MAXLEN];
    strcpy(tmp_name, find_lastname(f_name));

    //reset parent dirent entry, and save file inode
    file_inode = reset_parent_dir(tmp_block, tmp_name);

    if (file_inode == -1)
    {
        printf("The file %s does not exist\n", f_name);
        return file_inode;
    }

    inode* inode_f = &(m_disk->sup_block->inode_table[file_inode]);
    
    //free any data blocks allocated
    for (int i = 0; i < 8; i++)
    {
        if (inode_f->blockptrs[i] != -1)
        {
            m_disk->sup_block->free_list[inode_f->blockptrs[i]] = '1';
            inode_f->blockptrs[i] = -1;
        }
    }

    update_parent_size(m_disk, f_name, inode_f->size * -1);
    inode_f->used = 0;

    return file_inode;

}


int delete_directory(disk*m_disk, char* d_name)
{
    /*
     * (Recursively) delete internals of a specified folder,
     * and then delete the folder as well.
     * 
     */
    int dir_inode = -1;
    
    //parent directory inode and block pointers
    int d_inode = traverse_path(m_disk, d_name);
    inode* parent_inode = &(m_disk->sup_block->inode_table[d_inode]);
    block *tmp_block = &(m_disk->data_blocks[parent_inode->blockptrs[0]]);
    
    char tmp_name[FILENAME_MAXLEN];
    strcpy(tmp_name, find_lastname(d_name));


    //get inode value
    for (int i = 0; i < 64; i++)
    {
        if (strcmp(tmp_block->dirent_ptr[i].name, tmp_name) == 0)
        {
            //save file inode for deletion
            dir_inode = tmp_block->dirent_ptr[i].inode;
            break;
        } 
    }

    if (dir_inode == -1)
    {
        printf("Dir not found\n");
        return dir_inode;
    }

    inode* inode_d = &(m_disk->sup_block->inode_table[dir_inode]);
   
    //nothing to delete internally
    if (inode_d->rsvd)
    {
        inode_d->used = 0;
        reset_parent_dir(tmp_block, tmp_name);
        return dir_inode;
    }
    
    block* tmp_block2 = (block *) &(m_disk->data_blocks[inode_d->blockptrs[0]]);
    char buffer[256] = "";

    //further delete internals (if any)
    for (int i = 0; i< 64; i++)
    {
        strcpy(buffer, d_name);
        strcat(buffer, "/");
        if (tmp_block2->dirent_ptr[i].inode != -1)
        {
            int tmp_inode = tmp_block2->dirent_ptr[i].inode;
            if (m_disk->sup_block->inode_table[tmp_inode].dir)
            {
                strcat(buffer, tmp_block2->dirent_ptr[i].name);
                delete_directory(m_disk, buffer);
            } else {
                strcat(buffer, tmp_block2->dirent_ptr[i].name);
                delete_file(m_disk, buffer);
            }
            
        }  
    }
    
    //reset direntry for parent
    reset_parent_dir(tmp_block, tmp_name);

    m_disk->sup_block->free_list[inode_d->blockptrs[0]] = '1';
    inode_d->blockptrs[0] = -1;

    inode_d->used = 0;
    return dir_inode;
}

int create_file(disk* m_disk, char* f_name, int f_size)
{
    int ret_val = -1;
    int f_inode_num = -1;
    
    if (f_size > (8 * BLOCK_SIZE))
    {
        printf("File size larger than maximum.\n");
        return ret_val;
    }
    

    if (f_name[0] != '/')
        return ret_val;    
    
    //parent inode for file
    int d_inode = traverse_path(m_disk, f_name);
    if (d_inode == -1)
    {
        return d_inode;
    }
    
    //get inode ptr for parent directory to create file
    inode* parent_dir_inode = &(m_disk->sup_block->inode_table[d_inode]);
    block* tmp_block = NULL;

    //if parent folder is empty, allocate data block
    if (parent_dir_inode->rsvd)
    {
        parent_dir_inode->rsvd = 0;
        allocate_folder(m_disk, parent_dir_inode);
    }

    //a folder can only have first blockptrs entry in use (1 block enough)
    tmp_block = (block*) &(m_disk->data_blocks[parent_dir_inode->blockptrs[0]]);

    //check if file already exists in parent
    if (chk_file_exists(m_disk, f_name))
    {
        printf("File %s already exists.\n", f_name);
        return ret_val;
    }
        
   
   //find free inode for file
    for (int i = 0; i < MAX_INODES; i++)
    {
        if (m_disk->sup_block->inode_table[i].used == 0)
        {
            m_disk->sup_block->inode_table[i].used = 1;
            f_inode_num = i;
            break;
        }
    }

    if (f_inode_num == -1)
    {
        printf("No free inode for file\n");
        return ret_val;
    }



    //find empty dirent entry in parent array
    for (int i = 0; i < 64; i++)
    {
        if(tmp_block->dirent_ptr[i].inode == -1)
        {
            //setup direntry for current file
            tmp_block->dirent_ptr[i].inode = f_inode_num;
            strcpy(tmp_block->dirent_ptr[i].name, find_lastname(f_name));
            tmp_block->dirent_ptr[i].namelen = strlen(find_lastname(f_name));
            break;
        }
    }
    
    update_parent_size(m_disk, f_name, f_size);

    //setup file inode
    inode* f_inode = &(m_disk->sup_block->inode_table[f_inode_num]);

    if(populate_file(m_disk, f_inode, f_size) == -1)
        return -1;
    
    f_inode->dir = 0;
    strcpy(f_inode->name, find_lastname(f_name));
    f_inode->size = f_size;
    f_inode->rsvd = 0;

    ret_val = f_inode_num;
         

    return ret_val;
}


int create_directory(disk* m_disk, char* d_name, int d_size)
{

    int child_inode_idx = -1;
    int parent_inode_idx = -1;
    block* tmp_block = NULL;
    //get parent directory
    parent_inode_idx = traverse_path(m_disk, d_name);
    inode* parent_inode = &(m_disk->sup_block->inode_table[parent_inode_idx]);

    if (parent_inode->rsvd)
    {
        parent_inode->rsvd = 0;
        allocate_folder(m_disk, parent_inode);
    }
    
    //folder can only have first blockptrs entry in use (64 entries enough)
    tmp_block = (block*) &(m_disk->data_blocks[parent_inode->blockptrs[0]]);

    //check for already existing directory in parent
    for (int i = 0; i < 64; i++)
    {
        if(tmp_block->dirent_ptr[i].inode != -1 && 
            (strcmp(tmp_block->dirent_ptr[i].name, find_lastname(d_name)) == 0))
        {
            printf("Folder %s already exists.\n", d_name);
            return -1;   
        
        }
    }

    //find free inode for new directory
    for (int i = 0; i < MAX_INODES; i++)
    {
        if (m_disk->sup_block->inode_table[i].used == 0)
        {
            m_disk->sup_block->inode_table[i].used = 1;
            child_inode_idx = i;
            break;
        }   
    }
    
    if (child_inode_idx == -1)
    {
        printf("No new inode found\n");
        return -1;
    }

    

    //find empty dirent entry in parent's array
    for (int i = 0; i < 64; i++)
    {
        if(tmp_block->dirent_ptr[i].inode == -1)
        {
            //setup direntry for new directory
            tmp_block->dirent_ptr[i].inode = child_inode_idx;
            strcpy(tmp_block->dirent_ptr[i].name, find_lastname(d_name));
            tmp_block->dirent_ptr[i].namelen = strlen(find_lastname(d_name));
            break;
        }
    }

    //setup directory inode
    inode* tmp = &(m_disk->sup_block->inode_table[child_inode_idx]);

    tmp->dir = 1;
    strcpy(tmp->name, find_lastname(d_name));
    tmp->size = d_size;
    tmp->used = 1;
    if (d_size == 0)
        tmp->rsvd = 1;

    return child_inode_idx;

}

int copy_file(disk* m_disk, char* src_path, char* dest_path, int move_flag)
{
    /* 
     * Create new file on destination path,
     * using information from source.
     * Delete original if move flag is set.
     */

    int src_inode = -1;
    
    //parent directory of src_file
    int d_inode = traverse_path(m_disk, src_path);

    if(d_inode == -1)
        return -1;

    inode* parent_inode = &(m_disk->sup_block->inode_table[d_inode]);

    char tmp_name[FILENAME_MAXLEN];
    strcpy(tmp_name, find_lastname(src_path));
    block *tmp_block = &(m_disk->data_blocks[parent_inode->blockptrs[0]]);

    for (int i = 0; i < 64; i++)
    {
        if (strcmp(tmp_block->dirent_ptr[i].name, tmp_name) == 0)
        {
            //get src file inode entry
            src_inode = tmp_block->dirent_ptr[i].inode;
            break;
        } 
    }

    if (src_inode == -1)
    {
        printf("Src File %s not found\n", src_path);
        return src_inode;
    }    

    if(m_disk->sup_block->inode_table[src_inode].dir)
    {
        printf("Cannot handle directories for copying.\n");
        return -1;
    }

    //overwrite existing destination
    if (chk_file_exists(m_disk, dest_path) == 1)
        assert(delete_file(m_disk, dest_path) != -1);

    if (chk_file_exists(m_disk, dest_path) == -1)
    {
        printf("Destination is a directory. Copy Unsucccessful\n");
        return -1;
    }
    
    int new_size = m_disk->sup_block->inode_table[src_inode].size;

    if(traverse_path(m_disk, src_path) == -1)
    {
        return -1;
    }

    if (move_flag)
        assert(delete_file(m_disk, src_path) != -1 );

    int ret = create_file(m_disk, dest_path, new_size);
    
    return ret;
}

void list_all(disk* m_disk, char* m_path)
{
    /*
     * List all files in disk. Files of the current folder
     * will be listed before listing folders. 
     * This will then take place recursively for each sub folder.
     * Format: IsDirectiory Size Path
     */

    char buffer[256] = "";
    int dir_inode = 0;

    //if root, parent is NULL
    if (strcmp(m_path, "/") == 0)
    {
        goto current_folder;
    }
    
    //access parent folder directory entries
    int parent_inode_num = traverse_path(m_disk, m_path);
    inode* p_inode = &(m_disk->sup_block->inode_table[parent_inode_num]);
    int block_num = p_inode->blockptrs[0];
    dirent* parent_dir = m_disk->data_blocks[block_num].dirent_ptr;

    strcpy(buffer, find_lastname(m_path));

    //find inode from parent dirent
    for (int i = 0; i < 64; i++)
    {
        if (strcmp(parent_dir[i].name, buffer) == 0)
        {
            dir_inode = parent_dir[i].inode;
            break;
        } 
    }
    
    current_folder: ;
    inode* d_inode = &(m_disk->sup_block->inode_table[dir_inode]);
    int block_num2 = d_inode->blockptrs[0];
    dirent* dir_ptr = m_disk->data_blocks[block_num2].dirent_ptr;

    //print current folder
    strcpy(buffer, m_path); 
    printf("Directory: %d\tSize: %d\t%s\n",d_inode->dir, d_inode->size,buffer);
 

    //print all files in pwd
    for (int i = 0; i < 64; i++)
    {
        if(d_inode->rsvd)
            break;

        strcpy(buffer, m_path);
        if (strcmp(m_path, "/") != 0)
            strcat(buffer, "/");

        if (dir_ptr[i].inode != -1)
        {
            int tmp_inode = dir_ptr[i].inode;
            inode *tmp_inode_ptr = &(m_disk->sup_block->inode_table[tmp_inode]);

            if (!(tmp_inode_ptr->dir))
            {
                strcat(buffer, dir_ptr[i].name);
                printf("Directory: 0\tSize: %d\t%s\n",tmp_inode_ptr->size, buffer); 
            }
        }  
    }

    //enter all subfolders and list them recursively
    for (int i = 0; i < 64; i++)
    {
        if(d_inode->rsvd)
            break;

        strcpy(buffer, m_path);
        if (strcmp(m_path, "/") != 0)
            strcat(buffer, "/");

        if (dir_ptr[i].inode != -1)
        {
            int tmp_inode = dir_ptr[i].inode;
            if (m_disk->sup_block->inode_table[tmp_inode].dir)
            {
                strcat(buffer, dir_ptr[i].name);
                list_all(m_disk, buffer);
            }
        }
    }

    return;
}

void write_state(disk* m_disk)
{
    FILE* myfs_fd;
    if((myfs_fd = fopen("myfs", "wb")) == NULL )
    {
        printf("Error writing on file. Please re-run executable.\n");
        exit(1);
    }

    fwrite(m_disk->fs_array, sizeof(m_disk->fs_array), 1, myfs_fd);
    fclose(myfs_fd);
}

int read_state(disk* m_disk)
{
    /*
     * Attempt to open existing myfs file
     * and restore disk to previous state.
     * Return 0 on success, else -1.
     */

    FILE* myfs_fd;
    if((myfs_fd = fopen("myfs", "rb")) == NULL )
    {
        printf("Previous state does not exist.\n");
        printf("Creating new disk...\n\n");
        return -1;
    }

    fread(m_disk->fs_array, sizeof(m_disk->fs_array), 1, myfs_fd);
    
    fclose(myfs_fd);
    return 0;
}

void init_disk(disk* m_disk, int resume)
{
    // read_state(m_disk);
    m_disk->sup_block = (super_block*) m_disk->fs_array;
    m_disk->data_blocks = (block*) &(m_disk->fs_array[1024]);
    
    if (resume == 0)
    {
        printf("\t****Restored state from previous file.****\n\n");
        return;
    }
    
    //set super block to occupied
    m_disk->sup_block->free_list[0] = '0';

    for (int i = 1; i < (DATA_BLOCKS+1); i++)
    {
        m_disk->sup_block->free_list[i] = '1';
    }
    
    for (int i = 0; i < MAX_INODES; i++)
    {
        m_disk->sup_block->inode_table[i].used = 0;
        
        for (int j = 0; j < 8; j++)
        {
            m_disk->sup_block->inode_table[i].blockptrs[j] = -1;
        }
        
    }

    //setup root directory
    m_disk->sup_block->inode_table[0].used = 1;
    inode* tmp = &(m_disk->sup_block->inode_table[0]);
    tmp->dir = 1;
    strcpy(tmp->name, "/");
    tmp->size = 0;
    tmp->used = 1;
    tmp->rsvd = 1;
    
}


/*
 * main
 */
int main (int argc, char* argv[]) {

  if (argc < 2)
  {
      printf("Please provide input file as argument.\n");
      exit(1);
  }

  if (argc > 2)
  {
      printf("Too many arguments.\n");
      exit(1);
  }

  disk m_disk;
  init_disk(&m_disk, read_state(&m_disk));

  
  FILE * input_fd = fopen(argv[1], "r");
  assert(input_fd != 0);

  char tmp_buffer[256];
  char cmd_src[256];
  char cmd_dest[256];
  int cmd_size;
  char *token;

  while (fgets(tmp_buffer, sizeof (tmp_buffer), input_fd) != NULL )
  {
      strtok(tmp_buffer, "\n"); //rmv trailing newline
      token = strtok(tmp_buffer, " "); //get cmd
      
      if ( strcmp(token, "CR") == 0)
      {
        token = strtok(NULL, " ");
        strcpy(cmd_src, token);
        token = strtok(NULL, " ");
        cmd_size = atoi(token);
        create_file(&m_disk, cmd_src, cmd_size);
      
      } else if (strcmp(token, "CD") == 0)
      {
        token = strtok(NULL, " ");
        strcpy(cmd_src, token);

        create_directory(&m_disk, cmd_src, 0);

      }  else if (strcmp(token, "CP") == 0)
      {
        token = strtok(NULL, " ");
        strcpy(cmd_src, token);
        token = strtok(NULL, " ");
        strcpy(cmd_dest, token);

        copy_file(&m_disk, cmd_src, cmd_dest, 0);
        
      } else if (strcmp(token, "MV") == 0)
      {
        token = strtok(NULL, " ");
        strcpy(cmd_src, token);
        token = strtok(NULL, " ");
        strcpy(cmd_dest, token);
        copy_file(&m_disk, cmd_src, cmd_dest, 1);
        
      } else if (strcmp(token, "DL") == 0)
      {
        token = strtok(NULL, " ");
        strcpy(cmd_src, token);
        delete_file(&m_disk, cmd_src);
      
      } else if (strcmp(token, "DD") == 0)
      {
        token = strtok(NULL, " ");
        strcpy(cmd_src, token);
        delete_directory(&m_disk, cmd_src);
      
      } else if (strcmp(token, "LL") == 0)
      {
        list_all(&m_disk, "/");
      } else
      {
          printf("Wrong command encountered\n.");
      }

    write_state(&m_disk);
  }
    fclose(input_fd);
	return 0;
}
