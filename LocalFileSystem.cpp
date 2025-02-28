#include <iostream>
#include <string>
#include <vector>
#include <cassert>
#include <cstring>
#include <algorithm> 
#include "LocalFileSystem.h"
#include "ufs.h"

using namespace std;

//good
LocalFileSystem::LocalFileSystem(Disk *disk) {
  this->disk = disk;
}

//good
void LocalFileSystem::readSuperBlock(super_t *super) {
  char block[UFS_BLOCK_SIZE];

  // read block 0 ( this is the superblock)
  disk->readBlock(0, block);  // removes error check since readBlock returns void

  // copy the superblock data into the 'super' structure
  memcpy(super, block, sizeof(super_t));
}

// questionable - test now - old code works for now
void LocalFileSystem::readInodeBitmap(super_t *super, unsigned char *inodeBitmap) {
  for (int i = 0; i < super->inode_bitmap_len; i++) {  
    char block[UFS_BLOCK_SIZE];  
    int blockNum = super->inode_bitmap_addr + i;  
    disk->readBlock(blockNum, block);  

    // copy the inode bitmap block into memory  
    memcpy(reinterpret_cast<char*>(inodeBitmap) + (i * UFS_BLOCK_SIZE), block, UFS_BLOCK_SIZE);  
}
}

void LocalFileSystem::writeInodeBitmap(super_t *super, unsigned char *inodeBitmap) {
  // write each block of inode bitmap back to disk
  for (int i = 0; i < super->inode_bitmap_len; i++) {
      char block[UFS_BLOCK_SIZE];
      // copy corresponding section of inodeBitmap to block buffer
      memcpy(block, reinterpret_cast<char*>(inodeBitmap) + (i * UFS_BLOCK_SIZE), UFS_BLOCK_SIZE);
      // write block to disk
      int blockNum = super->inode_bitmap_addr + i;
      disk->writeBlock(blockNum, block);
  }
}

void LocalFileSystem::readDataBitmap(super_t *super, unsigned char *dataBitmap) {
  // read each block of the data bitmap region -> copy into dataBitmap.
  for (int i = 0; i < super->data_bitmap_len; i++) {
      char block[UFS_BLOCK_SIZE];
      int blockNum = super->data_bitmap_addr + i;
      disk->readBlock(blockNum, block);
      memcpy(reinterpret_cast<char*>(dataBitmap) + (i * UFS_BLOCK_SIZE), block, UFS_BLOCK_SIZE);
  }
}

void LocalFileSystem::writeDataBitmap(super_t *super, unsigned char *dataBitmap) {
  // write each block of data bitmap back to disk
  for (int i = 0; i < super->data_bitmap_len; i++) {
      char block[UFS_BLOCK_SIZE];
      // copy corresponding section of dataBitmap to block buffer
      memcpy(block, reinterpret_cast<char*>(dataBitmap) + (i * UFS_BLOCK_SIZE), UFS_BLOCK_SIZE);
      // write block to disk
      int blockNum = super->data_bitmap_addr + i;
      disk->writeBlock(blockNum, block);
  }
}

void LocalFileSystem::readInodeRegion(super_t *super, inode_t *inodes) {
  // read 'inode_region_len' blocks from disk into the inodes array
  for (int i = 0; i < super->inode_region_len; i++) {
      char block[UFS_BLOCK_SIZE];
      int blockNum = super->inode_region_addr + i;  // starting block of inode region + offset

      // read one block from the disk
      disk->readBlock(blockNum, block);

      // copy the block into the correct place in the 'inodes' array yippee
      memcpy(reinterpret_cast<char*>(inodes) + (i * UFS_BLOCK_SIZE),
             block,
             UFS_BLOCK_SIZE);
  }
}

void LocalFileSystem::writeInodeRegion(super_t *super, inode_t *inodes) {
  // Write the inode table back to disk
  for (int i = 0; i < super->inode_region_len; i++) {
      char block[UFS_BLOCK_SIZE];
      // Copy the corresponding section of inodes to the block buffer
      memcpy(block, reinterpret_cast<char*>(inodes) + (i * UFS_BLOCK_SIZE), UFS_BLOCK_SIZE);
      // Write block to disk
      int blockNum = super->inode_region_addr + i;
      disk->writeBlock(blockNum, block);
  }
}

