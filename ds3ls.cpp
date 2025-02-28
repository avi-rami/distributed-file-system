#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <cstring>

#include "StringUtils.h"
#include "LocalFileSystem.h"
#include "Disk.h"
#include "ufs.h"

using namespace std;

// Lookup function to find an entry in a directory
int lookup(LocalFileSystem *fs, int parentInode, const string &name) {
    inode_t inode;
    if (fs->stat(parentInode, &inode) != 0) {
        return -1;
    }

    if (inode.type != UFS_DIRECTORY) {
        return -1;
    }

    // Read the entire directory content
    char buf[inode.size];
    if (fs->read(parentInode, buf, inode.size) < 0) {
        return -1;
    }

    // Search for the entry
    for (int i = 0; i < inode.size / static_cast<int>(sizeof(dir_ent_t)); ++i) {
        dir_ent_t *ent = reinterpret_cast<dir_ent_t*>(buf + i * sizeof(dir_ent_t));
        if (name == ent->name) {
            return ent->inum;
        }
    }

    return -1;  // Not found
}

// resolve a path (like "/a/b/c") to an inode number starting at the root
int resolvePath(LocalFileSystem *fs, const string &path) {
    int currentInode = UFS_ROOT_DIRECTORY_INODE_NUMBER;
    if (path == "/" || path.empty()) {
        return currentInode;
    }
    
    size_t start = (path[0] == '/') ? 1 : 0;
    while (start < path.size()) {
        size_t end = path.find('/', start);
        string token;
        if (end == string::npos) {
            token = path.substr(start);
            start = path.size();
        } else {
            token = path.substr(start, end - start);
            start = end + 1;
        }
        if (token.empty())
            continue;
            
        // Use our own lookup function instead of fs->lookup
        int nextInode = lookup(fs, currentInode, token);
        if (nextInode < 0)
            return -1;
        currentInode = nextInode;
    }
    return currentInode;
}

// List directory contents
void listDirectory(LocalFileSystem *fs, int inodeNumber, const string &path) {
    inode_t inode;
    if (fs->stat(inodeNumber, &inode) != 0) {
        return;
    }

    // If it's a regular file, print its inode and filename
    if (inode.type == UFS_REGULAR_FILE) {
        size_t pos = path.find_last_of('/');
        string name = (pos == string::npos || pos == 0) ? path : path.substr(pos + 1);
        cout << inodeNumber << "\t" << name << endl;
        return;
    }
    
    // If it's not a directory, exit
    if (inode.type != UFS_DIRECTORY) {
        return;
    }

    // Read the entire directory content
    char buf[inode.size];
    if (fs->read(inodeNumber, buf, inode.size) < 0) {
        return;
    }

    // Collect all entries
    vector<dir_ent_t> entries;
    for (int i = 0; i < inode.size / static_cast<int>(sizeof(dir_ent_t)); ++i) {
        dir_ent_t *ent = reinterpret_cast<dir_ent_t*>(buf + i * sizeof(dir_ent_t));
        entries.push_back(*ent);
    }

    // Sort entries by name
    sort(entries.begin(), entries.end(), [](const dir_ent_t &a, const dir_ent_t &b) {
        return strcmp(a.name, b.name) < 0;
    });

    // Print each entry
    for (const auto &entry : entries) {
        cout << entry.inum << "\t" << entry.name << endl;
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        cerr << argv[0] << ": diskimagefile directory" << endl;
        cerr << "for example:" << endl;
        cerr << "    $ " << argv[0] << " tests/disk_images/a.img /a/b" << endl;
        return 1;
    }

    // open disk image and create fs object
    Disk *disk = new Disk(argv[1], UFS_BLOCK_SIZE);
    LocalFileSystem *fs = new LocalFileSystem(disk);
    string path = argv[2];

    // resolve the path to an inode
    int inodeNum = resolvePath(fs, path);
    if (inodeNum < 0) {
        cerr << "Directory not found" << endl;
        delete fs;
        delete disk;
        return 1;
    }

    // list the directory contents
    listDirectory(fs, inodeNum, path);

    delete fs;
    delete disk;
    return 0;
}