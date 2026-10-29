# AutoAuth 仲恺农业工程学院校园网认证客户端

项目通过 Git 子模块使用以下第三方依赖：

- [curl](https://github.com/curl/curl)，固定版本为 8.23.0
- [cJSON](https://github.com/DaveGamble/cJSON)，固定版本为 1.7.18


## 获取源码

递归克隆仓库以同时获取依赖：

```sh
git clone --recurse-submodules https://github.com/mai302mai/ZHKUAutoAuth.git
```

如果已经克隆仓库但尚未获取子模块，请运行：

```sh
git submodule update --init --recursive
```

## 构建要求

- CMake 3.21 或更高版本
- Windows 构建需要 MinGW-w64 GCC 和 `mingw32-make`
- Linux 构建需要 GCC 等编译环境
- VS Code C/C++ 扩展（可选，用于调试）

项目使用 C17、CMake 和 libcurl。Windows 版本通过 Schannel 支持 HTTPS，
并使用 Windows 证书存储，无需单独安装 OpenSSL。

Windows CMake preset 会从 `PATH` 查找 MinGW-w64。如果找不到编译器或
`mingw32-make`，请将 MinGW 的 `bin` 目录加入 `PATH`。

## 构建和运行

### 在 Windows 平台构建和运行程序：

```powershell
cmake --preset mingw-debug
cmake --build --preset mingw-debug
```

在 Windows 上运行认证程序：

```powershell
.\build\mingw-debug\AutoAuth.exe 账户名称(学号) 账户密码
```

程序会先查询在线列表。若账户已经在线，会输出第一条记录的
在线时间、客户端IP、上传大小(Byte) 和 下载大小(Byte)；否则提交登录请求。

### 在 Linux 平台上构建和运行程序：

构建程序

```sh
cmake -S . -B build/linux
cmake --build build/linux
```

运行程序 
在 Linux 上构建并运行时，还需要传入网络接口名称 (eg. eth0)
指定接口功能用于OpenWRT等路由设备上多接口认证

```
./build/linux/AutoAuth 账户名称(学号) 账户密码 eth0
```

## 在 Windows 主机上交叉编译 Linux AArch64 程序

`linux-aarch64` CMake preset 使用 Zig 的 `aarch64-linux-musl` 目标并静态
链接，因此目标设备不需要另行安装 libc 或 libcurl 共享库。由于门户使用
HTTP，此 preset 会关闭 TLS。

请安装 Zig 0.14.1 和 LLVM-MinGW，并将两者的 `bin` 目录加入 `PATH`。工具链
配置也会检查以下默认安装目录：
`D:\Software\zig-0.14.1` 和 `D:\Software\LLVM-MinGW64`。

```powershell
cmake --preset linux-aarch64
cmake --build --preset linux-aarch64
```

生成的文件为 `build/linux-aarch64/AutoAuth`，这是 Linux AArch64 ELF 可执行
文件。可在 AArch64 Linux 设备上运行：

```sh
./AutoAuth your-account your-password eth0
```

如果 Zig 不在 `PATH` 中，请在配置时设置 `ZIG_EXECUTABLE`。如果 LLVM-MinGW
不在 `PATH` 中，请设置 `AUTOAUTH_LLVM_MINGW_ROOT` 为其安装目录。

## 功能说明

`network_auth.c` 实现认证流程：在 Linux 上获取指定接口的 IPv4 地址；在
Windows 上确定到门户的路由所选 IPv4 地址，并将该地址填入 `wlan_ac_ip`
参数。门户请求会绕过代理环境变量，直接发送至门户。

门户登录接口使用明文 HTTP，并按其协议要求在 URL 查询参数中发送密码。
请仅在可信网络中使用；命令行参数也可能被本机其他进程查看。

Windows 程序在标准输出连接到控制台时会将控制台代码页设为 UTF-8，并以
UTF-8 编码编译中文消息。如果输出被重定向，请确保接收终端或文件查看器
使用 UTF-8 编码。

GET 客户端实现位于 `http_client.c` 和 `http_client.h`。函数返回堆分配的
响应内容，认证模块使用 `free()` 释放。

`online_info_parse(json, json_size)` 用于解析响应并返回 `online_info`
结构体，其中的四个字段均为该结构体拥有的字符串。使用
`online_info_destroy()` 释放结构体及其字段。`http_get` 返回的响应缓冲区
仍需由调用方使用 `free()` 释放。

cJSON 1.7.18 位于 `third_party/cJSON` 子模块中，按 MIT 许可证发布，详见
`third_party/cJSON/LICENSE`。curl 8.23.0 源码及其许可证位于
`third_party/curl` 子模块中。

运行在线信息解析测试：

```powershell
ctest --test-dir build/mingw-debug --output-on-failure
```

## GitHub Actions 自动构建与发行

GitHub Actions 会构建以下平台的可执行文件：

- Linux amd64、i386、AArch64 和 ARM hard-float（armhf）
- Windows amd64 和 i386

##许可证
AutoAuth 使用 GNU 通用公共许可证第 3 版（GPL-3.0-only）授权，详见
[LICENSE](./LICENSE)。版权所有 (C) 2026 mai302mai。

