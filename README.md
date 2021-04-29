# Filesystem.c

## Description
The project is an emulation of a filesystem programmed in C, with support for related commands. It supports commands similar to that of the UNIX terminals, including file creation, deletion, and with support for copy/move operations.

The project has the following characteristics:
- The whole disk is 128 KB in size.
- The top most directory is the root directory (/).
- The system can have a maximum of 16 files/directories.
- A file can have a maximum of 8 blocks (no indirect pointers). Each block is 1 KB in size.
- A file/directory name can be of 8 chars max (including NULL char). There can be only one file of a given name in a directory.

After executing every command the program updates the state of the hard disk in a
file called "myfs" in the current directory. When the program terminates, this file will contain the snapshot of the hard disk at the end of the program. 
If it does not find the file titled "myfs" in the current directory it will create an empty hard disk by formatting it
according to the specified layout and creating a the first root directory (/).

### Disk Layout
The disk layout is as follows: The 128 blocks are divided into 1 super block and 127 data blocks.
The superblock contains the 128 byte free block list where each byte contains a boolean value
indicating whether that particular block is free or not.
Just after the free block list, in the super block, we have the inode table containing the 16 inodes
themselves. Each inode is 56 bytes in size and contains metadata about the stored files/directo-
ries as indicated by the data structure in the accompanying filesystem.c.

## Supported Commands

### Create file
syntax: ```CR filename size```

### Delete file
syntax: ```DL filename```

### Copy file
syntax: ```CP srcname dstname```

### Move file
syntax: ```MV srcname dstname```

### Create Directory
syntax: ```CD dirname```

### Remove Directory
syntax: ```DD dirname```

### List all files
syntax: ```LL```

## Material Referenced
[Operating Systems: Three Easy Pieces](https://pages.cs.wisc.edu/~remzi/OSTEP/) 
