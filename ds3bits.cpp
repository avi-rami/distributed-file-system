#include <iostream>
#include <string>
#include <cstring>
#include <algorithm>

#include "LocalFileSystem.h"
#include "Disk.h"
#include "ufs.h"

using namespace std;

int main(int argc, char *argv[]) {
    if (argc != 2) {
        cerr << argv[0] << ": diskImageFile" << endl;
        return 1;
    }

    // define block size for the filesystem
    int blockSize = UFS_BLOCK_SIZE;

    // create disk and filesystem objects
    Disk disk(argv[1], blockSize);
    LocalFileSystem fs(&disk);

    // read the superblock to get filesystem metadata
    super_t super;
    fs.readSuperBlock(&super);

    // print superblock information
    cout << "Super" << endl;
    cout << "inode_region_addr " << super.inode_region_addr << endl;
    cout << "inode_region_len " << super.inode_region_len << endl;

    // calculate the total number of inodes in the inode region
    int computedNumInodes = (super.inode_region_len * blockSize) / sizeof(inode_t);
    cout << "num_inodes " << computedNumInodes << endl;

    cout << "data_region_addr " << super.data_region_addr << endl;
    cout << "data_region_len " << super.data_region_len << endl;

    // calculate the number of data blocks in the data region
    int computedNumData = super.data_region_len;
    cout << "num_data " << computedNumData << endl;

    cout << endl;

    // read inode and data bitmaps from the filesystem
    int inodeBitmapSize = super.inode_bitmap_len * blockSize;
    int dataBitmapSize  = super.data_bitmap_len  * blockSize;

    unsigned char* inodeBitmap = new unsigned char[inodeBitmapSize];
    unsigned char* dataBitmap  = new unsigned char[dataBitmapSize];

    fs.readInodeBitmap(&super, inodeBitmap);
    fs.readDataBitmap(&super, dataBitmap);

    // determine the number of bytes to print based on actual inodes and data blocks
    int numInodeBytes = (computedNumInodes + 7) / 8;
    int numDataBytes  = (computedNumData  + 7) / 8;

    // print inode bitmap
    cout << "Inode bitmap" << endl;
    for (int i = 0; i < numInodeBytes; i++) {
        cout << static_cast<unsigned int>(inodeBitmap[i]) << " ";
    }
    cout << endl << endl;

    // print data bitmap
    cout << "Data bitmap" << endl;
    for (int i = 0; i < numDataBytes; i++) {
        cout << static_cast<unsigned int>(dataBitmap[i]) << " ";
    }
    cout << endl;

    // free allocated memory
    delete[] inodeBitmap;
    delete[] dataBitmap;

    return 0;
}
