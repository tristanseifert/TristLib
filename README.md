# TristLib
This repo contains a bunch of code (collectively referred to as TristLib, even though it's several separate libraries) that I reuse in many of my projects.

## What's Included?
Here's a list of all the sub-libraries provided, and their dependencies:

- **tristlib-core:** Base library for TristLib
    - *Provides:* Logging (via [plog](https://github.com/SergiusTheBest/plog))
    - *Depends:* plog (if not included statically)
- **tristlib-event:** Event loop support
    - *Provides:* Per-thread event loops, wrapping [libevent2.](http://libevent.org) Supports installing signal handlers, timers, manually triggered events, or wrapping arbitrary file descriptors and sockets. Additionally has wrappers for buffer events, which encapsulate creating, connecting, binding and listening on sockets. Sockets may support TLS, if OpenSSL is avaialble on the system.
    - *Depends:* tristlib-core, libevent2, OpenSSL (optional)
- **tristlib-rpc:** Local and remote RPC support
    - *Provides:* Local and remote RPC serers and clients running over arbitrary buffer events
    - *Depends:* tristlib-event, libcbor

## How to Use
Include the root `CMakeLists.txt` file in your project; in this case, the project defaults to building static libraries. Otherwise, if built as a top-level project, we'll build shared libraries instead.

Note that it is possible to override this default with the `TRISTLIB_BUILD_SHARED` configuration option.
