# Building Wayawolfcoin V3 for Windows

> **NOTE:** replace `z` with your username and `c:` with your drive. Tested on
> Windows 10/11 x64 with **MSYS2 MINGW64 (GCC 16.x)**. The steps below are the
> exact commands that produced working `wayawolfcoind.exe` and
> `Wayawolfcoin-qt.exe` binaries.

## Prerequisites

- **MSYS2** installed to `C:\msys64` (x86_64).
- **MINGW64** toolchain, Qt 5 static, miniupnpc, and download tools.

From an **MSYS2 MINGW64** shell (`C:\msys64\msys2_shell.cmd -mingw64`):

```
pacman -S --needed base-devel mingw-w64-x86_64-toolchain curl tar
pacman -S mingw-w64-x86_64-qt5-static mingw-w64-x86_64-miniupnpc
```

Verify the toolchain:

```
which gcc && gcc --version | head -1                          # expect GCC 16.x
/mingw64/qt5-static/bin/qmake --version                       # expect Qt 5.15.x
```

## Dependencies (build from source)

All dependencies install to `/mingw64/local/` (i.e. `C:\msys64\mingw64\local`)
to avoid conflicting with MSYS2 system packages. The source tarballs live under
`/c/Users/<user>/Downloads`.

### 1. OpenSSL 1.0.2u

> Pitfalls solved here: GCC 16 defaults to C23 and turns legacy C constructs
> into hard errors, and `make`'s incremental archive step silently produces an
> incomplete `libcrypto.a` (missing `bn`, `stack`, `err`, `objects` members).
> `make install` also fails at the `openssl.exe` app stage, which the coin does
> not need.

```
cd /c/Users/z/Downloads
curl -L -o openssl-1.0.2u.tar.gz https://www.openssl.org/source/old/1.0.2/openssl-1.0.2u.tar.gz
tar xzf openssl-1.0.2u.tar.gz
cd openssl-1.0.2u/
./Configure mingw64 --prefix=/mingw64/local --openssldir=/mingw64/local/ssl --static -fcommon
```

**Patch the Makefile** (append GCC-16 compat flags to the `CFLAG=` line so the
old C code compiles under C23 defaults):

```
sed -i 's|^CFLAG=.*|& -std=gnu11 -Wno-incompatible-pointer-types -Wno-pointer-sign -Wno-int-conversion -Wno-deprecated-non-prototype|' Makefile
```

**Build:**

```
make -j4
```

This compiles all of `crypto/`, `ssl/` and `engines/`. The final
`apps/openssl.exe` link may fail on undefined `ENGINE_load_*` symbols — that is
**expected and fine**, the coin never links `openssl.exe`.

**Verify + fix the static archive** (required — the make-generated archive can
be missing whole subdirectory objects):

```
ar t libcrypto.a | grep -c bn_lib.o     # must print >= 1, else rebuild the archive:
rm -f libcrypto.a libssl.a
ar r libcrypto.a crypto/*.o crypto/*/*.o
ranlib libcrypto.a
ar r libssl.a ssl/*.o
ranlib libssl.a
```

**Install** (headers via `install_sw`, libs copied manually; the failing
`apps/openssl.exe install` step is harmless):

```
make install_sw   # installs headers to /mingw64/local/include/openssl (fails at apps stage, ignore)
cp libcrypto.a libssl.a /mingw64/local/lib/
ranlib /mingw64/local/lib/libcrypto.a /mingw64/local/lib/libssl.a
ls /mingw64/local/include/openssl/ssl.h /mingw64/local/lib/libcrypto.a /mingw64/local/lib/libssl.a
```

### 2. Berkeley DB 4.8.30.NC

> Pitfall: `make install` enters an **infinite recursion**
> (`make -f Makefile.bdb install` loop) — kill it and install manually.

```
cd /c/Users/z/Downloads
curl -L -o db-4.8.30.NC.tar.gz http://download.oracle.com/berkeley-db/db-4.8.30.NC.tar.gz
tar xzf db-4.8.30.NC.tar.gz
cd db-4.8.30.NC/build_unix/
../dist/configure --prefix=/mingw64/local --enable-mingw --enable-cxx --disable-shared --enable-static --disable-replication
make -j4
```

