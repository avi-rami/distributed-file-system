#include <sstream>
#include <iostream>
#include <algorithm>
#include <iterator>
#include "DistributedFileSystemService.h"
#include "ClientError.h"
#include "ufs.h"
#include "WwwFormEncodedDict.h"

using namespace std;

// constructor for DistributedFileSystemService, initializing with a drive file
DistributedFileSystemService::DistributedFileSystemService(std::string driveFile) : HttpService("/ds3") {
    // create a new disk object using the provided drive file and block size
    Disk *diskObj = new Disk(driveFile, UFS_BLOCK_SIZE);
    fileSystem = new LocalFileSystem(diskObj);  // Set up the local file system with the disk
}

// function to split a given path into its parent directory and target file/directory name
std::pair<std::string, std::string> splitPath(const std::string &path) {
    size_t lastSlashPos = path.find_last_of('/');
    if (lastSlashPos == std::string::npos || lastSlashPos == 0) {
        // root directory case
        return std::pair<std::string, std::string>("/", path);
    }
    // return parent path and target name
    return std::pair<std::string, std::string>(path.substr(0, lastSlashPos), path.substr(lastSlashPos + 1));
}

// function to resolve the parent inode number based on the provided path
int resolveParentInode(LocalFileSystem *fileSystem, const std::string &parentPath) {
    int currentInode = 0;  // start from the root inode
    if (parentPath == "/") {
        return currentInode;  // return root inode if the path is the root
    }
    
    std::string remainingPath = parentPath.substr(1);  // remove leading slash
    size_t pos;
    while ((pos = remainingPath.find('/')) != std::string::npos) {
        // process each directory part
        std::string part = remainingPath.substr(0, pos);
        remainingPath = remainingPath.substr(pos + 1);
        currentInode = fileSystem->lookup(currentInode, part);
        if (currentInode < 0) {
            return -EINVALIDINODE;  // return error if part of the directory is not found
        }
    }
    if (!remainingPath.empty()) {
        // process last part of the path if any
        currentInode = fileSystem->lookup(currentInode, remainingPath);
    }
    return currentInode;
}

