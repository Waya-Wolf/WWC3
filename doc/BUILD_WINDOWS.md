# Building Wayawolfcoin V3 for Windows

** NOTE: replace `z` with your username and `c:` with your drive, adapt these conditions to make repeatable builds for windows (10/x64) **

## Prerequisites

- **MSYS2** (install to `C:\msys64`)
- **MINGW64** toolchain (GCC, make, etc.)

From an MSYS2 MINGW64 shell:
```
pacman -S --needed base-devel mingw-w64-x86_64-toolchain curl tar
pacman -S mingw-w64-x86_64-qt5-static mingw-w64-x86_64-miniupnpc
```

## Dependencies (build from source)

All dependencies install to `/mingw64/local/` to avoid conflicting with MSYS2 system packages.

### OpenSSL 1.0.2u
```
cd /c/Users/z/Downloads
curl -L -o openssl-1.0.2u.tar.gz https://www.openssl.org/source/old/1.0.2/openssl-1.0.2u.tar.gz
tar xzf openssl-1.0.2u.tar.gz
cd openssl-1.0.2u/
./Configure mingw64 --prefix=/mingw64/local --static no-shared no-dso no-engines
make depend && make -j4 && make install
```

### Berkeley DB 4.8.30.NC
```
cd /c/Users/z/Downloads
curl -L -o db-4.8.30.NC.tar.gz http://download.oracle.com/berkeley-db/db-4.8.30.NC.tar.gz
tar xzf db-4.8.30.NC.tar.gz
cd db-4.8.30.NC/build_unix/
../dist/configure --prefix=/mingw64/local --enable-mingw --enable-cxx --disable-shared --enable-static --disable-replication
make -j4 && make install
```

Note: `--disable-replication` avoids POSIX-only source files and socklen_t build issues with GCC 16.

### Boost 1.65.0
```
cd /c/Users/z/Downloads
curl -L -o boost_1_65_0.tar.bz2 https://archives.boost.io/release/1.65.0/source/boost_1_65_0.tar.bz2
tar xjf boost_1_65_0.tar.bz2
cd boost_1_65_0/

# Patch build.sh for GCC 16 compatibility
sed -i 's/cc=\"gcc\"/cc="gcc -std=gnu11 -Wno-implicit-function-declaration"/' build.sh
./bootstrap.sh
echo "using gcc : : g++ : <cxxflags>-std=gnu++11 ;" > user-config.jam
./b2 --user-config=user-config.jam toolset=gcc threading=multi link=static runtime-link=static variant=release --layout=system --prefix=/mingw64/local install
```

### LevelDB (bundled)
Built automatically via `makefile.mingw` or `wayawolfcoin.pro`. No manual steps.

## Source Changes

### 1. `wayawolfcoin.pro` — Qt build configuration

Point all dependency paths to `/mingw64/local/`:
```
BOOST_INCLUDE_PATH=C:/msys64/mingw64/local/include
BOOST_LIB_PATH=C:/msys64/mingw64/local/lib
BDB_INCLUDE_PATH=C:/msys64/mingw64/local/include
BDB_LIB_PATH=C:/msys64/mingw64/local/lib
OPENSSL_INCLUDE_PATH=C:/msys64/mingw64/local/include
OPENSSL_LIB_PATH=C:/msys64/mingw64/local/lib
MINIUPNPC_INCLUDE_PATH=C:/msys64/mingw64/include
MINIUPNPC_LIB_PATH=C:/msys64/mingw64/lib
```

Clear Boost library suffixes (Boost built with `--layout=system`):
```
BOOST_LIB_SUFFIX=
BOOST_THREAD_LIB_SUFFIX=
```

Disable the automatic `-mt` suffix override (lines 433-443):
```
# isEmpty(BOOST_LIB_SUFFIX) {
#     macx:BOOST_LIB_SUFFIX = -mt
#     windows:BOOST_LIB_SUFFIX = -mt
# }
# isEmpty(BOOST_THREAD_LIB_SUFFIX) {
#     win32:BOOST_THREAD_LIB_SUFFIX = _win32$$BOOST_LIB_SUFFIX
#     else:BOOST_THREAD_LIB_SUFFIX = $$BOOST_LIB_SUFFIX
# }
```

Use absolute paths for OpenSSL libraries to avoid pulling in system OpenSSL 3.x from Qt5Network's `.prl` file:
```
LIBS += $$OPENSSL_LIB_PATH/libssl.a $$OPENSSL_LIB_PATH/libcrypto.a -ldb_cxx$$BDB_LIB_SUFFIX
```

Move MINIUPNPC_LIB_PATH after OPENSSL_LIB_PATH in linker search order to ensure our local OpenSSL is found first.

### 2. `src/makefile.mingw` — Daemon build configuration

Update dependency paths:
```
INCLUDEPATHS= -I"/mingw64/local/include"
LIBPATHS= -L"/mingw64/local/lib"
```

Remove Boost suffixes (empty):
```
BOOST_SUFFIX?=
LIBS= -l boost_system -l boost_filesystem ...
```

Add C++11 flag and fix LDFLAGS:
```
CXXFLAGS=-std=gnu++11 ...
LDFLAGS=-Wl,--dynamicbase -Wl,--nxcompat -static
```

Add crypto hash object files to OBJS (blake, bmw, groestl, etc.).

Fix link order: `libleveldb.a` must be before `-l shlwapi`.

### 3. `src/net.cpp:977` — MiniUPNP API update

MiniUPNPC 2.x changed the `UPNP_GetValidIGD` signature (added wanaddr/wanaddrlen parameters):
```c
// Old (1.x): 5 args
r = UPNP_GetValidIGD(devlist, &urls, &data, lanaddr, sizeof(lanaddr));
// New (2.x): 7 args
r = UPNP_GetValidIGD(devlist, &urls, &data, lanaddr, sizeof(lanaddr), NULL, 0);
```

### 4. `src/rpcwallet.cpp:1703` — MinGW compatibility

MinGW does not provide `localtime_r`. Replace with MSVC-compatible `localtime_s`:
```c
// Before:
struct tm* dt = localtime_r(&CurTime, &Loc_MidNight);
// After:
localtime_s(&Loc_MidNight, &CurTime);
```

### 5. `.c stubs` (daemon make only)

The `makefile.mingw` pattern rule `obj/%.o: %.cpp %.c` requires a corresponding `.c` file for every `.cpp` file. Create empty stub `.c` files next to each `.cpp`:
```
for f in src/*.cpp; do
  stub="${f%.cpp}.c"
  [ -f "$stub" ] || touch "$stub"
done
```

For the Qt GUI build (via qmake), this is unnecessary since qmake handles source files directly.

## Build Commands

### Daemon (wayawolfcoind.exe)
```
cd src/
make -f makefile.mingw -j4
strip wayawolfcoind.exe
```

### Qt GUI (Wayawolfcoin-qt.exe)
```
cd /c/Users/z/Downloads/WWC3
PATH=/mingw64/qt5-static/bin:$PATH qmake wayawolfcoin.pro
make -j4
strip release/Wayawolfcoin-qt.exe
```

## Output

| File | Description |
|---|---|
| `src/wayawolfcoind.exe` | Daemon (console, 64-bit, ~9 MB stripped) |
| `release/Wayawolfcoin-qt.exe` | Qt GUI wallet (GUI, 64-bit, ~31 MB stripped, fully static) |

Both binaries are fully statically linked with no external DLL dependencies.