`--disable-replication` avoids POSIX-only source files and `socklen_t` build
issues with GCC 16.

**Install manually** (do NOT run `make install`):

```
cp libdb.a libdb_cxx.a /mingw64/local/lib/
cp db.h db_cxx.h db_config.h /mingw64/local/include/
ls /mingw64/local/lib/libdb.a /mingw64/local/lib/libdb_cxx.a
```

### 3. Boost 1.65.0

> Pitfalls solved here: the Boost.Build engine must be compiled with GCC-16
> compat flags, and `b2` on Windows cannot resolve the bare command `g++` from
> PATH (its glob misses `g++.exe`), so `user-config.jam` must give the
> **absolute** compiler path. `b2` is a native Windows binary and must be run
> with a **Windows-style PATH** (from PowerShell), not the Unix PATH of an MSYS
> shell.

```
cd /c/Users/z/Downloads
curl -L -o boost_1_65_0.tar.bz2 https://archives.boost.io/release/1.65.0/source/boost_1_65_0.tar.bz2
tar xjf boost_1_65_0.tar.bz2
cd boost_1_65_0/
```

**Patch the engine for GCC 16** (`tools/build/src/engine/build.sh:147` and
`tools/build/src/engine/build.jam:218`):

```
sed -i 's|gcc -DNT|gcc -DNT -std=gnu11 -Wno-implicit-function-declaration -Wno-implicit-int -Wno-incompatible-pointer-types -Wno-int-conversion -Wno-pointer-sign -Wno-deprecated-non-prototype|' tools/build/src/engine/build.sh
sed -i 's|-pedantic -fno-strict-aliasing|-pedantic -fno-strict-aliasing -std=gnu11 -Wno-implicit-function-declaration -Wno-implicit-int -Wno-incompatible-pointer-types -Wno-int-conversion -Wno-pointer-sign -Wno-deprecated-non-prototype -fcommon|' tools/build/src/engine/build.jam
```

**Bootstrap** (builds the engine; `b2.exe` ends up at the boost *root* and in
`tools/build/src/engine/bin.ntx86_64/`):

```
./bootstrap.sh
ls -la b2.exe    # must exist
```

**Configure the compiler** — absolute path is mandatory:

```
echo 'using gcc : : "C:/msys64/mingw64/bin/g++" : <cxxflags>-std=gnu++11 ;' > user-config.jam
```

**Build and install the required libraries** — run this from a **PowerShell**
prompt (so `b2` sees a Windows PATH), from the boost root:

```powershell
$env:Path = "C:\msys64\mingw64\bin;C:\msys64\usr\bin;C:\msys64\bin;$env:Path"
cd C:\Users\z\Downloads\boost_1_65_0
.\b2.exe --user-config=user-config.jam toolset=gcc threading=multi link=static runtime-link=static variant=release --layout=system --prefix=C:/msys64/mingw64/local --with-system --with-filesystem --with-program_options --with-thread --with-chrono -j2 install
```

Expected result:

```
ls /mingw64/local/lib/libboost_system.a /mingw64/local/lib/libboost_filesystem.a \
   /mingw64/local/lib/libboost_program_options.a /mingw64/local/lib/libboost_thread.a \
   /mingw64/local/lib/libboost_chrono.a
ls /mingw64/local/include/boost/version.hpp
```

### 4. LevelDB (bundled)

The in-tree `src/leveldb/` is built automatically by both `makefile.mingw` and
`wayawolfcoin.pro`. No manual steps required. Manual fallback, from
`src/leveldb/`:

```
make TARGET_OS=NATIVE_WINDOWS CC=gcc CXX=g++ libleveldb.a libmemenv.a
```

## Source tree state

The repository already contains all the integration changes described below.
**Verify** they are present rather than re-applying them:

- `wayawolfcoin.pro` — dependency paths point at `/mingw64/local` for
  BOOST/BDB/OPENSSL and `/mingw64` for MINIUPNPC; `BOOST_LIB_SUFFIX` and
  `BOOST_THREAD_LIB_SUFFIX` are empty; the `-mt` suffix override block
  (lines ~434-443) is commented out; OpenSSL is linked by absolute path
  (`LIBS += $$OPENSSL_LIB_PATH/libssl.a $$OPENSSL_LIB_PATH/libcrypto.a`).