// rm error and mkdir/touch func point testing - new function - its helping - DIAGNOSED AS PART OF ISSUE
int LocalFileSystem::lookup(int parentInodeNumber, string targetName) {
    inode_t parentDirInode;

    // get the parent directory's inode
    int status = this->stat(parentInodeNumber, &parentDirInode);
    if (status != 0) {
        return -EINVALIDINODE;
    }

    // ensure the inode belongs to a directory
    if (parentDirInode.type != UFS_DIRECTORY) {
        return -EINVALIDINODE;
    }

    int dirSize = parentDirInode.size;
    vector<char> dirBuffer(dirSize);

    // read directory data into buffer
    int bytesRead = this->read(parentInodeNumber, dirBuffer.data(), dirSize);
    if (bytesRead < 0) {
        return -EINVALIDINODE;
    }

    int offset = 0;
    while (offset < bytesRead) {
        dir_ent_t *entry = reinterpret_cast<dir_ent_t *>(dirBuffer.data() + offset);

        // check if the entry is valid and matches the target name
        if (entry->inum != -1 && strcmp(entry->name, targetName.c_str()) == 0) {
            return entry->inum;
        }

        offset += sizeof(dir_ent_t);
    }

    return -ENOTFOUND;  // entry not found
}

// questionable - test now - old code works for now
int LocalFileSystem::stat(int inodeNumber, inode_t *inode) {
    super_t super;
    readSuperBlock(&super); // read the superblock to get inode region details

    // check if the inode number is within a valid range
    if (inodeNumber < 0 || inodeNumber >= super.num_inodes) {
        return -1; // invalid inode number
    }

    // calculate the total number of inodes in the inode region
    int numInodesInRegion = (super.inode_region_len * UFS_BLOCK_SIZE) / sizeof(inode_t);
    if (inodeNumber >= numInodesInRegion) {
        return -1; // inode number is out of bounds
    }

    // read the entire inode table into memory
    vector<inode_t> inodes(numInodesInRegion);
    readInodeRegion(&super, inodes.data());

    // copy the requested inode data into the provided structure
    *inode = inodes[inodeNumber];
    return 0;
}

// questionable - test now - diagnosed as the issue for my read utility tests - fixed
int LocalFileSystem::read(int inodeNumber, void *buffer, int size) {
    inode_t inode;
    // retrieve inode information
    int statResult = stat(inodeNumber, &inode);
    if (statResult != 0) {
        return statResult; // return error if inode lookup fails
    }

    // check if requested size is valid
    if (size > MAX_FILE_SIZE || size < 0) {
        return -EINVALIDSIZE; // invalid file size
    }

    // if requested size is larger than the actual file size, adjust it
    if (size > inode.size) {
        size = inode.size;
    }

    int total_bytes_read = 0;
    char* bufferPtr = static_cast<char*>(buffer);

    // calculate number of blocks needed to read the requested size
    int num_blocks = (size + UFS_BLOCK_SIZE - 1) / UFS_BLOCK_SIZE;
    for (int i = 0; i < num_blocks; i++) {
        char buf[UFS_BLOCK_SIZE];

        // read the corresponding block from disk
        disk->readBlock(inode.direct[i], buf);

        // determine how many bytes to read from this block
        int bytes_to_read = min(size - total_bytes_read, UFS_BLOCK_SIZE);

        // copy the data into the provided buffer
        memcpy(bufferPtr + total_bytes_read, buf, bytes_to_read);
        total_bytes_read += bytes_to_read;
    }

    // return the total number of bytes read
    return size;
}

