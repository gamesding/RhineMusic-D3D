# Rhine Music Direct3D 0.1 · 构建与验证记录

验证日期：2026-10-11。发行版本为 **0.1**；保留的上游基础仍为 V0.3.0，不改写上游版本、历史或署名。

## 最终源码与发行目录

完整源码位于 `C:\Users\YuQing\Documents\GitHub\RhineMusic-D3D`。在这个目录重新创建了干净的 `build/`，执行 `python tools/build.py --verify --jobs 4`，构建、打包与全部默认验证成功。准备阶段使用随仓库提供的离线依赖，没有要求安装 Sony 手柄 SDK。此前还在独立暂存副本完成了完整编译与配置检查。

旧构建中间文件与被修改源码在项目内的 `.cache/configuration-migration/` 保留备份，不提交、不打包。最终发行程序统一位于 `Release/`，保留的 Git 历史未被重置；没有执行提交、推送或 GitHub 上传。

| 发行文件 | 内容 |
| --- | --- |
| `Release/RhineMusic.exe` | 单文件播放器，163,265,536 字节 |
| `Release/RhineConfiguration.exe` | 独立配置窗口与内嵌硬件探测器，422,400 字节 |
| `Release/RhineMusic-D3D-v0.1-Windows-x64.zip` | 两个 EXE、使用说明、许可和 SHA-256 清单 |
| `Release/使用说明.txt` | 先配置，确认成功后手动启动播放器 |
| `Release/LICENSE`、`NOTICE.md`、`THIRD-PARTY.txt` | 保留的许可与来源说明 |
| `Release/SHA256SUMS.txt` | 两个 EXE 与随包文档的校验值 |

播放器与配置程序的 Windows 文件 / 产品版本为 `0.1.0.0`，原生开发目标显示为 `0.1`。三个 EXE 都保留原工程 ICO 中的 6 张不同分辨率图像，逐字节校验一致。

## 本次增加的行为

- 配置界面使用 WinForms / GDI+ 软件绘制，包含暖昼、深夜主题、圆角卡片、线性悬停过渡、焦点边框、键盘可操作下拉菜单，以及真实滚动区域。
- 自动检测显卡、显存、DX11、DX12、D3D11On12 互操作、DXR / 光谱所需能力、内存、逻辑处理器与供电状态；后台探测有 20 秒超时。探测器嵌入配置 EXE，临时释放到程序自建目录并清理。
- 自动推荐按设备能力与资源给出保守配置，允许单独选择超分；自定义提供显卡、接口、反射、基础档和全部精细图形参数。自动推荐不是运行帧率基准后寻找全局最优解。
- 非 NVIDIA 设备不提供 DLSS；NVIDIA GTX 等不满足条件的设备也不提供 DLSS。DX11 / 非合格 RTX 不提供本项目的 RTX 光谱反射；软件渲染固定 DX11 / 原生 / 关闭反射。
- 多显卡按 LUID 区分，同名设备可独立选择。自动模式重新评估最佳设备，自定义保留可用的指定设备；设备消失后播放器可使用可用设备。
- 配置窗口按屏幕工作区确定初始尺寸，小窗口启用紧凑布局。确认按钮保持可见，精细参数滚动访问。
- 图形偏好在播放器第一次创建渲染器之前读取；使用兼容配置时，不再先创建默认 DX12 场景然后等待服务下发配置。
- 保存使用原有用户配置目录与原子替换，仅写图形字段，保留音频输出、音效、曲库、主题、音量与循环。播放器运行时拒绝并发保存；损坏配置不被覆盖。
- 保存成功后显示确认窗口；按确定关闭配置程序。没有自动启动播放器。
- 增加 DX11 WARP 软件兼容路线，继续绘制原有场景与界面，用于驱动兼容排查与恢复设置。CPU 处理复杂三维场景可能较慢。

## 实际验证

工具环境：Windows x64、Visual Studio 2022 Professional / MSVC 19.44、Windows SDK 10.0.26100.0、CMake 4.3.3、Python 3.14.7、系统 .NET Framework 编译器。测试机为两张 NVIDIA RTX A4500、32 个逻辑处理器、约 253 GiB 内存；这些结果不等于所有 Windows 设备兼容认证。

