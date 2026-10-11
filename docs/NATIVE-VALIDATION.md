# 原生 Direct3D 候选版验证记录

> 当前修复版与当前 EXE 散列见 [CD-SINGLETON-2026-10-11.md](CD-SINGLETON-2026-10-11.md)。本页保存 2026-10-11 17:04 的旧交付与设备验收记录；其中的旧散列不代表当前 EXE。

记录生成时间：2026-10-11T17:04:28+09:00。本记录只总结实际完成的检查，原版功能对照与未完成验收见 [PARITY.md](PARITY.md)。没有公开发布或上传。

## 当前交付文件

- [`RhineMusic.exe`](../RhineMusic.exe)：162,862,592 字节，约 155.3 MiB，单文件交付。
- EXE SHA-256：`7c803d4127b3d72fea71db3e0e90c930bf3991172797bf395fe66fdd75959b3d`。
- 内置 ZIP SHA-256：`b022d7397ec8a5c8fe06187ac4afe5d253f0d1bea65b6ab35e7da0468cce40ce`。
- 内置 826 个程序文件，总解包尺寸 308.8 MiB；逐文件清单见 [`package-manifest.json`](../build/package/package-manifest.json)。
- SHA 清单涵盖原生 EXE、Node／Python、Sony／Pad、DLSS／FSR、应用私有 VC 运行库、模型、字体、纹理、着色器与许可。启动器使用嵌入清单校验，缓存中的清单不能覆盖它。
- 原生前端为 Win32／Direct3D／Direct2D／DirectWrite；没有 Electron、Chromium 或 WebView。音频和服务继续使用原 Node／原生组件与 Sony 桥接。
- 程序与原生窗口图标使用上游 `windows/app.ico`，原 ICO SHA-256 为 `61947495be03e372c44da7e356925a654a359fac31763dd9cac07165376bcea1`；两份 EXE 内嵌的 6 种分辨率图像逐字节一致。见 [`icon-validation.json`](../build/icon-validation.json)。

## 本机与设备证据

显卡：NVIDIA RTX A4500，硬件 DX11／DX12 与 DXR 1.2。Sony libScePad 初始化成功。检测到实际 DualShock 4。此前用户在可见隔离窗口完成按键与触摸板操作，记录 155 次真实输入并确认正常；CD 扩展之后的最终包另有连接与命令回归。拔插和蓝牙没有完成。

F: 音频 CD：45 条轨道。设备探测只读 2352 字节并保留在内存；实际播放测试包括 WASAPI、暂停、20 秒定位、恢复、下一首、停止与 DC 相位流式处理。处理后保留播放位置，测试产生的处理缓存已清理，没有保存原始抓轨文件。设备证据见 [`device-report.json`](../build/device-report.json)，打包后的播放证据见 [`cd-playback.json`](../build/portable-validation/cd-playback.json)。

连接 FiiO DAC 后，用 EXE 实际解包出的运行库通过 ASIO PCM 192／384 kHz、原生 DSD64／128／256、DoP64／128 的播放、暂停、1.2 秒定位及停止。DoP256 超出驱动载波采样率，拒绝后恢复 PCM 通过。使用合成静音；用户音量偏好没有修改。最终复测前曾再次出现 `driver unavailable (3)`，当时 Windows PnP 没有枚举 FiiO DAC，而 DS4 仍在；此段通过结果只对应设备重新可用后的验证，不将未连接状态下的失败算作通过，也不能据此断言更早失败的根本原因。见 [`asio-playback.json`](../build/asio-playback.json)。

新增 WASAPI 独占也使用最终 EXE 解包的运行库实测：FiiO 的 PCM 44.1／48／96／192／384 kHz 保持源采样率，暂停／定位／恢复／停止通过；两个宿主同时占用设备时第二个被拒绝，释放后恢复；不支持的采样率和无效端点被拒绝；DSD64 转 PCM、自然结束释放设备以及共享／独占切换通过。全部使用静音合成源，不修改系统全局音量。按 Windows 端点标识选择设备、忽略过期的枚举序号和缺失标识拒绝也通过；12 项证据见 [`wasapi-exclusive.json`](../build/wasapi-exclusive.json)。

