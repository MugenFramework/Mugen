# Mugen Demon Agent

Demon is the primary Mugen agent, written in C and assembly.

## Directories

### src/asm
Assembly code - return address stack spoofing.

### src/core
Core functions: connect to server, dynamically load Win32 APIs, syscalls.

### src/crypt
Encryption and decryption functions.

### src/inject
Injection functions and utilities.

### src/main
Entry points:

- `MainExe.c` - PE executable entry point
- `MainSvc.c` - Service executable entry point
- `MainDll.c` - DLL entry point

## Note on CMakeLists.txt

The `CMakeLists.txt` in this directory is for IDE support only (CLion/CMake-based IDEs). It is not used for building the Demon payload - payload compilation is handled by the teamserver at runtime.
