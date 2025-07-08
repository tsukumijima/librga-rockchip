# RGA模块SLT测试工具



## 概述

该工具用于芯片SLT阶段使用，可以通过配置实现对RGA模块硬件进行测验。



## 使用说明

### 前置条件

#### 图像准备

由于RGA硬件执行需要外部输入图像进行处理，所以需要提前准备对应的图片并确保将图像存入设备指定目录。

图像存储的目录可以在slt_config.h中进行配置，文件命名规则如下：

- Raster mode

```
in%dw%d-h%d-%s.bin
out%dw%d-h%d-%s.bin

示例：
1280×720 RGBA8888的输入图像： in0w1280-h720-rgba8888.bin
1280×720 RGBA8888的输出图像： out0w1280-h720-rgba8888.bin
```

- FBC mode

```
in%dw%d-h%d-%s-fbc.bin
out%dw%d-h%d-%s-fbc.bin

示例：
1280×720 RGBA8888的输入图像： in0w1280-h720-rgba8888-afbc.bin
1280×720 RGBA8888的输出图像： out0w1280-h720-rgba8888-afbc.bin
```

> 参数解释如下：
>
> 输入文件为 in ， 输出文件为 out
> --->第一个%d 是文件的索引， 一般为 0， 用于区别格式及宽高完全相同的文件
> --->第二个%d 是宽的意思， 这里的宽一般指虚宽
> --->第三个%d 是高的意思， 这里的高一般指虚高
> --->第四个%s 是格式的名字。



#### 内存限制

由于部分芯片平台搭载的RGA2硬件不支持大于32位的物理地址，所以建议测试环境保证地址映射在0~4G地址空间以内。



#### 编译配置

在工具编译前，可以通过修改 slt_config.h 配置测试工具。

> slt配置

| 配置                | 说明                                                         |
| ------------------- | ------------------------------------------------------------ |
| IM2D_SLT_THREAD_EN  | 使能该配置后，将使能多线程模式，每个case都单独在一个线程运行。 |
| IM2D_SLT_THREAD_MAX | 多线程模式有效，配置最大的线程数量。                         |
| IM2D_SLT_WHILE_NUM  | perf-test场景有效，默认测试case循环次数。                    |

> perf-test

| 配置                  | 说明                                |
| --------------------- | ----------------------------------- |
| IM2D_SLT_TEST_PERF_EN | 使能该配置后，将默认使能perf-test。 |

> 环境配置

| 配置                                | 说明                       |
| :---------------------------------- | :------------------------- |
| IM2D_SLT_DEFAULT_INPUT_PATH         | 默认的输入图像路径。       |
| IM2D_SLT_DEFAULT_OUTPUT_PATH        | 默认的输出图像路径。       |
| IM2D_SLT_DEFAULT_GOLDEN_PATH        | 默认的golden文件路径。     |
| IM2D_SLT_GENERATE_CRC_GOLDEN_PREFIX | golden数据的可自定义前缀。 |



### 编译

#### Android

在配置好Android SDK编译环境后，在源码目录下使用如下命令编译即可。

```
mm -j32
```



#### Linux

- cmake

  - 修改librga源码根目录下的**/cmake/buildroot.cmake**文件。执行以下脚本完成编译:

  ```
  $ chmod +x ./cmake-linux.sh
  $ ./cmake-linux.sh
  ```

   **[编译选项]**

  1. 指定TOOLCHAIN_HOME为交叉编译工具的路径
  2. 指定CMAKE_C_COMPILER为gcc编译命令的路径
  3. 指定CMAKE_CXX_COMPILER为g++编译命令的路径

  - 前级librga目录未编译时，需要修改librga.so的链接路径：

  ```
  vim CmakeList.txt +82
  修改 target_link_libraries(im2d_slt librga.so）路径
  ```



### 工具运行

#### 命令介绍

可以通过以下命令获取帮助信息

```shell
:/ # im2d_slt --help=all

====================================================================================================
   usage: im2d_slt  [--help/-h] [--chip/-c] [--perf/-f] [--input/-i] [--output/-o] [--golden/-g]
                    [--prefix/-p] [--crc/r]

---------------------------------------- Config ----------------------------------------------------
         --chip/-c     Set chip
                         <options>:
                           <chip>        chip ready for testing, e.g. "--chip=rk3588".
         --perf/-f     Set perf mode
                         <options>:
                           <num>         set loop num, e.g. "--perf=50".
         --input/-i    Set input image file path.
                         <options>:
                           <path>        input image file path, e.g. "--input=/data".
         --output/-o   Set output image file path.
                         <options>:
                           <path>        output image file path, e.g. "--output=/data".
         --golden/-g   Set golden file path.
                         <options>:
                           <path>        golden image file path, e.g. "--golden=/data".
         --suffix/-p   Set golden suffix.
                         <options>:
                           <string>      golden image file suffix, e.g. "--chip=rk3576 --suffix=crcdata", so that the file name is "rk3576_crcdata.bin".
         --crc/-r      Generate golden by CRC. The target file will be generated according to --golden and --prefix
---------------------------------------- Other -----------------------------------------------------
         --help/-h     Call help
                         <options>:
                           all           Show full help.
====================================================================================================
```

