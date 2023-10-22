<h1><center>计算机网络实验报告</center></h1>

<h3><center>lab2:配置Web服务器，编写简单页面，分析交互过程</center></h3>

<h5><center>2112614 刘心源</center></h5>

# 一、Web服务器

### 实验环境

系统配置：MacOS（arm64）+Vscode

`http-server` 是一个简单的、零配置的命令行HTTP服务器，它是由Node.js编写的。它非常适合于为前端开发者提供一个快速、简单的方法来运行一个本地Web服务器，以便在开发过程中提供静态文件。

本实验使用node.js中的`http-server`来快速运行一个本地Web服务器:

- 安装Node.js

  从[Node.js官方网站](https://nodejs.org/)下载并安装，安装完之后测试：

  ```bash
  node -v
  v18.18.2
  npm -v
  9.8.1
  ```

  安装成功🎉。

- 安装http-server

  ```bash
  sudo npm install -g http-server
  ```

- 在编写的html文件所在的文件夹目录下，使用终端输入`http-server`开启服务

  ![image-20231022170444965](typora/image-20231022170444965.png)

- 在浏览器的url中输入`127.0.0.1:8080`，如图：

  <img src="typora/image-20231023004248714.png" alt="image-20231023004248714" style="zoom:30%;" />



# 二、WireShark抓包

### 1.使用流程

由于在macOS上，捕获循环（loopback）流量可能比在其他系统上更为复杂。macOS的网络架构不允许Wireshark直接捕获循环流量。因此我使用tcpdump+Wireshark的方式进行捕获流量并分析。

##### tcpdump

`tcpdump`是一个强大的命令行网络分析工具，它允许用户捕获网络上的数据包以进行实时分析或保存以供以后分析。它是在UNIX和类UNIX操作系统（如Linux和macOS）上广泛使用的工具。以下是`tcpdump`的一些主要特点和用途：

1. **数据包捕获**:
   - `tcpdump`可以捕获通过网络接口传输的数据包。这对于诊断网络问题、分析网络性能或研究网络协议非常有用。
2. **过滤**:
   - `tcpdump`提供了一个强大的过滤语言，允许用户指定要捕获哪些数据包。例如，你可以指定只捕获来自特定IP地址或只捕获特定协议的数据包。
3. **实时分析和保存**:
   - `tcpdump`可以实时显示捕获的数据包，也可以将数据包保存到文件中以便以后分析。
4. **与Wireshark集成**:
   - 虽然`tcpdump`是一个命令行工具，但捕获的数据可以保存到PCAP文件中，然后可以在图形网络分析工具如Wireshark中打开和分析。
5. **协议解析**:
   - `tcpdump`能够解析多种网络协议，显示数据包的详细信息。

在使用`tcpdump`时，通常需要具有管理员或超级用户权限，因为数据包捕获通常需要更高的系统权限。



### 2.结果分析

##### WireShark面板结构

![image-20231022175644107](typora/image-20231022175644107.png)

**Frame**: 表示物理层的数据帧，此处表示 Wireshark 在网络上捕获了一个 68 字节（544 位）的数据包。

**Null/Loopback**:表示数据包是在本地回环接口（Loopback Interface）上捕获的。本地回环接口通常用于在同一台计算机上的网络通信测试。

**Internet Protocol Version 4**:互联网IP包的头部信息，表示数据包使用的是 IPv4 协议。

**Transmission Control Protocol**：传输层的数据段头部信息，这里表示数据包使用的是 TCP 协议。



##### TCP数据段格式