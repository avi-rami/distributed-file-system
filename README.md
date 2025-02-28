# Distributed File System (DS3)

A distributed file system implementation that provides a simple HTTP/REST API for file operations. This project implements a basic distributed storage system similar to Amazon S3, allowing multiple clients to access the same file system simultaneously.

## Features

- HTTP/REST API for file operations (GET, PUT, DELETE)
- Support for both files and directories
- Implicit directory creation
- Consistent error handling
- Transaction support for atomic operations

## Project Structure

The project consists of three main components:

1. **Local File System**: Implements basic file system operations on disk
2. **Distributed File System Service**: Provides HTTP/REST API endpoints
3. **Command Line Utilities**: Tools for managing and debugging the file system

## Getting Started

### Prerequisites

- C++ compiler with C++11 support
- CMake (version 3.10 or higher)
- Docker (for development environment)

### Building the Project

1. Clone the repository
2. Create a build directory:
```bash
mkdir build && cd build
```

3. Build the project:
```bash
cmake ..
make
```

### Running the Server

To start the distributed file system server:

```bash
./gunrock_web <disk_image_file>
```

The server will start on port 8080 by default.

## API Usage

The API is accessible at the `/ds3/` endpoint. Here are some example operations using curl:

```bash
# Create/Update a file
curl -X PUT -d "file contents" http://localhost:8080/ds3/path/to/file.txt

# Read a file
curl http://localhost:8080/ds3/path/to/file.txt

# List directory contents
curl http://localhost:8080/ds3/path/to/directory/

# Delete a file
curl -X DELETE http://localhost:8080/ds3/path/to/file.txt
```

## Development

The project uses Visual Studio Code with Docker for development. See the development guide for setup instructions.

## License

This project is licensed under the MIT License - see the LICENSE file for details.

