# WindowCaster
[![MS Build](https://github.com/SwartzMss/WindowCaster/actions/workflows/ms_build.yaml/badge.svg)](https://github.com/SwartzMss/WindowCaster/actions/workflows/ms_build.yaml)
[![Rust Build](https://github.com/SwartzMss/WindowCaster/actions/workflows/rust_build.yaml/badge.svg)](https://github.com/SwartzMss/WindowCaster/actions/workflows/rust_build.yaml)

**WindowCaster** 是一个远程显示工具，旨在让用户能够远程控制目标计算机上某个指定窗口的显示内容。  
其基本原理是：  
- **客户端（Rust）** 通过获取目标窗口的句柄，确定需要显示的窗口，然后将显示数据发送至服务端；  
- **服务端（C++）** 接收客户端传输的显示数据，并进行实时渲染，呈现目标窗口的画面。

## 功能特点

- **远程显示**  
  实现对目标计算机指定窗口的实时显示，无需直接控制目标计算机。

- **客户端数据控制**  
  客户端负责获取目标窗口的句柄，可以选择图片或者视频数据来源。

- **服务端实时渲染**  
  服务端接收来自客户端的显示数据后，进行实时渲染，呈现目标窗口的画面。

- **模块化架构**  
  服务端与客户端分别采用 C++ 和 Rust 实现，双方通过标准接口进行通信。数据格式采用 [Protocol Buffers](https://developers.google.com/protocol-buffers) 定义，便于后续功能扩展和维护。

## 项目结构

- `Server/`  
  C++ 服务端代码，负责接收客户端传输的显示数据，并实时渲染展示目标窗口的画面。
- `client/`  
  Rust 客户端代码，负责获取目标窗口的句柄、采集窗口显示数据，并将数据发送至服务端。

## 使用说明
# client.exe

该工具可在指定窗口上渲染图片或视频，并支持通过指定服务器的 IP 地址和端口号进行连接。[默认: 127.0.0.1:12345]

## 1. 列出窗口
获取当前系统可用窗口及其句柄（十六进制格式）：
```bash
client.exe list -i 127.0.0.1 -p 12345
```

## 2. 渲染图片
将图片渲染到指定窗口：
```bash
client.exe image --hwnd 0x12345678 --file /path/to/image.png -i 127.0.0.1 -p 12345
```

## 3. 渲染视频
将视频渲染到指定窗口：
```bash
client.exe video --hwnd 0x12345678 --file /path/to/video.mp4 -i 127.0.0.1 -p 12345
```

# server.exe
默认端口 12345,也可以指定端口
```bash
server.exe [端口号]
```

[点击观看项目介绍视频](https://www.bilibili.com/video/BV1Tdo4YzEhp)

## 贡献指南

欢迎大家提交 PR 或 issue，提出建议或改进方案。

## 许可证

请参阅 [LICENSE](LICENSE) 文件，了解本项目的具体许可协议。



