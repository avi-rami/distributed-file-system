#include <iostream>
#include <string>
#include "LocalFileSystem.h"
#include "Disk.h"
#include "ufs.h"

using namespace std;

int main(int argc, char *argv[]) {
    if (argc != 4) {
        cerr << argv[0] << ": diskimagefile parentinode name" << endl;
        cerr << "for example:" << endl;
        cerr << "    $ " << argv[0] << " a.img 0 file.txt" << endl;
        return 1;
    }
    
    int parentInode;
    try {
        parentInode = stoi(argv[2]);
    } catch (...) {
        cerr << "Error removing entry" << endl;
        return 1;
    }
    
    string entryName = argv[3];
    
    try {
        // use automatic objects so destructors are called properly
        Disk disk(argv[1], UFS_BLOCK_SIZE);
        LocalFileSystem fs(&disk);
        
        int ret = fs.unlink(parentInode, entryName);
        if (ret < 0) {
            cerr << "Error removing entry" << endl;
            return 1;
        }
    } catch (...) {
        cerr << "Error removing entry" << endl;
        return 1;
    }
    
    return 0;
}




// #include <iostream>
// #include <string>
// #include "LocalFileSystem.h"
// #include "Disk.h"
// #include "ufs.h"

// using namespace std;

// int main(int argc, char *argv[]) {
//     if (argc != 4) {
//         cerr << argv[0] << ": diskimagefile parentinode name" << endl;
//         cerr << "for example:" << endl;
//         cerr << "    $ " << argv[0] << " a.img 0 file.txt" << endl;
//         return 1;
//     }
    
//     // get command line args
//     Disk *disk = new Disk(argv[1], UFS_BLOCK_SIZE);
//     LocalFileSystem *fs = new LocalFileSystem(disk);
    
//     int parentInode;
//     try {
//         parentInode = stoi(argv[2]);
//     } catch (...) {
//         cerr << "Error removing entry" << endl;
//         delete fs;
//         delete disk;
//         return 1;
//     }
    
//     string entryName = argv[3];
    
//     // try to remove the entry
//     int ret = fs->unlink(parentInode, entryName);
//     if (ret < 0) {
//         cerr << "Error removing entry" << endl;
//         delete fs;
//         delete disk;
//         return 1;
//     }
    
//     // cleanup and exit
//     delete fs;
//     delete disk;
//     return 0;
// }


// #include <iostream>
// #include <string>
// #include <algorithm>
// #include <cstring>

// #include "LocalFileSystem.h"
// #include "Disk.h"
// #include "ufs.h"

// using namespace std;

// // code below parses the command line args/inputs
// // pay attention to buffer overflow and memory stuff when it happens
// int main(int argc, char *argv[]) {
//   if (argc != 4) {
//     cerr << argv[0] << ": diskImageFile parentInode entryName" << endl;
//     return 1;
//   }

//   // // Parse command line arguments
//   // Disk *disk = new Disk(argv[1], UFS_BLOCK_SIZE); // takes disk image, initializes disk
//   // LocalFileSystem *fileSystem = new LocalFileSystem(disk); // take disk image and initialize local file system
//   // int parentInode = stoi(argv[2]); //
//   // string entryName = string(argv[3]);

//   // if (LocalFileSystem -> unlink

//   return 0;
// }