- `src/makefile.mingw` — `INCLUDEPATHS`/`LIBPATHS` point at `/mingw64/local`,
  empty `BOOST_SUFFIX`, `CXXFLAGS=-std=gnu++11`, static `LDFLAGS`, crypto hash
  objects in `OBJS`, and `libleveldb.a` before `-l shlwapi` in `LIBS`.
- `src/net.cpp:977` — already uses the MiniUPNP 2.x seven-arg
  `UPNP_GetValidIGD(devlist, &urls, &data, lanaddr, sizeof(lanaddr), NULL, 0)`.
- `src/rpcwallet.cpp:1703` — already uses `localtime_s(&Loc_MidNight, &CurTime)`
  (MinGW has no `localtime_r`).
- `src/*.c` — an empty `.c` stub exists next to every `.cpp` (the
  `obj/%.o: %.cpp %.c` pattern rule in `makefile.mingw` requires both). **Do not
  delete them.**

**Required fix — `src/makefile.mingw:7`:** the line is `USE_UPNP:=- ` with a
**trailing space**, which makes `USE_UPNP` equal `"- "` instead of `"-"`. The
`ifneq (${USE_UPNP}, -)` check then wrongly enables UPnP and the broken macro
`-DUSE_UPNP=-` is emitted, which fails to compile:

```
src/net.cpp:1619: error: expected primary-expression before ')' token
    MapPort(GetBoolArg("-upnp", USE_UPNP));
```

Remove the trailing space:

```
sed -i 's|USE_UPNP:=- |USE_UPNP:=-|' src/makefile.mingw
```

After this the daemon builds with UPnP disabled (default). The Qt build enables
UPnP itself (`-DUSE_UPNP=1`) and links the installed `miniupnpc`.

## Build commands

### Daemon (`wayawolfcoind.exe`)

```
cd /c/Users/z/Downloads/WWC3-main/WWC3-main/src
make -f makefile.mingw -j4
```

Produces `src/wayawolfcoind.exe` (already stripped by the makefile).

### Qt GUI wallet (`Wayawolfcoin-qt.exe`)

`wayawolfcoin.pro` lives in the **repository root**, not `src/`:

```
cd /c/Users/z/Downloads/WWC3-main/WWC3-main
PATH=/mingw64/qt5-static/bin:$PATH qmake wayawolfcoin.pro
make -j4
strip release/Wayawolfcoin-qt.exe
```

Produces `release/Wayawolfcoin-qt.exe`.

## Verify

Daemon (creates a datadir, then runs the server):

```
mkdir -p /tmp/wwctest
./src/wayawolfcoind -printtoconsole -datadir=/tmp/wwctest
```

Expected first lines:

```
Wayawolfcoin version v1.0.0.1canis-lupus (Aug  8 2026, ...)
Using OpenSSL version OpenSSL 1.0.2u  20 Dec 2019
```

Qt wallet: launch `release/Wayawolfcoin-qt.exe -datadir=<dir>` and confirm the
GUI window opens.

## Output

| File | Description |
|---|---|
| `src/wayawolfcoind.exe` | Daemon (console, 64-bit, ~9 MB stripped) |
| `release/Wayawolfcoin-qt.exe` | Qt GUI wallet (GUI, 64-bit, ~31 MB stripped, fully static) |

Both binaries are fully statically linked with no external DLL dependencies.

## Packaging (optional)

Copy the binaries to a `dist/` folder and zip with the naming convention
`<binaryname>-<targetplatform>-<arch>.zip`:

```powershell
New-Item -ItemType Directory -Force C:\Users\z\AppData\Local\Temp\opencode\wwc3build\dist | Out-Null
Copy-Item src\wayawolfcoind.exe dist\
Copy-Item release\Wayawolfcoin-qt.exe dist\
Compress-Archive dist\wayawolfcoind.exe    dist\wayawolfcoind-windows-x64.zip
Compress-Archive dist\Wayawolfcoin-qt.exe dist\Wayawolfcoin-qt-windows-x64.zip
```
