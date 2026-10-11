# DMP-Z1 整机调音与 DSD Remaster 研究

2026-10-09：完成 Sony 官方 DMP-Z1 1.02 系统、启动镜像和音频驱动的追踪。**DSD Remaster 与 DMP-Z1 整机调音音色均尚未移植。** 另取得的 ICX1300 文件属于 DSEE 参数，不能作为用户所要求的整机音色实现；已撤下开发中的 DSEE 参数设置开关，文件与对照入口仅保留作研究。 用户要求仅接受 Sony 原版，因此没有加入近似 PCM→DSD 算法或模拟 DAC 的 EQ。

## DSEE 参数旁支：不能代替整机调音

原固件 `libSoundServiceFw.so` 的 `DseeAi::LoadCoefFiles`，Thumb 地址 `0x0013cb71`，直接选择 `DseeAi_HP_ICX1300.bin` 与 `DseeAi_HP_ICX1300.dcfg`。[带原指令字节的反汇编](../research/sony-dmp-z1/analysis/dsee-model-selection.txt)保留完整函数，文件名在地址 `0x0013cba6`／`0x0013cbbc` 装载；没有用文件名猜测机器型号。

原版三份 AI 模型内容相同，均为 60,000 字节。ICX1291 与 ICX1300 的 20 字节配置前四个 32 位字相同，最后一字分别是 `1`、`2`。保留原字节，不解释为自制均衡器、音色曲线或新模型。原文件哈希、配置内容和来源见 [研究证据](DMP-Z1-RESEARCH-EVIDENCE.json)。

原配置保留于私有源码并通过原 ARM／x86 对照，研究 CLI 可通过 `--dmp-z1 1` 选择 ICX1300。正式设置没有第二个 DSEE 开关，也没有把它当作整机调音效果器。现有 DSEE、相位、黑胶的设置与默认参数继续沿用；192 kHz 等输入仍逐项旁路不支持的处理，原生 DSD／DoP 仍旁路所有 PCM 音效。

## 本轮用户提供的官方 Windows EXE

新提供的 `DMP-Z1_V1_02.exe` 大小 **125,947,728 字节**，SHA-256 为 `79966f4d6e0366b007888c239d60d5c4fe4e3e149afb9792000d995e507df2f3`，与此前下载的官方 Windows 包完全相同。只读拆包，没有执行 Sony 更新器。PE 文件偏移 `475136` 开始是 Sony Packaging Tool 容器；其中偏移 `1728` 开始的 CAB 经逐字节 `0xFF` 异或还原，使用 LZX:21 压缩，包含 18 个更新器／许可／固件文件。

实际从这份 EXE 展开的 `Data/Device/NW_WM_FW.UPG` 为 **123,896,000 字节**，SHA-256 `fb77e44e06ce7c14a74351f0a3fa991d4c7c9f643a222054994db7cf1c65590b`。与此前官方 macOS 包里的 UPG 逐字节一致；外层 MD5 通过，重新解出的 9 个条目与已有研究逐项相同。`nw-wm1z` 是解包工具匹配的加密标识，不是据此把 DMP-Z1 误判为 WM1Z。完整外壳、CAB、18 个文件、UPG 条目及哈希保存于固件旁研究副本；[证据 JSON](DMP-Z1-RESEARCH-EVIDENCE.json)记录原始字节来源。该新文件没有提供与现有系统不同的 Remaster 核心或整机音色模型。

## DSD Remaster 的实际路径

