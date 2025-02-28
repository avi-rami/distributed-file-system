#include <iostream>
#include "Disk.h"
#include "LocalFileSystem.h"

int main(int argc, char *argv[]) {
    if (argc != 4) {
        std::cerr << "Usage: ds3touch <disk image> <parent inode> <file name>" << std::endl;
        return 1;
    }

    std::string diskImage = argv[1];
    int parentInode = std::stoi(argv[2]);
    std::string fileName = argv[3];

    Disk disk(diskImage, UFS_BLOCK_SIZE);
    LocalFileSystem fs(&disk);

    int result = fs.create(parentInode, UFS_REGULAR_FILE, fileName);
    if (result < 0) {
        std::cerr << "Error creating file" << std::endl;
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

// // same code mostly as ds3mkdir
// int main(int argc, char *argv[]) {
//     if (argc != 4) {
//         cerr << argv[0] << ": diskImageFile parentInode file" << endl;
//         cerr << "For example:" << endl;
//         cerr << "    $ " << argv[0] << " a.img 0 file.txt" << endl;
//         return 1;
//     }

//     // parse the command line arguments
//     Disk *disk = new Disk(argv[1], UFS_BLOCK_SIZE);
//     LocalFileSystem *fileSystem = new LocalFileSystem(disk);
//     int parentInode = stoi(argv[2]);
//     string fileName = argv[3];

//     // try to create the file.
//     int ret = fileSystem->create(parentInode, UFS_REGULAR_FILE, fileName);
//     if (ret < 0) {
//         cerr << "Error creating file" << endl;
//         delete fileSystem;
//         delete disk;
//         return 1;
//     }

//     // success :D
//     delete fileSystem;
//     delete disk;
//     return 0;
// }


// #include <iostream>
// #include <string>
// #include <vector>

// #include "LocalFileSystem.h"
// #include "Disk.h"
// #include "ufs.h"

// using namespace std;

// int main(int argc, char *argv[]) {
//   if (argc != 4) {
//     cerr << argv[0] << ": diskImageFile parentInode fileName" << endl;
//     cerr << "For example:" << endl;
//     cerr << "    $ " << argv[0] << " a.img 0 a.txt" << endl;
//     return 1;
//   }

//   // Parse command line arguments
//   /*
//   Disk *disk = new Disk(argv[1], UFS_BLOCK_SIZE);
//   LocalFileSystem *fileSystem = new LocalFileSystem(disk);
//   int parentInode = stoi(argv[2]);
//   string fileName = string(argv[3]);
//   */
  
//   return 0;
// }
