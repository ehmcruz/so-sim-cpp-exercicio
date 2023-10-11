# Compiling in Ubuntu

Packages:
- libncurses-dev

make CONFIG_TARGET_LINUX=1

# Compiling in Windows

Considering MSYS2:
- mingw-w64-ucrt-x86_64-ncurses

make CONFIG_TARGET_WINDOWS=1

# Running in Windows

Considering a MSYS2 terminal:

unset TERM
./arq-sim-so.exe