Sony [产品说明](https://www.sony.jp/audio/products/DMP-Z1/feature_4.html)将此功能描述为 PCM 转约 5.6 MHz DSD。[官方帮助](https://helpguide.sony.net/dmp/dmpz1/v1/en/contents/TP0001691077.html)要求先开启 Direct Source；该状态不能调整其他 Sound Settings。原设备也不是把 Remaster 与所有音效无条件叠加。

本次完整解包包含 **1,391 个系统普通文件**。固件 ARM HAL 中的 `AudioHalOutDsdRme` 处理缓冲、可选 PCM 升采样及 ALSA 输出；`WriteToChunk` 调用 `memcpy`。[HAL 反汇编](../research/sony-dmp-z1/analysis/disassembly.txt)保留原地址、字节和导入函数。

启动镜像中的 MTK 内核已解压，其函数及设备名包含 `mt8590_dsdenc_hw_params`、`ICX_AULOS_PCM_DSD`、`DSD_ENC_CON0` 和 `CODEC_AK4497_DUAL_L/R`。另从 [Sony 官方源码分发页](https://oss.sony.net/Products/Linux/Audio/DMP-Z1.html)取得 Linux 3.10.26 原源码，保留 GPL 许可与版权。关键原代码如下：

| 原文件 | 可核查的行为 |
| --- | --- |
| [icx-aulos.c](../research/sony-dmp-z1/kernel-source/sound/soc/sony/icx-aulos.c) 的 `icx_aulos_pcmdsd_hw_params`，约 2115 行 | 为 352.8／384 kHz PCM 设置 `SND_SOC_DAIFMT_PCM2DSD`，写 `DSD_ENC_CON0` 并启用编码器。 |
| [mt8590-dsdenc.c](../research/sony-dmp-z1/kernel-source/sound/soc/mediatek/mt8590/mt8590-dsdenc.c)，约 35 行 | 选择 `I2S_OUT_DSD_USE_SONY_IP`，开启 `MT_CG_AUDIO_DSD_ENC` 和硬件电源。 |
| [mt8590-afe.c](../research/sony-dmp-z1/kernel-source/sound/soc/mediatek/mt8590/mt8590-afe.c) 的 `afe_dsdenc_configurate`，约 5625 行 | PCM8→DSD128 等模式写三个编码器寄存器；开启函数设置硬件使能位。 |
| [mt8590-afe-reg.h](../research/sony-dmp-z1/kernel-source/sound/soc/mediatek/mt8590/mt8590-afe-reg.h)，约 1435 行 | 编码器位于 `AFE_BASE + 0x11C0/0x11C4/0x11C8` 的 MMIO 寄存器。 |
| [dsdenc_test_data.c](../research/sony-dmp-z1/kernel-source/sound/soc/mediatek/mt8590/dsdenc_test_data.c) | 保存有限的 PCM／DSD 输入及 golden 输出向量；不是完整转换实现。 |

这些证据支持：**已追踪的 Remaster PCM→DSD 转换发生在 MT8590 硬件编码器。** 已取得的 ARM 控制代码和 GPL 驱动没有提供该硬件的完整算法、RTL 或可用于全输入对照的参考实现。把寄存器写入逐句翻成 C++，电脑仍缺少这个硬件，不会因此产生原版 DSD。有限测试向量也不能证明任意音乐的一比一还原。

因此正式设置没有增加不能工作的 Remaster／整机音色开关，也没有把近似算法或普通 DoP 封装宣称为 Sony Remaster。未来需要原编码器实现／完整规格及原机参考输出，才有资格扩展一比一验证。

## 新增追踪：CM4 微码、调试调参入口与其他分区

原 UPG 的全部八个非 system 条目已另行检查。`CM4` 是 22,296 字节的原 Cortex-M4 微码，日志与 [低功耗音频原驱动](../research/sony-dmp-z1/kernel-source/sound/soc/mediatek/mt8590/mt8590-lp-audio.c)对应 DMA、memif、IRQ、SRAM／DRAM 及唤醒控制；已展开保存，不能把其存在当作已经找到软件 Remaster。另有 TEE、安全区域、LOGO 与升级背景；没有从中提取出可验证的 Remaster 运算核心。

`Filter::IsTuningEnabled`（`0x000fc4d1`）读取 `persist.sys.sony.icx.tuning`；`GetPathTuningStorage`（`0x000fc595`）返回 `/contents_ext`，开启后等待 SD 卡挂载。DSEE／Vinyl 的加载函数可从该位置读调试替换参数。这里的 tuning 字样不是一份已取得的 DMP-Z1 整机频响／模拟音色模型；不能仅靠属性开关便复刻整机。完整 [后续审计](../research/sony-dmp-z1/analysis/deep-audit.json)保留相关原符号、其他分区与哈希。

## 整机调音的边界

原 `icx_aulos_set_filter`（同一 `icx-aulos.c` 约 1205 行）把六种数字滤波模式写入 AK4497 左右 DAC 的寄存器，组合值为 `0x00、0x01、0x02、0x03、0x04、0x06`。这是芯片模式选择，不能当成可直接移植的 FIR 系数。原代码同时控制时钟、静音和硬件输出路径。

ICX1300 参数是确实可移植的 DSEE 数字处理部分。DMP-Z1 的双 AK4497、模拟放大、电源、负载与整机传递特性不由这个参数文件描述；目前声音经过电脑当前的 DAC／声卡，因此不能声称完整重现 DMP-Z1 整机声音。

## 验证与研究副本

- [ARM／C++ 完整对照](SONY-X86-VERIFICATION.json)：**336／336 组**通过，8,650,752 个 PCM 值和全部已分配状态逐位一致；其中新增 90 组 ICX1300，覆盖两种 DSEE 输入率、所有 DC／黑胶组合与不同信号。该范围不含硬件 Remaster。
- [实际流水线检查](DMP-Z1-PIPELINE-VERIFICATION.json)：1、2、3 个连续工作进程的完整输出相同；ICX1300 开关确实改变 PCM，缓存隔离且源文件未变。短样本首次产出约 0.27／0.39／0.39 秒；这是测试产出时间，不是所有曲目的设备启播延迟或持续性能承诺。
- [真实音乐时间—频率测量](SONY-SPECTROGRAM-VERIFICATION.json)：8 种开关组合各处理 60 秒；每组另取 5 秒与原 ARM 比较，完整输出及状态逐位一致。三项全开约 8.24 倍实时速度，首个半秒缓冲约 0.53 秒。
- [82 组持续实时测试](SONY-REALTIME-VERIFICATION.json)：每组 30 秒合成输入，覆盖 44.1／48 kHz 的全部相位／黑胶档位组合和 192 kHz 的单项／组合，全部快于实时，最慢约 6.60 倍。该范围仅为已移植的软件三音效，不验证硬件 Remaster 或整机声音。
- 原版运行文件仍位于 `vendor/sony-walkman`，新增配置仅 20 字节；C++ 指令图及 DSP DLL未变，相关指令已覆盖这份原参数。独立 EXE 包含新配置和哈希检查。参数研究测试不代表整机音色验证。研究源码、整个固件与测试向量不进入 EXE。
- [研究源码及复现命令](../research/sony-dmp-z1/README.md)保留 53 个原 GPL 文件、许可、哈希和分析脚本。固件旁既有 `Extracted-Research-2026-10-07` 研究副本另保存完整系统、启动镜像解压结果、GPL 原源码压缩包与最新播放器源码／验证文件；不包含个人音乐或凭据。

验证包含的是数字软件与相应控制路径；没有 DMP-Z1 实机逐样本、模拟回路、DSD Remaster 或完整整机听音对照。
