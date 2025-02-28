#include <iostream>
#include <string>
#include <stdexcept>
#include <vector>
#include "LocalFileSystem.h"
#include "Disk.h"
#include "ufs.h"

using namespace std;

int main(int argc, char *argv[]) {
    if (argc != 3) {
        cerr << "Usage: ds3cat <disk_image_file> <inode_number>" << endl;
        return 1;
    }

    int inodeNumber;
    try {
        inodeNumber = stoi(argv[2]);
    } catch (exception &e) {
        cerr << "Error: Invalid inode number format." << endl;
        return 1;
    }

    try {
        Disk disk(argv[1], UFS_BLOCK_SIZE);
        LocalFileSystem fs(&disk);

        // Check inode validity
        inode_t inode;
        int statResult = fs.stat(inodeNumber, &inode);
        if (statResult != 0) {
            cerr << "Error reading file" << endl;
            return 1;
        }

        // Ensure inode is valid for reading
        if (inode.type == UFS_DIRECTORY || inode.size == 0) {
            cerr << "Error reading file" << endl;
            return 1;
        }

        // Print file blocks
        cout << "File blocks" << endl;
        int numBlocks = (inode.size + UFS_BLOCK_SIZE - 1) / UFS_BLOCK_SIZE;
        for (int i = 0; i < numBlocks; i++) {
            if (inode.direct[i] == 0) {
                cerr << "Error: Inode contains an invalid block address." << endl;
                return 1;
            }
            cout << inode.direct[i] << endl;
        }

        // Insert a blank line after the file blocks (as expected)
        cout << endl;

        // Print file data header
        cout << "File data" << endl;

        // Read file data
        vector<char> buffer(inode.size);
        int bytesRead = fs.read(inodeNumber, buffer.data(), inode.size);
        if (bytesRead != inode.size) {
            cerr << "Error reading file" << endl;
            return 1;
        }

        // Print file data WITHOUT appending an extra newline at the end
        cout << string(buffer.begin(), buffer.end());

    } catch (const ios_base::failure &e) {
        cerr << "Error: Failed to access the disk image file." << endl;
        return 1;
    } catch (const exception &e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }

    return 0;
}