## 本次追加的控件与循环

“开发与资源”首行是“此版本重构：GamesDing”，链接指向 https://github.com/gamesding；原作者和许可保留。输入框共用焦点边框并保留 Windows 标准闪烁光标，菜单使用线性展开／收起／选中行滑动，减少动态效果时直接切换。空在线入口暂时隐藏，主题适配的 Esc 键帽继续放在标识下方。新增控件与原版测量有意不同，原版几何比较只排除这次要求的 Esc 键帽和循环按钮；原有节点仍检查。

玻璃播放条的三种模式均有 SVG 图标和中文文字，适配暖昼／深夜并保存。原生控件在 1600×900、1000×750、488×1055 输出尺寸完成 312 项命令检查，包括设置、曲库、搜索、流派、内部在线和档案的全部编辑框；标准 caret 归属和系统闪烁周期有证据。自测窗口是隐藏窗口，输入框的 WS_VISIBLE 与系统 caret 通过自身 HWND 查询；真实触屏和实际手柄按键仍未替代为这些命令测试。截图与证据见 [`controls-validation.json`](../build/controls-validation.json) 和 [`controls-update/`](../build/controls-update/)。

循环通过最终 EXE 的真实音频结束事件验证：不循环顺序前进并在末尾停止；单曲多次从头播放、手动切歌和停止；专辑末尾回到首曲并按顺序播放；单曲专辑；播放时关闭循环；重启保存和无自动播放；独占输出重新取得设备；原版 Sony DC 处理后的循环。7 项行为组证据见 [`repeat-playback.json`](../build/repeat-playback.json)。

## 本次手柄选歌与 SVG 提示

进入专辑会将手柄焦点放在当前播放曲目或第一首可播放歌曲；动画中输入的上下方向保留。上下选择与正在播放的歌曲分离，确认才播放所选曲目；自动滚动使焦点行可见，列表边界不跳到其他专辑，关闭设置后恢复所选行。原版搜索定位与高亮、手动滚动、返回和再次打开的流程纳入检查。定位后选歌从目标歌曲继续。设置内原生输入框也支持手柄上下导航，确认及左右光标操作不会穿透到后面的专辑。

PS4／PS5 触摸板左、中、右各三分之一。左右短按松开为上一曲／下一曲，按住 500ms 后连续快退／快进，约 10 倍速度；中间按下播放／暂停。长按松开不会追加切歌，区域在按下时锁定；失焦或断连取消操作，重连后的按住状态要先松开再按。未按下时保留原旋转／滚动。按下的焦点切换改到 R3，Xbox 仍为右摇杆按下。

