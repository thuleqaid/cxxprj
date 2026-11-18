这是一个Windows环境下，使用MinGW-w64和CMake工具链的C++项目的模板。已集成`Dear ImGui`框架。

# 模板结构
- `extsrc`：
  - 库文件的源代码，需要时再编译
  - 编译库文件的批处理文件（`buildext*.bat`）
  - 使用库文件的CMakeLists设置（`extlibs.cmake`）
- `extlibs`：库文件的安装路径
  - `doc`：库文件的文档
  - `include`：头文件库的文件
  - `x86`：编译32位程序使用的外部库/预编译库（含头文件）
  - `x64`：编译64位程序使用的外部库/预编译库（含头文件）
- `modules/entry`：`main`函数
- `include/xxx`：`xxx`模块的公开头文件
- `modules/xxx/inc`：`xxx`模块的私有头文件
- `modules/xxx/src`：`xxx`模块的实现文件
- `tests/xxx`：`xxx`模块的`GoogleTest`测试用例
- `tools/mkdocs2mdbook`：将MkDocs工程转换成mdBook工程

# 工具
## 必备
- [MinGW-w64](https://winlibs.com)：编译器，带CMake，32位和64位分别下载（选择UCRT运行时，不带LLVM/Clang的版本）
- [mdBook](https://github.com/rust-lang/mdBook/releases/tag/v0.5.4)：生成某些库的文档
- [sed](https://github.com/uutils/sed/releases/tag/0.3.0)：Rust版本sed工具

## 可选
- [VS Code](https://code.visualstudio.com/download)：代码编辑器

# 库
## 头文件库
仅包含头文件的库。
- [cxxopts](https://github.com/jarro2783/cxxopts/releases/tag/v3.3.1)：命令行参数解析库。
- [nlohmann/json](https://github.com/nlohmann/json/releases/tag/v3.12.0)：JSON解析器。
- [inja](https://github.com/pantor/inja/releases/tag/v3.5.0)：类似`Python`中[Jinja](https://palletsprojects.com/projects/jinja/)的模板库，依赖`nlohmann/json`。
- [spdlog](https://github.com/gabime/spdlog/releases/tag/v1.17.0)：Log库。
- [TinyFSM](https://digint.ch/tinyfsm/download.html)：状态机。
- [doctest](https://github.com/doctest/doctest/releases/tag/v2.5.3)：测试框架，不支持Mock。
- [Fake Function Framework](https://github.com/meekrosoft/fff/releases/tag/v1.1)：单元测试时，定义依赖的外部函数的桩函数。
- [fkYAML](https://github.com/fktn-k/fkYAML/releases/tag/v0.5.0)：YAML解析库。（`tools/mkdocs2mdbook`依赖于这个库，这个库的文档转换又依赖于`tools/mkdocs2mdbook`，如果需要文档的话，生成2次就行。。。）

## 集成模块
需要补足一些通用接口函数的实现或者其它原因不能独立作为lib库使用，因此要集成到代码中一起编译。
- [Dear ImGui](https://github.com/ocornut/imgui/releases/tag/v1.93.0)：即时模式（每一帧都会重新绘制UI，没有控件内部状态和事件回调）的Gui框架。需要编码绘制UI，定义外部变量保存UI控件的状态。如果链接`FreeType`库，可以显示彩色的Emoji，否则只能显示黑白的Emoji。
- [Dear ImGui Test Engine](https://github.com/ocornut/imgui_test_engine/releases/tag/v1.93.0)：`Dear ImGui`的UI自动化测试框架。需要编码定义测试用例。
- [isotp-c](https://github.com/SimonCahill/isotp-c/releases/tag/v1.9.3)：C语言实现的ISO-TP层（ISO 15765-2）传输协议，长报文拆包和组包，处理流控帧等。需要编码实现报文发送，以及接收报文后转给`isotp-c`库，实现提供时间信息的函数等。

## 外部库
直接可以编译成独立的动态/静态lib库。
- [zlib](https://github.com/madler/zlib/releases/tag/v1.3.2)：压缩库。
- [Abseil](https://github.com/abseil/abseil-cpp/releases/tag/20250512.1)：通用C++扩展库。
- [Protobuf](https://github.com/protocolbuffers/protobuf/releases/tag/v36.2)：跨语言数据序列化库，常用于通信协议。依赖`Abseil`和`zlib`（可选）库。
- [NNG](https://github.com/nanomsg/nng/releases/tag/v2.1.0)：线程间、进程间、网络主机间的通信库。
- [libpng](https://github.com/pnggroup/libpng/releases/tag/v1.6.59)：png文件读写库。
- [FreeType](https://download.savannah.gnu.org/releases/freetype/)：字体渲染引擎。依赖`zlib`（可选）和`libpng`（可选）。
- [GoogleTest](https://github.com/google/googletest/releases/tag/v1.17.0)：测试框架，支持C++类的Mock（只能Mock虚函数）。
- [uchardet](https://www.freedesktop.org/software/uchardet/releases/)：文字编码检测库。
- [Mbed TLS](https://github.com/Mbed-TLS/mbedtls/releases/tag/mbedtls-4.2.0)：适用于嵌入式系统的TLS库。
- [vector_blf](https://github.com/Technica-Engineering/vector_blf/releases/tag/v2.4.2)：解析BLF格式的通信Log。依赖`zlib`。

## 预编译库
已经由其他人/组织编译好的动态/静态lib库。
- [LibreSSL](https://github.com/libressl/portable/releases/tag/v4.4.0)：`OpenSSL`的分支版本，更快修复安全漏洞。Windows下可编译，32位的MinGW需要特殊处理，64位可直接编译。
- [gettext & iconv](https://mlocati.github.io/articles/gettext-iconv-windows.html)：gettext和iconv库，用于开发多语言程序。

## 字体
- [Segoe UI Emoji - All characters](https://www.aivosto.com/fontitin/reports/font-segoe-ui-emoji.html)
- [Noto Emoji](https://fontmeme.com/fonts/noto-emoji-font/)
- [Noto Sans SC](https://fontmeme.com/fonts/noto-sans-sc-font/)