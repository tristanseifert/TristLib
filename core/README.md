# TristLib Core
Base library that most other TristLib libraries depend on. It offers the following features:

- Header-only utilities
    - CBOR parsing, hexdump printing, etc.
- Logging
    - General logging is provided via the [plog](https://github.com/SergiusTheBest/plog) library
    - Sets up the logger (including color, if stdout is on a tty) according to some user-supplied configuration

## Dependencies
Our only dependency is on plog; if the system provides it as a shared library, we'll prefer to use that; otherwise, we include it statically ourselves.