手柄操作时显示当前设备的按键提示，鼠标／键盘恢复 ESC 键帽。操作说明提供 PS4、PS5、Xbox 的面键、方向键与左右摇杆的上下左右 SVG，包含 Sony 触摸板三段图示。59 个 SVG 来自 [Kenney 官方 Input Prompts](https://kenney.nl/assets/input-prompts)，CC0；原字节与主题转换后的哈希、原许可随包保留。

最终单文件 EXE 在 1600×900、1000×750、488×1055 输出尺寸通过 411 项手柄命令与 SVG 检查，含模拟触摸板采样→实际播放器暂停／定位／恢复／切歌。PS4 连接由 SDK 实机读取，按键与触摸板动作是确定性输入样本，PS5／Xbox 是操作说明预览，不能算它们的物理设备按键测试。16 组短按、长按、移指、掉焦、断连、重连与渲染卡顿输入检查也通过。见 [`controller-validation.json`](../build/controller-validation.json)、[`controller-tracks/`](../build/controller-tracks/) 和 CTest 日志。

用户另在 CD 弹出扩展前的可见隔离窗口进行实际 DS4 打开专辑、选歌、确认播放、触摸板暂停／恢复／切歌／长按定位与返回，并回复“已测试，操作正常”。记录 155 次物理 SDK 输入，不含脚本注入；该记录保留当时的原生 EXE 哈希，不将它冒充最终包测试。见 [`physical-validation.json`](../build/physical-controller/physical-validation.json)。

## CD 弹出与底部避让

选中 CD 专辑后，右下角显示适配主题的“弹出”SVG 和文字；PS4／PS5 的 □ 或 Xbox 的 X 执行同一操作。普通专辑保留播放／暂停映射，触摸板中间仍为播放／暂停。先停止正在播放的所选 CD 并释放该光驱，再核对光驱类型及当前 TOC 身份，使用 Windows 弹出指令；成功后连接的手柄短震约 160ms 并归零。其他专辑播放不会被该操作主动停止，重复请求在操作期间被拒绝，失败时保留错误提示且不震动。

底部专辑计数、上下导航、滑尺、分类左右导航和辅助文字在播放玻璃显示时整体抬升，CD 按钮与播放条分开。小竖屏使用紧凑手柄提示并保留完整操作说明入口，避免占住专辑文字。最终 EXE 在 1280×720、1920×1080、2100×900、900×1600、800×600、480×800，以及用户截图的 1920×974 比例，覆盖昼夜完成 71 项原生命令、按键区别及占位边界检查；这里的 CD 专辑是拒绝物理弹出的隔离 UI 样本。另有 8 项光驱释放顺序、其他专辑播放保护与失败隔离检查。见 [`cd-controls-validation.json`](../build/cd-controls-validation.json)、[`cd-controls/`](../build/cd-controls/) 和 [`cd-service-validation.json`](../build/cd-service-validation.json)。

震动力度与提示排版调整前的单文件 EXE的可见隔离窗口记录 4 次真实输入，F: 弹出被驱动确认，CD 专辑移除；Sony 震动开始和归零返回 0。用户确认弹出与震动正常，要求提高力度、保持时长。该记录保留当时 EXE 哈希 `15c8af771afc05e88f1c3a3da46515220d08a8ba3c41623ed7bb7696cad95283`；后续没有重复实际弹出，不将旧包触感测试算作最终包的完整实机测试。 见 [`physical-validation.json`](../build/physical-cd/physical-validation.json)。

## 提示自动排版与反馈强度

全部临时提示使用同一个 DirectWrite 布局对象测量和绘制，尺寸保留取整裕量；短提示保持一行，长提示、显式换行、中英混排与连续路径按可用宽度自动换行。提示高度随内容变化，并避开播放玻璃和底部导航。极小窗口内的超长诊断保留全文，可用滚轮或触摸拖动阅读全文；阅读时暂停消失倒计时，正常提示保留淡入／淡出和 5.7 秒时长。

最终单文件包覆盖八种窗口尺寸、两种主题，通过 248 项实际渲染与排版检查，包括用户截图的 1920×974、最小 360×320、长路径、触摸拖动到末尾、播放中的避让及正常消失。见 [`notification-validation.json`](../build/notification-validation.json) 与 [`notifications/`](../build/notifications/)。

Sony 双电机提高至 160／104（0–255），XInput 提高至 42000／26000（0–65535），默认脉冲仍为 160ms。反馈归零在停绘分支之前执行，失焦或最小化会取消。最终包内的设备探测器使用实际 DS4，通过 8 项 SDK 启动／归零、持续时间和失焦取消检查：正常归零 168.61ms，失焦取消 55.97ms；这两次脉冲期间没有输入轮询或渲染。这里验证实际 SDK 行为，没有用它冒充用户对新力度的触感反馈。见 [`haptic-validation.json`](../build/haptic-validation.json)。

## 自动化结果

- CTest：3／3 通过，包括状态／连续运动、实际设备探测与 16 组手柄输入检查；日志见 [`LastTest.log`](../build/Testing/Temporary/LastTest.log)。
- 音频与服务：70／70 通过，失败 0，跳过 0。原测试针对包内解码器和 Sony 原文件运行，覆盖采样率、DSF／DFF 解码、声道隔离、并行与串行一致、取消与缓存、淡入淡出顺序、队列、FTP 边界、在线来源与只读标签。见 [`audio-validation.log`](../build/audio-validation.log)。
- 原生图形与交互基线：13／13 场景通过。交互压力 66 项、真实 CD 与窗口变化 29 项、大封面库 126 项。图形性能样本来自前次同一渲染实现的检查；本次控件更新另以最终包的三个图形路径、411 项手柄命令与 CD／导航检查复测。大库为 720 张隔离合成专辑，没有增加用户真实曲库数量。
- 单文件包：12／12 检查通过，包含中文目录、移除开发 PATH、损坏修复、三个图形路径、原版 Hi-Res／DSD、Sony／DS4、真实 CD 和模拟在线队列。
- 原版 UI 对照：四种 CSS 尺寸（1024×576、1536×864、720×1280、390×844）的主页／详情 8 场景、设置／曲库／搜索／在线页面 16 场景，均对照原版 DOM 记录核验；所检查的外框和内部节点尺寸／位置偏差小于 2 CSS 像素。设置末尾增加的原生图形选项不纳入原版正文总高度比较。Hi-Res／DSD 的专辑与曲目标记、DSEE／DC／Vinyl 动画、暂停冻结与减少动态效果通过原生场景。证据见 [`ui-parity-validation.json`](../build/ui-parity-validation.json)、[`main-ui-validation.json`](../build/main-ui-validation.json)，不是全部画面的逐像素验收。
- 真实曲库：111 张专辑封面接口全部返回 200，响应逐张匹配原图片 SHA-256；Origin 和目录越界仍被拒绝。M4A 与两张 DSD 专辑分别通过实际原生封面渲染，图片失败数均为 0，DSD／Hi-Res 标记可见；使用迁移所有曲目级封面引用的隔离索引，原真实索引字节保持一致。见 [`real-cover-requests.json`](../build/real-cover-requests.json)、[`personal-ui-validation.json`](../build/personal-ui-validation.json)。
- 真实文件播放：1 首 M4A、两张 DSD 专辑中的 DSD64 各 1 首及 1 首 DSD128，通过音量 0 的 WASAPI 播放、暂停、1.2 秒定位、恢复及停止；不是整库播放验收。见 [`real-library-playback.json`](../build/real-library-playback.json)。

| 最终原生场景 | 命令检查数 | 输出尺寸 | API | 实际超分提供器 | 调试错误 | 调试警告 | 证据 |
| --- | ---: | --- | --- | --- | ---: | ---: | --- |
| large-covers | 126 | 1280×720 | Direct3D 11 | native | 0 | 0 | [JSON](../build/native-review/large-covers.json) · [图](../build/native-review/large-covers.png) |
| stress | 66 | 1280×720 | Direct3D 11 | native | 0 | 0 | [JSON](../build/native-review/stress.json) · [图](../build/native-review/stress.png) |
| cd-playback | 29 | 1280×720 | Direct3D 11 | native | 0 | 0 | [JSON](../build/native-review/cd-playback.json) · [图](../build/native-review/cd-playback.png) |
| compact-night-detail | 0 | 1280×720 | Direct3D 11 | native | 0 | 0 | [JSON](../build/native-review/compact-night-detail.json) · [图](../build/native-review/compact-night-detail.png) |
| portrait-day-detail | 0 | 900×1600 | Direct3D 11 | native | 0 | 0 | [JSON](../build/native-review/portrait-day-detail.json) · [图](../build/native-review/portrait-day-detail.png) |
| ultrawide-night-detail | 0 | 2100×900 | Direct3D 11 | native | 0 | 0 | [JSON](../build/native-review/ultrawide-night-detail.json) · [图](../build/native-review/ultrawide-night-detail.png) |
| small-reduced-browse | 0 | 800×600 | Direct3D 11 | native | 0 | 0 | [JSON](../build/native-review/small-reduced-browse.json) · [图](../build/native-review/small-reduced-browse.png) |
| rtx-dlss-detail | 0 | 1920×1080 | Direct3D 12 | NVIDIA DLSS Super Resolution 310.9.1 | 0 | 16934 | [JSON](../build/native-review/rtx-dlss-detail.json) · [图](../build/native-review/rtx-dlss-detail.png) |
| dx11-native-ssr-performance | 0 | 1280×720 | Direct3D 11 | native | 0 | 0 | [JSON](../build/native-review/dx11-native-ssr-performance.json) · [图](../build/native-review/dx11-native-ssr-performance.png) |
| dx11-native-off-performance | 0 | 1280×720 | Direct3D 11 | native | 0 | 0 | [JSON](../build/native-review/dx11-native-off-performance.json) · [图](../build/native-review/dx11-native-off-performance.png) |
| dx11-fsr-performance | 0 | 1280×720 | Direct3D 11 | AMD FSR 1.0 (Direct3D 11 compatibility) | 0 | 0 | [JSON](../build/native-review/dx11-fsr-performance.json) · [图](../build/native-review/dx11-fsr-performance.png) |
| dx12-dlss-rtx-performance | 0 | 1280×720 | Direct3D 12 | NVIDIA DLSS Super Resolution 310.9.1 | 0 | 0 | [JSON](../build/native-review/dx12-dlss-rtx-performance.json) · [图](../build/native-review/dx12-dlss-rtx-performance.png) |
| dx12-fsr-rtx-performance | 0 | 1280×720 | Direct3D 12 | AMD FSR 3.1.5 | 0 | 0 | [JSON](../build/native-review/dx12-fsr-rtx-performance.json) · [图](../build/native-review/dx12-fsr-rtx-performance.png) |

尺寸覆盖紧凑横屏 1280×720、竖屏 900×1600、超宽屏 2100×900 和 800×600。主题覆盖暖昼／深夜；减少动态效果、详情、档案、查看器、设置、滑尺、定位、吸顶标签与触摸命令纳入回归。CD 压力用例在实际播放过程中改变真正的窗口尺寸，并切换 DX11→DX12 DLSS／RTX→DX11，确认播放持续。

非性能用例开启 Direct3D 调试层，所有已执行用例均无错误和消息丢弃。DX12 仍有 820／821（资源没有优化清除值）及 1328（缓冲区隐式状态提升）警告，未过滤，表中保留实际计数；因此没有宣称调试层零警告。性能用例关闭调试层，警告栏的 0 不代表开启调试后没有警告。

## 性能与优化

| 路径，1280×720 | 稳态整组进程私有内存 MiB | GPU 帧中位数 ms | GPU 帧 P99 ms | RTX 中位数 ms | 时间戳样本 |
| --- | ---: | ---: | ---: | ---: | ---: |
| dx11-native-ssr-performance | 455.6 | 10.010 | 10.458 | — | 600 |
| dx11-native-off-performance | 454.7 | 10.015 | 10.409 | — | 600 |
| dx11-fsr-performance | 434.1 | 10.013 | 10.465 | — | 600 |
| dx12-dlss-rtx-performance | 999.6 | 9.997 | 10.394 | 0.403 | 600 |
| dx12-fsr-rtx-performance | 839.2 | 9.996 | 10.421 | 0.406 | 600 |

内存包括原生主进程及其 Node／音频／Sony 子进程，采样忽略启动前 8 秒；不包含单文件解包启动器。GPU 数字来自非阻塞硬件时间戳，包括队列等待／呈现节奏，不是前台交互 FPS，也没有与 Electron 在相同条件下测量。因此不能据此宣布比原 Electron 快多少。

封面从固定 4096×6912 图集改为按需增长的稳定槽位和 LRU，保留原封面尺寸与选中封面的高分辨率缓存；已解码 URL 和进行中的请求共享。该优化前后，DX11／原生／SSR 测试的整组进程稳态私有内存中位数由 558.7 MiB 降为 455.6 MiB。720 张隔离专辑滚动、图集淘汰与切换期间封面连续性检查通过。历史基线见 [`native-validation-before-atlas.json`](../build/native-validation-before-atlas.json)。

## 光追与彩虹证据

本机确实启用硬件 DXR Inline Ray Query。光子每帧 16384 个、最多 8 次追踪、6 个波长；玻璃反射另外分离 650／535／450 nm 三条有界路径。几何使用原 GLB 与实例变换，包含波长相关折射、全反射、地面落点累积、时序重投影与遮挡变化拒绝。

1080p DLSS／RTX 捕获时，焦散网格有 1417 个发光 texel，RGB 能量均非零，异常值 0；不同颜色的落点质心不同。这证明实际光谱计算在运行，**不证明薄 CD 盒在每个镜头都产生参考照片那样的可见长彩虹**。当前主要接收面是原场景地面，专辑阵列遮挡会影响可见程度。详情见 [`rtx-dlss-detail.json`](../build/native-review/rtx-dlss-detail.json)。

## 单文件独立启动

验证把最终 EXE 复制到中文路径中，该交付目录始终只有这一个文件。子进程 PATH 仅保留 Windows／System32，移除开发 Node／Python 环境；运行时来自 EXE。所有 826 个文件逐个比对 SHA-256，分别篡改着色器和运行时清单后再次启动，原子修复均成功。它是同一 Windows 系统上的路径隔离验证，**不是干净系统 VM 验证**。

| 独立启动检查 | 结果 |
| --- | --- |
| Unicode path cold extraction with a single EXE | 通过 |
| Every extracted program file matches the package SHA-256 inventory | 通过 |
| Owned cache corruption is detected and atomically repaired | 通过 |
| A modified checksum manifest cannot bypass embedded verification | 通过 |
| Packaged dx11 render, original Sony and physical DS4 initialization | 通过 |
| Packaged dx12-dlss-rtx render, original Sony and physical DS4 initialization | 通过 |
| Packaged dx12-fsr-rtx render, original Sony and physical DS4 initialization | 通过 |
| Packaged original-badges render, original Sony and physical DS4 initialization | 通过 |
| Embedded runtime backend-smoke | 通过 |
| Embedded runtime cd-playback | 通过 |
| Embedded runtime online-playback | 通过 |
| Delivery folder still contains exactly one executable | 通过 |

详细证据见 [`standalone-validation.json`](../build/portable-validation/standalone-validation.json)。 FSR 原 DLL 的 VC143 x64 依赖已按 Microsoft 可分发清单以应用私有 DLL 内置，没有安装或改变系统运行库。

## 原文件与包内容

原工作树快照 496 个文件的哈希全部保持一致；Sony 原组件 10／10、音频运行文件 8／8 匹配固定清单。没有发现个人配置、凭据、曲库索引或缓存。包内音频只包含原版的五个演示／界面资源；实际 CD、测试 PCM 和个人音乐均不进入 EXE。详见 [`package-input-audit.json`](../build/package-input-audit.json)。

## 尚未覆盖与视觉限制

- 环境贴图使用原 RoomEnvironment 几何／光照并作 GGX 卷积；Three.js PMREM、AO、DOF、玻璃与原生文本渲染的逐像素一致未验收。原视觉效果种类与参数被保留，仍有画面差异。
- 桌面 computer-use 服务缺失 native pipe，按技能要求重试后仍失败。原生截图和命令检查通过；用户实际 DS4 操作另有确认与输入记录。真实触屏及全面鼠标操作尚未完成。
- Xbox、DualSense、蓝牙／热插拔、旧 GTX／AMD／Intel GPU、FSR 4.1.1 专用硬件、其他 ASIO／DSD／DoP DAC、其他光驱及 CD 换盘／读取故障恢复尚未实机验证。当前 F: 弹出范围见上面的独立记录。
- 实际 DPI／跨显示器切换、最小化恢复、驱动移除、长期连续运行与干净 Windows VM 尚未完成。
- 没有零 bug 保证，也没有未经基线支持的“比 Electron 更快”结论。以上缺口仍保留在候选版验收清单中。

## 官方组件来源

本次联网选择并固定的来源与文件哈希在 [`sdk-sources.json`](../vendor/sdk-sources.json)。相关官方说明：[NVIDIA DLSS 310.9.1](https://github.com/NVIDIA/DLSS/releases/tag/v310.9.1)、[AMD FidelityFX SDK](https://github.com/GPUOpen-LibrariesAndSDKs/FidelityFX-SDK)、[AMD FSR](https://gpuopen.com/amd-fsr-upscaling/)、[Microsoft DXR 规范](https://microsoft.github.io/DirectX-Specs/d3d/Raytracing.html)、[Microsoft VC 文件部署](https://learn.microsoft.com/en-us/cpp/windows/deployment-in-visual-cpp?view=msvc-170)。许可归属见 [DEPENDENCIES.txt](DEPENDENCIES.txt)。