// handle GET requests: retrieve file/directory information
void DistributedFileSystemService::get(HTTPRequest *request, HTTPResponse *response) {
    std::string requestedPath = request->getPath();
    // get the path from the HTTP request (e.g., /ds3/a/b/c.txt)
    
    int parentInodeId = 0;  // start from root inode
    std::string targetFileName = requestedPath;
    
    if (requestedPath != "/") {
        // split the requested path into parent and target
        std::pair<std::string, std::string> pathParts = splitPath(requestedPath);
        std::string parentDirectoryPath = pathParts.first;
        targetFileName = pathParts.second;
        
        if (targetFileName.empty() && requestedPath.back() == '/') {
            // handle directory listing if path ends with '/'
            parentDirectoryPath = requestedPath.substr(0, requestedPath.find_last_of('/'));
            targetFileName = "";
        }
        parentInodeId = resolveParentInode(fileSystem, parentDirectoryPath);
        if (parentInodeId < 0) {
            response->setStatus(404);
            response->setBody("Parent directory not found.");
            return;
        }
    }
    
    if (targetFileName.empty()) {
        // handle directory listing
        inode_t parentInode;
        if (fileSystem->stat(parentInodeId, &parentInode) < 0) {
            response->setStatus(500);
            response->setBody("Failed to retrieve parent inode information.");
            return;
        }
        
        // read the directory contents
        std::vector<char> directoryBuffer(parentInode.size, 0);
        int bytesRead = fileSystem->read(parentInodeId, directoryBuffer.data(), parentInode.size);
        if (bytesRead < 0) {
            response->setStatus(500);
            response->setBody("Failed to read directory.");
            return;
        }
        
        dir_ent_t *directoryEntries = reinterpret_cast<dir_ent_t *>(directoryBuffer.data());
        std::vector<std::string> directoryContents;
        for (size_t i = 0; i < parentInode.size / sizeof(dir_ent_t); i++) {
            // process valid directory entries
            if (directoryEntries[i].inum != -1 && directoryEntries[i].name[0] != '\0' &&
                std::string(directoryEntries[i].name) != "." && std::string(directoryEntries[i].name) != "..") {
                std::string entryName = directoryEntries[i].name;
                inode_t entryInode;
                if (fileSystem->stat(directoryEntries[i].inum, &entryInode) == 0 && entryInode.type == UFS_DIRECTORY) {
                    entryName += "/";  // append '/' to directories
                }
                directoryContents.push_back(entryName);
            }
        }
        
        std::sort(directoryContents.begin(), directoryContents.end());
        
        std::string responseBody;
        for (const auto &entry : directoryContents) {
            responseBody += entry + "\n";
        }
        response->setStatus(200);
        response->setBody(responseBody);
        return;
    }
    
    // lookup inode for the specified file
    int fileInodeId = fileSystem->lookup(parentInodeId, targetFileName);
    if (fileInodeId < 0) {
        response->setStatus(404);
        response->setBody("File or directory not found.");
        return;
    }
    
    inode_t fileInode;
    if (fileSystem->stat(fileInodeId, &fileInode) < 0) {
        response->setStatus(500);
        response->setBody("Failed to retrieve inode information.");
        return;
    }
    
    if (fileInode.type == UFS_DIRECTORY) {
        // handle directory (similar to listing)
        std::vector<char> directoryBuffer(fileInode.size);
        int bytesRead = fileSystem->read(fileInodeId, directoryBuffer.data(), fileInode.size);
        if (bytesRead < 0) {
            response->setStatus(500);
            response->setBody("Failed to read directory.");
            return;
        }
        
        dir_ent_t *directoryEntries = reinterpret_cast<dir_ent_t *>(directoryBuffer.data());
        std::vector<std::string> directoryContents;
        for (size_t i = 0; i < fileInode.size / sizeof(dir_ent_t); i++) {
            // process valid directory entries
            if (directoryEntries[i].inum != -1 && directoryEntries[i].name[0] != '\0' &&
                std::string(directoryEntries[i].name) != "." && std::string(directoryEntries[i].name) != "..") {
                std::string entryName = directoryEntries[i].name;
                inode_t entryInode;
                if (fileSystem->stat(directoryEntries[i].inum, &entryInode) == 0 && entryInode.type == UFS_DIRECTORY) {
                    entryName += "/";  // append '/' to directories
                }
                directoryContents.push_back(entryName);
            }
        }
        
        std::sort(directoryContents.begin(), directoryContents.end());
        
        std::string responseBody;
        for (const auto &entry : directoryContents) {
            responseBody += entry + "\n";
        }
        response->setStatus(200);
        response->setBody(responseBody);
    } else if (fileInode.type == UFS_REGULAR_FILE) {
        // handle regular file reading
        char fileBuffer[UFS_BLOCK_SIZE];
        int bytesRead = fileSystem->read(fileInodeId, fileBuffer, sizeof(fileBuffer));
        if (bytesRead < 0) {
            response->setStatus(500);
            response->setBody("Failed to read file.");
        } else {
            response->setStatus(200);
            response->setBody(std::string(fileBuffer, bytesRead));
        }
    } else {
        response->setStatus(500);
        response->setBody("Invalid inode type.");
    }
}

