# Socket Project

This document provides instructions for setting up, building, and running the Socket project.

## Prerequisites

Ensure that your system is updated and that the required libraries are installed. Use the following commands:

```bash
sudo apt-get update && sudo apt-get install -y \
    g++ \
    libcpprest-dev \
    libboost-all-dev \
    libssl-dev \
    libcrypto++-dev \
    cmake
```

### Key Libraries
1. **g++**: The GNU C++ compiler for building the application.
2. **libcpprest-dev**: For RESTful API support.
3. **libboost-all-dev**: A collection of peer-reviewed portable C++ source libraries.
4. **libssl-dev**: Provides SSL and TLS support.
5. **libcrypto++-dev**: A library for cryptography.
6. **cmake**: A build-system generator tool.

## Generating OpenSSL Keys

To use SSL/TLS with the project, you need to generate the necessary SSL keys. Follow these steps:

1. Generate a private key:
   ```bash
   openssl genrsa -out private.key 2048
   ```

2. Create a certificate signing request (CSR):
   ```bash
   openssl req -new -key private.key -out request.csr
   ```

3. Generate a self-signed certificate:
   ```bash
   openssl x509 -req -days 365 -in request.csr -signkey private.key -out certificate.crt
   ```

You will now have three files:
- `private.key` (private key)
- `request.csr` (certificate signing request)
- `certificate.crt` (self-signed certificate)

## Compiling the Program

To compile the server program, use the following command:

```bash
/usr/bin/g++ -fdiagnostics-color=always -g server.cpp -o server -I/usr/include -L/usr/lib -lssl -lcrypto -I/in.h -std=c++17
```

### Key Flags Explained
- `-fdiagnostics-color=always`: Enables colorized output for diagnostics.
- `-g`: Includes debug information in the build.
- `server.cpp`: The source code file.
- `-o server`: Specifies the output file name.
- `-I/usr/include`: Includes the directory for header files.
- `-L/usr/lib`: Links the directory for libraries.
- `-lssl`: Links the OpenSSL library for SSL/TLS.
- `-lcrypto`: Links the library for cryptographic functions.
- `-I/in.h`: Specifies the directory for additional header files.
- `-std=c++17`: Specifies the C++17 standard for compilation.

## Running the Program

Once compiled, you can start the server using:

```bash
./server
```

Ensure that your generated SSL keys (`private.key` and `certificate.crt`) are in the same directory or specified correctly in your code.

## Notes
- Make sure to handle SSL/TLS certificates securely and never expose private keys in public repositories.
- Always test the setup in a controlled environment before deploying in production.

For any issues, please open an issue in the project repository or contact the maintainer.