// rm error and mkdir/touch func point testing - new function - its helping
int LocalFileSystem::create(int parentInodeNumber, int type, string name) {
    super_t super;
    readSuperBlock(&super);

    // validate filename length
    if (name.empty() || name.size() >= DIR_ENT_NAME_SIZE) {
        return -EINVALIDNAME;
    }

    // ensure valid file type
    if (type != UFS_REGULAR_FILE && type != UFS_DIRECTORY) {
        return -EINVALIDTYPE;
    }

    // fetch parent directory's inode
    inode_t parentInode;
    if (this->stat(parentInodeNumber, &parentInode) != 0 || parentInode.type != UFS_DIRECTORY) {
        return -EINVALIDINODE;
    }

    // check if parent inode number is valid
    if (parentInodeNumber < 0 || parentInodeNumber >= super.num_inodes) {
        return -EINVALIDINODE;
    }

    // read directory entries from parent inode
    int totalEntries = parentInode.size / sizeof(dir_ent_t);
    dir_ent_t dirEntries[totalEntries];
    if (this->read(parentInodeNumber, dirEntries, parentInode.size) != parentInode.size) {
        return -EINVALIDINODE;
    }

    // check if file/directory already exists
    for (int i = 0; i < totalEntries; i++) {
        dir_ent_t entry = dirEntries[i];
        if (entry.name == name) {
            inode_t existingInode;
            stat(entry.inum, &existingInode);
            return (existingInode.type == type) ? entry.inum : -EINVALIDTYPE;
        }
    }

    // check for available disk space
    bool hasEnoughSpace = false;
    int freeBlocks = 0;
    vector<unsigned char> dataBitmap(super.data_bitmap_len * UFS_BLOCK_SIZE);
    readDataBitmap(&super, dataBitmap.data());

    for (int i = 0; i < super.num_data; ++i) {
        int byteIdx = i / 8;
        int bitIdx = i % 8;
        if (!(dataBitmap[byteIdx] & (1 << bitIdx))) {
            freeBlocks++;
        }
    }

    // determine if additional space is needed
    if ((parentInode.size % UFS_BLOCK_SIZE) == 0) {
        hasEnoughSpace = (freeBlocks >= 2);
    } else {
        hasEnoughSpace = (freeBlocks >= 1);
    }

    if (!hasEnoughSpace) {
        return -ENOTENOUGHSPACE;
    }

    // locate a free inode
    int inodeBitmapSize = super.inode_bitmap_len * UFS_BLOCK_SIZE;
    vector<unsigned char> inodeBitmap(inodeBitmapSize);
    readInodeBitmap(&super, inodeBitmap.data());

    int newInodeNum = -1;
    for (int i = 0; i < super.num_inodes; i++) {
        if (!(inodeBitmap[i / 8] & (1 << (i % 8)))) {
            inodeBitmap[i / 8] |= (1 << (i % 8));
            newInodeNum = i;
            break;
        }
    }

    if (newInodeNum == -1) {
        return -ENOTENOUGHSPACE;
    }

    // initialize new inode
    inode_t newInode;
    memset(&newInode, 0, sizeof(inode_t));
    newInode.type = type;
    newInode.size = 0;

    // allocate block if it's a directory
    int newBlockNum = -1;
    if (type == UFS_DIRECTORY) {
        for (int i = 0; i < super.num_data; ++i) {
            if (!(dataBitmap[i / 8] & (1 << (i % 8)))) {
                newBlockNum = i;
                dataBitmap[i / 8] |= (1 << (i % 8));
                break;
            }
        }

        if (newBlockNum == -1) {
            inodeBitmap[newInodeNum / 8] &= ~(1 << (newInodeNum % 8));
            writeInodeBitmap(&super, inodeBitmap.data());
            return -ENOTENOUGHSPACE;
        }

        newInode.direct[0] = super.data_region_addr + newBlockNum;
        
        // create "." and ".." directory entries
        dir_ent_t initEntries[2];
        memset(initEntries, 0, sizeof(initEntries));
        strncpy(initEntries[0].name, ".", DIR_ENT_NAME_SIZE - 1);
        initEntries[0].inum = newInodeNum;

        strncpy(initEntries[1].name, "..", DIR_ENT_NAME_SIZE - 1);
        initEntries[1].inum = parentInodeNumber;

        disk->writeBlock(newInode.direct[0], initEntries);
        newInode.size = sizeof(initEntries);
        writeDataBitmap(&super, dataBitmap.data());
    }

    // update inode region
    vector<inode_t> inodeTable(super.num_inodes);
    readInodeRegion(&super, inodeTable.data());
    inodeTable[newInodeNum] = newInode;
    writeInodeRegion(&super, inodeTable.data());

    // create new directory entry
    dir_ent_t newEntry;
    memset(&newEntry, 0, sizeof(newEntry));
    name += "\0";
    strncpy(newEntry.name, name.c_str(), DIR_ENT_NAME_SIZE);
    newEntry.inum = newInodeNum;

    // read and update parent directory data
    vector<char> parentBuffer(parentInode.size + UFS_BLOCK_SIZE - 1, 0);
    if (this->read(parentInodeNumber, parentBuffer.data(), parentInode.size) != parentInode.size) {
        inodeBitmap[newInodeNum / 8] &= ~(1 << (newInodeNum % 8));
        writeInodeBitmap(&super, inodeBitmap.data());
        return -EINVALIDINODE;
    }

    // allocate a new block if parent directory is full
    if (parentInode.size == UFS_BLOCK_SIZE) { 
        int parentNewBlockIdx = -1;
        for (int i = 0; i < super.num_data; ++i) {
            if (!(dataBitmap[i / 8] & (1 << (i % 8)))) {
                parentNewBlockIdx = i;
                dataBitmap[i / 8] |= (1 << (i % 8));
                break;
            }
        }

        if (parentNewBlockIdx == -1) {
            inodeBitmap[newInodeNum / 8] &= ~(1 << (newInodeNum % 8));
            writeInodeBitmap(&super, inodeBitmap.data());
            return -ENOTENOUGHSPACE;
        }

        for (int i = 0; i < DIRECT_PTRS; ++i) {
            if (parentInode.direct[i] == 0) {
                parentInode.direct[i] = super.data_region_addr + parentNewBlockIdx;
                break;
            }
        }

        writeDataBitmap(&super, dataBitmap.data());
    }

    // append new entry to parent directory
    memcpy(parentBuffer.data() + parentInode.size, &newEntry, sizeof(dir_ent_t));
    parentInode.size += sizeof(dir_ent_t);

    // update parent inode type and write back
    inodeTable[parentInodeNumber].type = UFS_REGULAR_FILE;
    writeInodeRegion(&super, inodeTable.data());

    if (this->write(parentInodeNumber, parentBuffer.data(), parentInode.size) != parentInode.size) {
        inodeBitmap[newInodeNum / 8] &= ~(1 << (newInodeNum % 8));
        writeInodeBitmap(&super, inodeBitmap.data());

        if (type == UFS_DIRECTORY) {
            int blockIdx = (newInode.direct[0] - super.data_region_addr) / UFS_BLOCK_SIZE;
            dataBitmap[blockIdx] &= ~(1 << ((newInode.direct[0] - super.data_region_addr) % UFS_BLOCK_SIZE));
            writeDataBitmap(&super, dataBitmap.data());
        }

        inodeTable[parentInodeNumber].type = UFS_DIRECTORY;
        writeInodeRegion(&super, inodeTable.data());

        return -ENOTENOUGHSPACE;
    }

    // finalize inode updates
    writeInodeBitmap(&super, inodeBitmap.data());
    readInodeRegion(&super, inodeTable.data());
    inodeTable[parentInodeNumber] = parentInode;
    writeInodeRegion(&super, inodeTable.data());

    return newInodeNum;
}