// handle PUT requests: upload a file or create a directory
void DistributedFileSystemService::put(HTTPRequest *request, HTTPResponse *response) {
    std::string requestedPath = request->getPath();
    std::string fileContent = request->getBody();
    
    // split the requested path into parent directory and file name
    std::pair<std::string, std::string> pathParts = splitPath(requestedPath);
    std::string parentDirectoryPath = pathParts.first;
    std::string fileName = pathParts.second;
    
    // resolve or create the parent directories
    int parentInodeId = resolveParentInode(fileSystem, parentDirectoryPath);
    if (parentInodeId < 0) {
        // try to create missing parent directories
        parentInodeId = 0;
        std::string remainingPath = parentDirectoryPath.substr(1);
        size_t pos;
        while ((pos = remainingPath.find('/')) != std::string::npos) {
            std::string part = remainingPath.substr(0, pos);
            remainingPath = remainingPath.substr(pos + 1);
            int nextInodeId = fileSystem->lookup(parentInodeId, part);
            if (nextInodeId < 0) {
                nextInodeId = fileSystem->create(parentInodeId, UFS_DIRECTORY, part);
                if (nextInodeId < 0) {
                    response->setStatus(500);
                    response->setBody("Failed to create parent directory: " + part);
                    return;
                }
            }
            parentInodeId = nextInodeId;
        }
        if (!remainingPath.empty()) {
            int nextInodeId = fileSystem->lookup(parentInodeId, remainingPath);
            if (nextInodeId < 0) {
                nextInodeId = fileSystem->create(parentInodeId, UFS_DIRECTORY, remainingPath);
                if (nextInodeId < 0) {
                    response->setStatus(500);
                    response->setBody("Failed to create parent directory: " + remainingPath);
                    return;
                }
            }
            parentInodeId = nextInodeId;
        }
    }
    
    // create or update the file
    int fileInodeId = fileSystem->create(parentInodeId, UFS_REGULAR_FILE, fileName);
    if (fileInodeId < 0) {
        response->setStatus(500);
        response->setBody("Failed to create file.");
        return;
    }
    
    // write the content into the file
    int bytesWritten = fileSystem->write(fileInodeId, fileContent.data(), fileContent.size());
    if (bytesWritten < 0) {
        response->setStatus(500);
        response->setBody("Failed to write file contents.");
    } else {
        response->setStatus(201);
        response->setBody("File created successfully.");
    }
}

// handle DELETE requests: remove a file or directory
void DistributedFileSystemService::del(HTTPRequest *request, HTTPResponse *response) {
    std::string requestedPath = request->getPath();
    
    // split the path into parent directory and target file/directory name
    std::pair<std::string, std::string> pathParts = splitPath(requestedPath);
    std::string parentDirectoryPath = pathParts.first;
    std::string targetName = pathParts.second;
    
    // resolve the parent inode
    int parentInodeId = resolveParentInode(fileSystem, parentDirectoryPath);
    if (parentInodeId < 0) {
        response->setStatus(404);
        response->setBody("Parent directory not found.");
        return;
    }
    
    // lookup the target inode
    int targetInodeId = fileSystem->lookup(parentInodeId, targetName);
    if (targetInodeId < 0) {
        response->setStatus(404);
        response->setBody("File or directory not found.");
        return;
    }
    
    // get inode information
    inode_t targetInode;
    if (fileSystem->stat(targetInodeId, &targetInode) < 0) {
        response->setStatus(500);
        response->setBody("Failed to retrieve inode information.");
        return;
    }
    
    if (targetInode.type == UFS_DIRECTORY) {
        // check if the directory is empty
        std::vector<char> directoryBuffer(targetInode.size);
        if (fileSystem->read(targetInodeId, directoryBuffer.data(), targetInode.size) < 0) {
            response->setStatus(500);
            response->setBody("Failed to read directory.");
            return;
        }
        dir_ent_t *directoryEntries = reinterpret_cast<dir_ent_t *>(directoryBuffer.data());
        int validEntries = 0;
        for (size_t i = 0; i < targetInode.size / sizeof(dir_ent_t); i++) {
            if (directoryEntries[i].inum != -1 && directoryEntries[i].name[0] != '\0') {
                std::string entryName = directoryEntries[i].name;
                if (entryName != "." && entryName != "..") {
                    validEntries++;
                }
            }
        }
        if (validEntries > 0) {
            response->setStatus(400);
            response->setBody("Directory is not empty.");
            return;
        }
    }
    
    // remove the file or directory
    int result = fileSystem->unlink(parentInodeId, targetName);
    if (result < 0) {
        response->setStatus(500);
        response->setBody("Failed to delete file or directory.");
    } else {
        response->setStatus(200);
        response->setBody("File or directory deleted successfully.");
    }
}