# WiiMote

A library for interfacing with Nintendo Wii Remote controllers via Bluetooth and
forwarding the data on via HTTP POST with JSON payloads.

## Build Instructions

### Prerequisites

- CMake >= 3.10
- libcurl-devel
- wiiuse library installed to a known location
    - Install the `wiiuse` library with the following CMake flags:
        - `DBUILD_EXAMPLE_SDL=NO` to not build the SDL example.
        - `DBUILD_EXAMPLE=NO` to not build the example.
        - `DINSTALL_EXAMPLES=NO` to not install the examples.
        - **IMPORTANT** --> `DBUILD_SHARED_LIBS=ON` to ensure a dynamically linked library is built.  

    - Example when building wiiuse from the root of the directory:
        1. `mkdir build`
        2. `cd build`
        3. `cmake .. -DBUILD_SHARED_LIBS=ON -DBUILD_EXAMPLE_SDL=NO -DBUILD_EXAMPLE=NO -DINSTALL_EXAMPLES=NO`
        4. `sudo make install`

### Building

1. Create a build directory:
```bash
mkdir build && cd build
```

2. Configure the project:
```bash
cmake ..
```

If your wiiuse library is installed to a custom location, specify it with:
```bash
cmake -DWIIUSE_ROOT=/path/to/wiiuse/install ..
```

3. Build:
```bash
cmake --build .
```

4. (Optional) Install:
```bash
cmake --install .
```

## Project Structure

```
wiimote/
├── CMakeLists.txt          # Root CMakeLists configuration
├── README.md               # This file
├── .gitignore              # Git ignore rules
├── src/
│   └── main.c              # Main source file
├── include/
│   └── wiimote/            # Project-specific headers
└── cmake/
    └── FindWiiUse.cmake    # Custom find module for wiiuse
```

## Dependencies

- **libcurl**: HTTP client library (installed system-wide)
- **wiiuse**: Wii Remote Bluetooth library (custom location supported)