int LocalFileSystem::write(int inodeNumber, const void *buffer, int size) {
    // get the inode for the given file
    inode_t inode;
    if (stat(inodeNumber, &inode) < 0) {
        return -1; // file doesn't exist
    }

    // make sure it's a regular file (not a directory)
    if (inode.type != UFS_REGULAR_FILE) {
        return -1; // cannot write to non-regular files
    }

    // read the superblock, which contains filesystem metadata
    super_t super;
    readSuperBlock(&super);

    // figure out how many blocks are needed for the new data
    int blocksNeeded = (size + UFS_BLOCK_SIZE - 1) / UFS_BLOCK_SIZE; // round up
    if (blocksNeeded > DIRECT_PTRS) {
        blocksNeeded = DIRECT_PTRS; // limit to max direct pointers
        size = DIRECT_PTRS * UFS_BLOCK_SIZE; // adjust size accordingly
    }

    // load the data bitmap, which tracks free and used blocks
    int dataBitmapSize = super.data_bitmap_len * UFS_BLOCK_SIZE;
    vector<unsigned char> dataBitmap(dataBitmapSize);
    readDataBitmap(&super, dataBitmap.data());

    // count how many blocks are already allocated to this file
    int currentBlocks = 0;
    for (int i = 0; i < DIRECT_PTRS; i++) {
        if (inode.direct[i] != 0) {
            currentBlocks++;
        }
    }

    // allocate additional blocks if needed
    for (int i = currentBlocks; i < blocksNeeded; i++) {
        int newBlock = -1;
        // find a free block in the data region
        for (int j = 0; j < super.num_data; j++) {
            int byteIndex = j / 8;
            int bitIndex = j % 8;
            if ((dataBitmap[byteIndex] & (1 << bitIndex)) == 0) {
                newBlock = j;
                // mark it as used
                dataBitmap[byteIndex] |= (1 << bitIndex);
                break;
            }
        }
        if (newBlock == -1) {
            // no free blocks left, adjust file size to fit available space
            blocksNeeded = i;
            size = i * UFS_BLOCK_SIZE;
            break;
        }
        inode.direct[i] = newBlock + super.data_region_addr;
    }

    // free up blocks if the file now needs fewer than before
    for (int i = blocksNeeded; i < currentBlocks; i++) {
        if (inode.direct[i] != 0) {
            int dataBlockNum = inode.direct[i] - super.data_region_addr;
            int byteIndex = dataBlockNum / 8;
            int bitIndex = dataBlockNum % 8;
            dataBitmap[byteIndex] &= ~(1 << bitIndex); // mark block as free
            inode.direct[i] = 0;
        }
    }

    // write the file data to allocated blocks
    int bytesWritten = 0;
    const char* bufPtr = static_cast<const char*>(buffer);
    char block[UFS_BLOCK_SIZE];
    for (int i = 0; i < blocksNeeded && bytesWritten < size; i++) {
        int bytesToWrite = std::min(UFS_BLOCK_SIZE, size - bytesWritten);
        memset(block, 0, UFS_BLOCK_SIZE); // clear the block before writing
        memcpy(block, bufPtr + bytesWritten, bytesToWrite);
        disk->writeBlock(inode.direct[i], block);
        bytesWritten += bytesToWrite;
    }

    // update the inode with the new file size
    inode.size = size;

    // update the inode table on disk
    int numInodesInRegion = (super.inode_region_len * UFS_BLOCK_SIZE) / sizeof(inode_t);
    vector<inode_t> inodes(numInodesInRegion);
    readInodeRegion(&super, inodes.data());
    inodes[inodeNumber] = inode;
    writeInodeRegion(&super, inodes.data());

    // save the updated data bitmap back to disk
    writeDataBitmap(&super, dataBitmap.data());

    return bytesWritten;
}