- --chip：配置需要测试的芯片型号，例如--chip=RK3588、--chip=RK3576等等。
- --perf：配置当前测试是否需要使能性能测试，可以通过添加传参配置性能测试循环的次数，例如--perf=50，即性能测试循环运行50次。
- --input：配置当前测试源数据存储的绝对路径，未配置该命令时使用默认路径，默认路径可以通过slt_config.h进行修改。
- --output：配置当前测试输出数据存储的绝对路径，未配置该命令时使用默认路径，默认路径可以通过slt_config.h进行修改。
- --golden：配置当前测试读取、输出的golden数据存储的绝对路径，未配置该命令时使用默认路径，默认路径可以通过slt_config.h进行修改。
- --suffix：配置当前测试读取、输出的golden数据文件名的后缀，未配置该命令时使用默认后缀，默认后缀可以通过slt_config.h进行修改。
- --crc：配置当前测试使用CRC32生成golden数据，由“--golden/--suffix”指导生成golden数据路径、文件名。



#### golden生成

通常我们需要先在正常的设备上生成golden数据，用于实际测试场景，如果已经获取golden数据可以跳过该小节。

以下以RK3588为例：

- 创建源/目标数据存放路径，并将准备好的源数据存放到该路径下。

```shell
:/ # mkdir -p /data/rga
:/ # mv <source_image> /data/rga/
```

- 创建golden存放路径。

```shell
:/ # mkdir -p /data/rga/golden
```

- 执行im2d_slt生成golden数据。

```shell
:/ # im2d_slt --chip=rk3588 --input=/data/rga --output=/data/rga --golden=/data/rga/golden --crc
set chip[rk3588]
set input_path[/data/rga]
set output_path[/data/rga]
set golden_path[/data/rga/golden]
enable generate golden by CRC
-------------------------------------------------
creat Sync pthread[0x6dae20bbf0] = 1, id = 1
creat Sync pthread[0x6dae10dbf0] = 2, id = 2
creat Sync pthread[0x6dae00fbf0] = 3, id = 3
ID[2]: RGA3_core1 genrate CRC golden /data/rga/golden/crcdata_RGA3_core1.txt
ID[2]: RGA3_core1 running success!
ID[1]: RGA3_core0 genrate CRC golden /data/rga/golden/crcdata_RGA3_core0.txt
ID[1]: RGA3_core0 running success!
ID[3]: RGA2_core0 genrate CRC golden /data/rga/golden/crcdata_RGA2_core0.txt
ID[3]: RGA2_core0 running success!
-------------------------------------------------
RGA raster-test success!
-------------------------------------------------
creat Sync pthread[0x6dae20bbf0] = 1, id = 1
creat Sync pthread[0x6dae10dbf0] = 2, id = 2
ID[1]: RGA3_core0_fbc genrate CRC golden /data/rga/golden/crcdata_RGA3_core0_fbc.txt
ID[1]: RGA3_core0_fbc running success!
ID[2]: RGA3_core1_fbc genrate CRC golden /data/rga/golden/crcdata_RGA3_core1_fbc.txt
ID[2]: RGA3_core1_fbc running success!
-------------------------------------------------
RGA special-test success!
-------------------------------------------------
```

这里可以看到生成了多项.bin/.txt后缀的golden数据到目标路径。



#### 运行测试

在获取golden数据后，在测试环境下依旧需要准备好环境，并将测试工具以及golden数据推入测试设备。

以下以RK3588为例：

- 创建源/目标数据存放路径，并将准备好的源数据存放到该路径下。

```shell
:/ # mkdir -p /data/rga
:/ # mv <source_image> /data/rga/
```

- 创建golden存放路径，并将准备好的golden数据存放到该路径下。

```shell
:/ # mkdir -p /data/rga/golden
```

- 执行im2d_slt，测试当前设备。

```shell
:/ # im2d_slt --chip=rk3588 --input=/data/rga --output=/data/rga --golden=/data/rga/golden --perf                             set chip[rk3588]
set input_path[/data/rga]
set output_path[/data/rga]
set golden_path[/data/rga/golden]
set perf[500]
-------------------------------------------------
creat Sync pthread[0x79f5e07bf0] = 1, id = 1
creat Sync pthread[0x79f5d09bf0] = 2, id = 2
creat Sync pthread[0x79f5c0bbf0] = 3, id = 3
ID[1]: RGA3_core0 running success!
ID[2]: RGA3_core1 running success!
ID[3]: RGA2_core0 running success!
-------------------------------------------------
RGA raster-test success!
-------------------------------------------------
creat Sync pthread[0x79f5e07bf0] = 1, id = 1
creat Sync pthread[0x79f5d09bf0] = 2, id = 2
ID[1]: RGA3_core0_fbc running success!
ID[2]: RGA3_core1_fbc running success!
-------------------------------------------------
RGA special-test success!
-------------------------------------------------
creat Sync pthread[0x79f5e07bf0] = 1, id = 1
creat Sync pthread[0x79f5d09bf0] = 2, id = 2
creat Sync pthread[0x79f3c0bbf0] = 3, id = 3
creat Sync pthread[0x79f3b0dbf0] = 4, id = 4
creat Sync pthread[0x79f2307bf0] = 5, id = 5
creat Sync pthread[0x79ef3f9bf0] = 6, id = 6
creat Sync pthread[0x79eef77bf0] = 7, id = 7
creat Sync pthread[0x79ee069bf0] = 8, id = 8
creat Sync pthread[0x79ecf6bbf0] = 9, id = 9
ID[2]: perf_test running success!
ID[6]: perf_test running success!
ID[4]: perf_test running success!
ID[7]: perf_test running success!
ID[1]: perf_test running success!
ID[9]: perf_test running success!
ID[8]: perf_test running success!
ID[3]: perf_test running success!
ID[5]: perf_test running success!
-------------------------------------------------
RGA perf-test success!
-------------------------------------------------
```

确认没有failed项，即说明当前测试成功。