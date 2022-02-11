# PET Submodule Library Build Instructions

First, cd into the outer directory containing the submodule:

    cd extern/evapotranspiration/evapotranspiration

Before library files can be built, a CMake build system must be generated.  E.g.:

    cmake -B cmake_build -S .

Note that when there is an existing directory, it may sometimes be necessary to clear it and regenerate, especially if any changes were made to the [CMakeLists.txt](CMakeLists.txt) file.

After there is build system directory, the shared library can be built using the `petbmi` CMake target. For example, the PET shared library file (i.e., the build config's `petbmi` target) can be built using:

    cmake --build cmake_build --target petbmi -- -j 2

This will build a `cmake_build/libpetbmi.<version>.<ext>` file, where the version is configured within the CMake config, and the extension depends on the local machine's operating system.    
