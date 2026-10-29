# AutoAuth C environment

AutoAuth is licensed under the GNU General Public License, version 3 only
(GPL-3.0-only). See [LICENSE](./LICENSE) for the full license text.
Copyright (C) 2026 mai302mai.

The project uses the following upstream dependencies as Git submodules:

- [curl](https://github.com/curl/curl), pinned to 8.23.0; see
  [its COPYING file](./third_party/curl/COPYING).
- [cJSON](https://github.com/DaveGamble/cJSON), pinned to 1.7.18; see
  [its MIT license](./third_party/cJSON/LICENSE).

The dependency submodules retain their upstream licenses; the project's
GPLv3 license does not replace those notices.

Clone this project and its dependencies with:

```sh
git clone --recurse-submodules https://github.com/mai302mai/ZHKUAutoAuth.git
```

If you already cloned the project without submodules, initialize them with
`git submodule update --init --recursive`.

The project is configured for C17 with MinGW-w64, CMake, and libcurl. Windows
builds use Schannel for HTTPS and the Windows certificate store without
requiring a separate OpenSSL installation.

## Requirements

- CMake 3.21 or later
- MinGW-w64 GCC and `mingw32-make` on `PATH` for the Windows build
- VS Code C/C++ extension for debugging

The Windows CMake preset finds MinGW-w64 through `PATH`. Add the MinGW `bin`
directory to `PATH` if the compiler or `mingw32-make` cannot be found.

## Build and run

Press `Ctrl+Shift+B` in VS Code to configure and build, or run:

```powershell
cmake --preset mingw-debug
cmake --build --preset mingw-debug
```

Run the online authentication workflow (Windows):

```powershell
.\build\mingw-debug\AutoAuth.exe your-account your-password
```

The program queries the configured portal's online list first. If the account
is already online, it prints the first record's `online_time`, `online_ip`,
`uplink_bytes`, and `downlink_bytes`; otherwise, it submits the login request.

On Linux, build and run with a network interface name:

```sh
cmake -S . -B build/linux
cmake --build build/linux
./build/linux/AutoAuth your-account your-password eth0
```

Linux interface binding for the general `http_get()` API is enabled by
default on Linux and remains an optional CMake feature. The authentication
module always binds portal requests to the required interface name.

## Windows host: cross-compile a static Linux AArch64 executable

The `linux-aarch64` CMake preset uses Zig's `aarch64-linux-musl` target and
links a static executable, so the target device does not need a separate libc
or libcurl shared library. TLS is disabled for this preset because the
configured portal uses HTTP. Install Zig 0.14.1 and LLVM-MinGW, and add both
toolchains' `bin` directories to `PATH`. The toolchain file also checks
`D:\Software\zig-0.14.1` and `D:\Software\LLVM-MinGW64` as default locations.

```powershell
cmake --preset linux-aarch64
cmake --build --preset linux-aarch64
```

The output is `build/linux-aarch64/AutoAuth`, a Linux AArch64 ELF executable.
Run it on an AArch64 Linux device:

```sh
./AutoAuth your-account your-password eth0
```

If Zig is not on `PATH`, set `ZIG_EXECUTABLE` when configuring. If LLVM-MinGW
is not on `PATH`, set `AUTOAUTH_LLVM_MINGW_ROOT` to its installation directory.

`network_auth.c` contains the portal workflow. It gets the selected Linux
interface's IPv4 address, or determines the Windows route-selected IPv4
address, and places it in the `wlan_ac_ip` parameter. Portal requests bypass
proxy environment variables so local authentication traffic is sent directly
to the portal.

The portal login endpoint uses plain HTTP and sends the password in the URL
query as required by its protocol. Use this only on a trusted network; command
line arguments may also be visible to other local processes.

On Windows, the program sets the console output code page to UTF-8 when stdout
is attached to a console, and compiles its Chinese messages as UTF-8. If output
is redirected, configure the receiving terminal or file viewer to read UTF-8.

The GET client is in `http_client.c` / `http_client.h`; it returns a
heap-allocated response body which the authentication module releases with
`free()`.

`online_info_parse(json, json_size)` parses the response and returns an
`online_info` structure containing the four fields as owned strings. Release
the structure and its fields with `online_info_destroy()`. The HTTP response
buffer returned by `http_get` remains heap-allocated and must be released with
`free()`.

cJSON 1.7.18 is checked out as a submodule under `third_party/cJSON` and is
distributed under the MIT license; see `third_party/cJSON/LICENSE`.

Run the parser tests with:

```powershell
ctest --test-dir build/mingw-debug --output-on-failure
```

The curl 8.23.0 source and its license are in the `third_party/curl`
submodule.
