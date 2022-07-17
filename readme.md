![plugin](https://user-images.githubusercontent.com/73447957/179381097-695492d8-f237-4faa-82b6-946ede809a0c.gif)

## Fake Unicode for Classicube

Add language support to your MCGalaxy server with this plugin suite by replacing unused CP437 symbols with Unicode symbols of your choice.

#### Requirements

* MCGalaxy 1.9.3 or newer
* ClassiCube 1.3.3 or newer (bd94a9a)

##### Installation

**MCGalaxy**
1. Place `fakeUnicodeServer.dll` in the `plugins` folder of your MCGalaxy installation
2. Create a `fakeUnicodeServer` folder in `plugins` and create a `bindings.cfg` file. An example configuration is bundled with every release
3. Install a fitting texture pack on your server (an example one is bundled with every release) 
**ClassiCube*
1. Place `fakeUnicodeSupport.so` on Linux or `fakeUnicodeSupport.dll` in the `plugins` folder of your ClassiCube installation

## Compile

#### Linux

There's a bundled makefile for Linux.

**Prerequisites**

For linux builds

- [ ] GCC

For windows builds

- [ ] x86_64-w64-mingw32-gcc
- [ ] mingw-w64-tools
