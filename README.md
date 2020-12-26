Agat Emulator
=============

This is a fork of the [Agat Emulator] project on [SourceForge].

[Agat Emulator]: https://sourceforge.net/projects/agatemulator/
[SourceForge]: https://sourceforge.net/

Building
========

Prerequisites
-------------

To build Agat Emulator you'll need:

- Visual Studio 2010 or later
- CMake 3.8 or later, Windows version
- Optional: UPX executable compressor
- Optional: HTML Help Workshop
- Optional: Inno Setup

Note that currently, GCC cannot be used to build Agat Emulator because the
emulator sources are not fully compatible with ANSI C.

Procedure
---------

### 1. Create a build directory

Create a build directory in a convenient location. The rest of this document
will assume that your build directory is called `build` and is located on the
same level as the agat-emulator source tree:

```
agat-emulator
|
+ agat-emulator
|  |
|  + .git
|  + CMakeLists.txt
|  ...
|
\ build
   |
   + CMakeCache.txt
   ...
```

This is conventient for long-term development as it leaves the source tree
clean and searchable with command-line tools like `grep` and `ag`.

### 2. Run CMake

Run CMake in the build directory. In the simplest case it can be as easy as

```
cd build
cmake ../agat-emulator
```

Please see [potential configuration issues](#potential-configuration-issues)
for a discussion on what can go wrong and how to fix it.

### 3. Open the project in Visual Studio

Start Visual Studio by opening the build directory in Windows Explorer and
double-clicking the generated `AgatEmulator.sln`. Alternatively start Visual
Studio from the Start menu or from a Windows shortcut and use File > Open >
Project/Solution.

DO NOT start Visual Studio or Windows Explorer from your Cygwin- or MSYS-based
command line. This can result in weird build failures because Visual Studio
gets confused by duplicate environment variables with the same name but
different case like e.g. `Path` vs `PATH`.

### 4. Build and run the project

Set the `interface` project as your startup project by right-clicking it in
Solution Explorer and selecting Set as StartUp Project. Now you can build, run
and debug the emulator.

### 5. Build an installer

To build an installer, first all release files need to be collected in one
place. For that you need to tell CMake where you want them. E.g. to create a
release image in a `release` directory next to the `build` directory, run the
following in the `build` directory:

```
cmake . -DCMAKE_INSTALL_PREFIX=../release
```

Alternatively the `CMAKE_INSTALL_PREFIX` variable can be configured directly
in `build/CMakeCache.txt` or using CMake GUI. Make sure that the path you
configure is absolute. When set from the command line CMake converts the path
to absolute for you.

You'll probably want to switch to Release solution configuration in Visual
Studio. Then build the `INSTALL` project. The `release` directory will be
populated with all the necessary files.

Finally use Inno Setup to build the installer. Note that you'll have to supply
the release version manually on the command line:

```
iscc -dAppVer=1.29.2 release/setup/setup.iss
```

The installer will be created in the `release` directory.

Potential Configuration Issues
------------------------------

### Executable compression

CMake configuration may fail with a "UPX is required" message. This means that
CMake wasn't able to find the `upx.exe` executable compressor. You have a few
options:

- Make sure that upx.exe is in your PATH and run CMake again
- Set the `UPX` CMake variable to the exact path to your upx.exe via CMake GUI,
  by editing build/CMakeCache.txt, or from the command line by running
  ```
  cmake . -DUPX=C:/path/to/upx.exe
  ```
- Disable executable compression by setting the `COMPRESS_EXECUTABLE` CMake
  variable to `NO` using any method listed above

### CHM Documentation

CMake configuration may fail with an "HTML Help Compiler is required" message.
This means that CMake wasn't able to find the `hhc.exe` documentation
compiler. As with UPX you can make sure `hhc.exe` is in your path, set the
`HHC` CMake variable to the full path of the `hhc.exe`, or disable
documentation generation by setting `BUILD_CHM_DOCS` CMake variable to `NO`.

### Windows XP support

Visual Studio 2008 was the last Visual Studio version which supported building
for Windows XP out of the box. Executables built with later versions of Visual
Studio will not run on Windows XP by default.

An additional toolset needs to be installed to enable XP support. E.g. in
Visual Studio 2017 Community this is done by running the Visual Studio
installer in Modify mode and ticking the Windows XP Support for C++ in the
Installation Details pane. This installs "Tools for creating Windows XP
applications using ... compiler toolset(v140)" and adds the `v140_xp` toolset.
The Visual Studio installer can be found in Apps & Features in Windows control
panel.

To use this toolset CMake needs to run with the `-Tv140_xp` option. Note that
the very first CMake run in a build directory must include this option. If you
already ran CMake without it you'll have to start over by deleting everything
in the build directory or by using a different build directory.