// rm error and mkdir/touch func point testing - new function - its helping
int LocalFileSystem::unlink(int parentInodeNumber, string name) {
    super_t super;
    inode_t parent_inode;
    readSuperBlock(&super);

    // check if the parent inode exists and is a directory
    if (stat(parentInodeNumber, &parent_inode) != 0 || parent_inode.type != UFS_DIRECTORY) {
        return -EINVALIDINODE;
    }

    // prevent unlinking special directory entries or invalid names
    if (name.empty() || name == "." || name == "..") {
        return -EUNLINKNOTALLOWED;
    }
    if (name.length() > DIR_ENT_NAME_SIZE) {
        return -EINVALIDNAME;
    }

    // read the inode bitmap to verify the parent inode is valid
    unsigned char inode_bitmap[super.inode_bitmap_len * UFS_BLOCK_SIZE];
    readInodeBitmap(&super, inode_bitmap);

    if ((inode_bitmap[parentInodeNumber / 8] & (1 << (parentInodeNumber % 8))) == 0) {
        return -EINVALIDINODE;
    }

    // find the inode number of the file to be deleted
    inode_t target_inode;
    int target_inode_num = lookup(parentInodeNumber, name);
    if (target_inode_num == -ENOTFOUND) {
        return 0; // file doesn't exist, nothing to delete
    }

    stat(target_inode_num, &target_inode);

    // check if it's a non-empty directory (cannot delete unless empty)
    if (target_inode.type == UFS_DIRECTORY) {
        if (target_inode.size > static_cast<int>(2 * sizeof(dir_ent_t))) {
            return -ENOTEMPTY;
        }
    }

    // mark the inode as free in the bitmap
    inode_bitmap[target_inode_num / 8] &= ~(1 << (target_inode_num % 8));
    writeInodeBitmap(&super, inode_bitmap);

    // read the data block bitmap
    unsigned char data_bitmap[super.data_bitmap_len * UFS_BLOCK_SIZE];
    readDataBitmap(&super, data_bitmap);

    // free all blocks allocated to the file
    int total_blocks = target_inode.size / UFS_BLOCK_SIZE;
    if (target_inode.size % UFS_BLOCK_SIZE != 0) {
        total_blocks++;
    }
    for (int i = 0; i < total_blocks; i++) {
        data_bitmap[target_inode.direct[i] / 8] &= ~(1 << (target_inode.direct[i] % 8));
    }
    writeDataBitmap(&super, data_bitmap);

    // load the parent directory entries
    vector<dir_ent_t> dir_entries(parent_inode.size / sizeof(dir_ent_t));
    read(parentInodeNumber, dir_entries.data(), parent_inode.size);

    // remove the target entry from the directory
    for (auto entry = dir_entries.begin(); entry != dir_entries.end();) {
        if (entry->name == name) {
            entry = dir_entries.erase(entry);
        } else {
            ++entry;
        }
    }

    // write the updated directory entries back to disk
    unsigned char *buf_ptr = reinterpret_cast<unsigned char *>(dir_entries.data());
    for (int i = 0, remaining_size = parent_inode.size; remaining_size > 0; i++) {
        disk->writeBlock(parent_inode.direct[i], buf_ptr);
        buf_ptr += UFS_BLOCK_SIZE;
        remaining_size -= UFS_BLOCK_SIZE;
    }

    // update the parent directory size
    parent_inode.size -= sizeof(dir_ent_t);

    // update the inode table
    inode_t inode_table[super.num_inodes];
    readInodeRegion(&super, inode_table);
    inode_table[parentInodeNumber] = parent_inode;
    writeInodeRegion(&super, inode_table);

    return 0;
}



