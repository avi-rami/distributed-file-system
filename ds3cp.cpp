#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <string>
#include "LocalFileSystem.h"
#include "Disk.h"
#include "ufs.h"

using namespace std;

int main(int argc, char *argv[]) {
    if (argc != 4) {
        cerr << argv[0] << ": diskimagefile src_file dst_inode" << endl;
        cerr << "for example:" << endl;
        cerr << "    $ " << argv[0] << " mydisk.img /path/to/src.txt 5" << endl;
        return 1;
    }

    // get our arguments
    string diskImageFile = argv[1];
    string srcFileName = argv[2];
    int dstInode;
    try {
        dstInode = stoi(argv[3]);
    } catch (...) {
        cerr << "Could not write to dst_file" << endl;
        return 1;
    }

    // open the src file in binary mode
    FILE *srcFile = fopen(srcFileName.c_str(), "rb");
    if (!srcFile) {
        perror("error opening src file");
        cerr << "Could not write to dst_file" << endl;
        return 1;
    }

    // get the size of the src file
    if (fseek(srcFile, 0, SEEK_END) != 0) {
        perror("error seeking src file");
        fclose(srcFile);
        cerr << "could not write to dst_file" << endl;
        return 1;
    }
    long fileSize = ftell(srcFile);
    if (fileSize < 0) {
        perror("error getting file size");
        fclose(srcFile);
        cerr << "Could not write to dst_file" << endl;
        return 1;
    }
    rewind(srcFile);

    // read the file into a buffer
    char *buffer = new(nothrow) char[fileSize];
    if (!buffer) {
        fclose(srcFile);
        cerr << "Could not write to dst_file" << endl;
        return 1;
    }
    size_t bytesRead = fread(buffer, 1, fileSize, srcFile);
    fclose(srcFile);
    if (bytesRead != static_cast<size_t>(fileSize)) {
        delete[] buffer;
        cerr << "Could not write to dst_file" << endl;
        return 1;
    }

    // create our disk and fs objects
    Disk *disk = new Disk(diskImageFile.c_str(), UFS_BLOCK_SIZE);
    LocalFileSystem *fs = new LocalFileSystem(disk);

    // write the file data to the given inode
    int bytesWritten = fs->write(dstInode, buffer, fileSize);
    delete[] buffer;

    if (bytesWritten < 0) {
        cerr << "Could not write to dst_file" << endl;
        delete fs;
        delete disk;
        return 1;
    }

    // clean up and exit
    delete fs;
    delete disk;
    return 0;
}



// #include <iostream>
// #include <string>

// #include <fcntl.h>
// #include <stdlib.h>
// #include <sys/types.h>
// #include <sys/uio.h>
// #include <unistd.h>

// #include "LocalFileSystem.h"
// #include "Disk.h"
// #include "ufs.h"

// using namespace std;

// int main(int argc, char *argv[]) {
//   if (argc != 4) {
//     cerr << argv[0] << ": diskImageFile src_file dst_inode" << endl;
//     cerr << "For example:" << endl;
//     cerr << "    $ " << argv[0] << " tests/disk_images/a.img dthread.cpp 3" << endl;
//     return 1;
//   }

//   // Parse command line arguments
//   /*
//   Disk *disk = new Disk(argv[1], UFS_BLOCK_SIZE);
//   LocalFileSystem *fileSystem = new LocalFileSystem(disk);
//   string srcFile = string(argv[2]);
//   int dstInode = stoi(argv[3]);
//   */
  
//   return 0;
// }
