#include <iostream>
#include "Disk.h"
#include "LocalFileSystem.h"

int main(int argc, char *argv[]) {
    if (argc != 4) {
        std::cerr << "Usage: ds3mkdir <disk image> <parent inode> <dir name>" << std::endl;
        return 1;
    }

    std::string diskImage = argv[1];
    int parentInode = std::stoi(argv[2]);
    std::string dirName = argv[3];

    Disk disk(diskImage, UFS_BLOCK_SIZE);
    LocalFileSystem fs(&disk);

    int result = fs.create(parentInode, UFS_DIRECTORY, dirName);
    if (result < 0) {
        std::cerr << "Error creating directory" << std::endl;
        return 1;
    }

    return 0;
}


// #include <iostream>
// #include <string>
// #include <vector>

// #include "LocalFileSystem.h"
// #include "Disk.h"
// #include "ufs.h"

// using namespace std;

// int main(int argc, char *argv[]) {
//   if (argc != 4) {
//     cerr << argv[0] << ": diskImageFile parentInode directory" << endl;
//     cerr << "For example:" << endl;
//     cerr << "    $ " << argv[0] << " a.img 0 a" << endl;
//     return 1;
//   }

//   // parse the  command line arguments.
//   Disk *disk = new Disk(argv[1], UFS_BLOCK_SIZE);
//   LocalFileSystem *fileSystem = new LocalFileSystem(disk);
//   int parentInode = stoi(argv[2]);
//   string directory = argv[3];

//   // try to create the directory.
//   int ret = fileSystem->create(parentInode, UFS_DIRECTORY, directory);
//   if (ret < 0) {
//     cerr << "Error creating directory" << endl;
//     delete fileSystem;
//     delete disk;
//     return 1;
//   }

//   // success yay!
//   delete fileSystem;
//   delete disk;
//   return 0;
// }

