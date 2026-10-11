# Sony 手柄组件的二进制边界

本源码发行版的 PS4 / PS5 底层组件只以 `runtime/drivers/rhine-pad-bridge.dll` 提供。
库将本机已有的控制器 SDK 调用封装为 `native/pad-bridge-api.h` 定义的项目自有 C ABI。

该头文件只描述本项目使用的逻辑按钮、归一化摇杆、触摸位置和震动接口；没有复制厂商的结构体、私有函数声明或常量定义。
`native/controller.cpp` 动态加载 DLL；公共工程无需安装、查找或链接 Sony SDK。
按钮导航、触摸板分区、短按切歌、长按快进快退、焦点管理和震动时长等项目交互逻辑仍提供源码。
Xbox 路线使用 Windows XInput，源码完整保留。

## 随包提供

- `runtime/drivers/rhine-pad-bridge.dll`：x64、ABI v1、静态 MSVC CRT，SDK 实现封装在库内。
- `runtime/drivers/pad-bridge.json`：ABI、SDK 版本、二进制 SHA-256 与许可说明。
- `vendor/scepad/license/`：原始许可文件。
- `native/pad-bridge-api.h`：可审阅的项目自有接口。

不提供底层实现源码、Sony SDK 头文件、SDK 静态库、示例、调试符号或私有 SDK 文档。
在没有 SDK 的机器上，用户仍能编译全部前端、服务、原生音频宿主与打包器，构建结果随带该二进制组件。
DLL 缺失、ABI 不符或固定依赖校验失败会明确报错，不会静默删除手柄支持。

二进制形式不会改变厂商许可和保密义务。本仓库按作者要求用于私有托管；对外分发这些组件前仍需自行确认相关授权。

## 接口与生命周期

初始化 → 按槽位打开 → 读取 → 关闭 → 终止；同一进程由一个控制器管理器持有。
读取结构包含 `byteSize`，不匹配时拒绝调用；返回整数错误码，不跨 DLL 传递 STL 对象或异常。
读取和设备管理在二进制组件内部串行保护。断开设备会清除型号与触摸元数据。
失去窗口焦点或结束播放反馈时立即停止震动；退出时关闭句柄，再卸载组件。
