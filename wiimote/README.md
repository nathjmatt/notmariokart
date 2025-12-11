# WiiMote

A library for interfacing with Nintendo Wii Remote controllers via Bluetooth and
forwarding the data on via HTTP POST with JSON payloads.

## Platforms and Dependencies

Wiimote currently operates on Linux, Windows, and Mac. You will need:

### Linux
- The kernel must support Bluetooth.
- The BlueZ Bluetooth drivers must be installed.
- If compiling you'll need the BlueZ dev files (Ubuntu package `libbluetooth-dev`)

### Windows

- Bluetooth driver
- Windows SDK (with Visual Studio Community 2017+, this is very easy to build now.)

### Mac

- Mac OS X 10.2 or newer

### All Platforms

- If compiling:
    - [CMake](https://cmake.org) is needed.
    - [Wiiuse library](https://github.com/wiiuse/wiiuse)
    - [CURL Development Libraries](https://curl.se/)

#### Installing wiiuse Library

##### Linux and Mac 

1. Clone the repo
```
git clone https://github.com/wiiuse/wiiuse.git
```
2. Change directories
```
cd wiiuse
```

3. Configure CMake to ensure that the `wiiuse` library is dynamically linked. 
```
mkdir build
cd build
cmake .. -DBUILD_SHARED_LIBS=ON -DBUILD_EXAMPLE_SDL=NO -DBUILD_EXAMPLE=NO -DINSTALL_EXAMPLES=NO
```

4. Install the library system-wide.
```
sudo make install
```

##### Windows

TODO: Fill in.


## Compiling

### Linux and Mac

Run the commands yourself:

```
cmake -B build
cmake --build build
```

OR trust my Makefile:

```
make build
make compile
```

Execute the binary with:
```
./build/wiimote
```

OR 

```
make execute
```

Compile and execute the binary in one step: 

```
make run
```

### Windows

The CMake GUI can be used to generate a Visual Studio solution.


### Known Issues


Wiimote can only connect to a device if it is in discoverable mode. Enable 
discoverable mode by pressing the button on the inside of the battery cover.


#### Windows

You must first pair the Wii remote to the operating system. 

1. Open `Settings`. 
2. Go to `Bluetooth & Devices`.
3. Click on `Devices`. 
4. Scroll all the way down. There should be an option 
`More devices and printer settings` with a pop-out arrow, click it.
5. In the window that popped up, click `Add a device`, in the upper left hand corner.
6. Connect the Wii remote as normal. The names that the Wii remote should appear
as are:
- RVL-CNT-01
- RVL-CNT-01-TR


#### Mac

Wiimote may not be able to connect to the device if it has been paired to the 
operating system. Unpair it by:

1. Click on `Apple` --> `System Preferences` --> `Bluetooth`
2. Select the device from one of the following names:
    - RVL-CNT-01
    - RVL-CNT-01-TR
3. Press the X next to the device OR right-click and select `Remove`.

Enable discovery mode and try again.