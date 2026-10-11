# Sony 原版运行文件

本私有增强副本按用户要求保留十个原版运行文件；程序包使用相同文件，启动和打包时校验固定 SHA-256。来源是 Sony DMP-Z1 官方 1.02，AI 为 DSX 3.0.7；详见 [组件清单](../../scripts/sony-walkman/assets.json)和 [来源／验证边界](../../docs/SONY-WALKMAN.md)。

`DseeAi.dcfg` 保留既有 ICX1291 配置。新增 `DseeAi_HP_ICX1300.dcfg` 是 DMP-Z1 的 `DseeAi::LoadCoefFiles` 原版入口实际选择的配置；模型字节相同，研究 CLI 可选择原参数，不修改 Sony 算法。该文件属于 DSEE 参数，不能代替 DMP-Z1 整机调音；正式界面没有新增第二个 DSEE 开关。DSD Remaster 的 MT8590 硬件编码器与 AK4497 芯片滤波均尚未还原为电脑软件；研究证据见 [DMP-Z1 研究记录](../../docs/DMP-Z1-RESEARCH.md)。

这些文件未重编译或改写，不改授本项目 MIT 许可。运行库包含原版 ARM libm 和 libc++。本轮仅用于用户授权的私有副本，未公开上传。完整固件、研究解包和原歌不放入此目录。