| 检查 | 实际结果与证据 |
| --- | --- |
| 最终目录干净构建 | 成功；`build.ps1` / `tools/build.py` 完成依赖准备、原生构建、配置 EXE、27 个着色器入口与打包 |
| 固定依赖 | 2,095 个文件通过 SHA-256 检查；`tools/verify-source.py` 确认不包含私有 Pad SDK 头文件、声明或静态库 |
| CTest | 6 / 6：配置策略、启动参数、原生状态连续性、CD 独立实例、设备接口、手柄触摸输入 |
| 配置程序 | 920 项检查通过；新增 768 项昼夜圆角背景像素检查；6 组模拟设备、选项联动、多显卡、低配置 / 电池、精细参数、下拉键盘操作、真实控件滚动、小屏幕布局、保存及成功确认退出 |
| 独立配置 EXE | 只复制一个配置 EXE 到独立中文目录，去掉开发工具 PATH，内嵌探测器与配置检查仍通过 |
| 音频 / 服务 | 70 / 70；使用隔离合成 / 上游示例，实际运行组件与最终包 SHA-256 一致 |
| 单文件播放器 | 12 项发行检查通过：中文路径、冷启动解包、871 个内嵌文件逐项哈希、损坏缓存修复、原生渲染、原音频 / 手柄运行组件初始化、CD 与在线服务软件场景 |
| 播放器界面回归 | 1600 × 900 暖昼下两组实际渲染场景，61 + 119 = 180 项状态检查通过；15 个输入框的闪烁光标像素、密码遮罩、播放 / 暂停、循环、滚动与简繁 / 拼音搜索验证成功 |
| 首次设备配置 | 5 / 5 实际渲染路径；首次与最终图形状态一致，均为零图形恢复、零调试错误与零丢弃调试消息 |
| 原图标 | 开发播放器、发行播放器、配置程序中的图标图像与上游 ICO 原字节一致 |
| 包输入审计 | 上游 483 文件快照未变；原音效组件字节未变；59 张手柄图标与许可保留；没有个人配置、曲库或缓存 |

首次设备验证覆盖：

| 保存配置 | 实际结果 |
| --- | --- |
| DX11 / 原生 / 关闭反射 / 65% 渲染比例 | 首次就采用 DX11 与 65%，阴影、环境遮蔽关闭，透射分辨率 25% |
| DX11 / FSR / 关闭反射 | 使用官方 DX11 兼容超分 |
| DX12 / FSR / SSR | 实际启用 FSR 与 DX12 |
| DX12 / DLSS / RTX / 指定第二张同名显卡 | 按指定 LUID 选择第二张 RTX A4500，DLSS 与光谱光追生效 |
| WARP / DX11 / 原生 / 关闭反射 / 50% | 使用软件设备完成界面与三维渲染 |

可重新运行：

```powershell
.\build.ps1 -Verify
python tools/validate-configuration.py
python tools/validate-interactions.py --packaged --quick
```

机器可读证据位于 `build/configuration-tests/report.json`、`build/configuration-delivery/validation.json`、`build/audio-validation.json`、`build/audio-validation.log`、`build/portable-validation/standalone-validation.json`、`build/icon-validation.json`、`build/package-input-audit.json` 与 `build/package/package-manifest.json`。这些生成记录不进入源码提交。

## 配图与未覆盖范围

README 中新增的三张配置截图来自程序自身真实窗口绘制，使用明确标注的合成设备数据展示自动 / 自定义 / 小屏幕界面。自动测试操作的是程序自身控件与事件，包括成功提示与确认退出；本轮桌面自动化连接不可用，没有将这些结果写成外部鼠标手动实测。

本次没有在真实 AMD、Intel、老显卡、不同 DPI 显示器切换、干净 Windows 虚拟机或所有驱动版本上实测，也没有重新要求连接 DAC、PS4 / PS5 或实体光驱。设备实测与较全面旧界面回归的既有记录保留在 [SOURCE-BUILD-VALIDATION.md](SOURCE-BUILD-VALIDATION.md)；新的图形测试使用隔离示例库。未覆盖长期运行和不同硬件组合，不能据此承诺绝对没有 Bug。

能力检测采用 Windows 标准接口：[D3D12 光追能力查询](https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ns-d3d12-d3d12_feature_data_d3d12_options5)、[D3D11On12 设备创建](https://learn.microsoft.com/en-us/windows/win32/api/d3d11on12/nf-d3d11on12-d3d11on12createdevice)。

## 配置窗口圆角白边修复

修复按钮及卡片圆角外侧的矩形白边：控件现在先绘制所在位置的完整父背景，再绘制圆角轮廓；卡片内的“重新检测”按钮保留卡片底色，顶部主题按钮保留页面渐变。圆角、焦点边框与线性悬停动画继续保留。

新增独立控件像素回归，逐个检查两个卡片和六个按钮的四角背景及不透明度，覆盖 6 组设备的自动、自定义、紧凑窗口及相反主题，共新增 768 项检查。此回归在修复前的实际源码上失败，定位到按钮四角未完整绘制；修复后的最终目录构建通过全部 920 项配置检查。滚动面板的系统滚动条不属于圆角客户区，测试仅比较实际绘制区域。

本次仅重建配置 EXE，更新 `Release` 中的 ZIP、SHA-256 清单及三张配置界面配图。播放器 EXE 与内嵌运行组件继续使用先前通过发行验证的原字节。修复后的单独配置 EXE 也通过隔离目录检查。当前完整修复包为 `Release/RhineMusic-D3D-v0.1-Windows-x64.zip`，已验证 7 个 ZIP 条目与发布目录逐字节一致。发行目录与构建脚本统一使用标准文件名。

## 本次最终 EXE 校验

```text
5a8246c8edcedc43f2d3b1e80a969a4e1efc91ee5e0a813d63e3343cdf3a189c  RhineMusic.exe
2e04f66b4f4dc7fe074537fb1618ce35da8a72f129bfc4b2d8ba690233562156  RhineConfiguration.exe
```

以上校验值对应本次交付的构建；重新编译后请以新生成的 `Release/SHA256SUMS.txt` 为准。
