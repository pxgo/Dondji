<div align="center">

# 🚀 Dondji（叮咚鸡）

> 开源对讲机固件项目 · 基于 PY32F071 的 UV-K1 / UV-K5·K6 V3 固件增强

<p>
  <a href="./README.md">🏠 首页</a> |
  <a href="./README.en.md">🇺🇸 English</a>
</p>

</div>

---

## Maintainer

**BD1AHN**

## Official Website

https://ethanyan6.github.io/Dondji/

---

## 商业合作声明

叮咚鸡（Dondji）源码采用 **Apache License 2.0** 开源协议。

源码开放使用，包括在遵守 Apache License 2.0 条款的前提下用于商业产品。

但是，以下内容属于 Dondji 项目的品牌及官方身份资产：

* 叮咚鸡（Dondji）名称；
* Dondji Logo；
* 官方网站；
* 官方刷机入口；
* 官方身份；
* 官方认证；
* 官方合作关系。

如商业主体希望获得以下品牌或官方合作权益：

* Dondji 品牌授权；
* Dondji 联名合作；
* 官方认证；
* 官方合作关系；
* 官方品牌宣传或支持；

请联系：

**BD1AHN**

> 商业使用 Dondji 源代码与获得 Dondji 品牌或官方合作授权属于不同事项。

详细说明：

* 商业使用说明：[COMMERCIAL_USE.md](https://github.com/EthanYan6/Dondji/blob/motorola_r7/COMMERCIAL_USE.md)
* 品牌政策：[TRADEMARK.md](https://github.com/EthanYan6/Dondji/blob/motorola_r7/TRADEMARK.md)


---

## ✨ 功能特性

* 基于 `F4HWN` **v5.3.1** 开发；当前 Dondji 发布版本 **v5.2.2**
* 仿 Motorola R7 UI（分支：`motorola_r7`）+ 手机风格图标菜单
* 双 VFO / 仅主信道（MAIN ONLY）界面优化（参考 UV-KX-firmware）
* 锁屏 / 键盘锁界面重绘
* 中英双语界面，运行时可切换语言
* 菜单重构与 UI 整理
* 全量 GB2312 中文字库（SPI Flash，6766 字）+ 九宫格拼音（T9）信道命名
* 浏览器在线刷机：固件 / 字库 / 校准 / 配置 / 写频 / 开机画面
* 频谱（`F+5`）：简易 / 专业模式 + 瀑布图
* CW 练习器（`F+7`）
* MDC1200 Roger / 机组 ID（收发弹窗）
* 中英文使用手册
* Fusion 版本常见能力：独立收音机页、自定义开机画面 / 提示音、收发计时等

---

## 4D2 版本与贪吃蛇

`4D2` 是独立编译预设：继承 Fusion，保留 FM 广播收音机等功能，关闭频谱并加入贪吃蛇；不改变原有 Custom/Fusion/Game，也不是运行时切换模式。

```text
cmake --preset 4D2
cmake --build --preset 4D2 --parallel 4
```

产物：`build/4D2/Dondji.4d2.bin`（同时生成 ELF、HEX 和 map）。也可使用 `./compile-with-docker.sh 4D2`，但该脚本会删除整个 `build` 目录，使用前先保留需要的产物。

入口：**MENU → 游戏 / Games → 贪吃蛇 / Snake**。

| 按键 | 作用 |
| --- | --- |
| MENU | 列表中开始；游戏中暂停/继续；结束后重新开始 |
| 2 / 4 / 6 / 8 | 上 / 左 / 右 / 下 |
| EXIT | 游戏返回列表；列表返回游戏图标 |

- 游戏列表及游戏期间暂停对讲 RX/TX，顶栏显示 `RF OFF`；退出后恢复。不要在需要保持收听时使用游戏。
- 只允许从空闲对讲状态进入；接收、发射、扫描、监听、FM 广播、低电量或串口配置期间会拒绝进入。需要先停止相关业务，再进入游戏。
- 退出后必须释放 PTT、侧键及矩阵按键，连续确认释放后才允许新的发射操作。低电量会退出游戏；收到有效 USB/UART 配置命令也会先退出游戏再处理命令。
- 无存档和高分写入，不改变外置 Flash 数据布局；中文标签使用现有字库，检测不到匹配字库时游戏入口/列表回退英文。
- 实测 Arm GNU 13.3.Rel1、Release/LTO：Flash **114140 / 120832 B**，RAM 链接占用 **13336 / 16384 B**。后者包含最小堆栈保留，不代表真机栈峰值已验证。
- `ENABLE_GAME_SNAKE` 可单独关闭。第一版要求 F4HWN，暂不支持与 VOICE、REGA 或原始 BK 寄存器调试写入功能共同启用，CMake 会拒绝这些组合。

测试与风险记录见 [`analysis_file/game-snake-plan.md`](analysis_file/game-snake-plan.md)，主机测试见 [`tests/README.md`](tests/README.md)。尚未进行真机显示、射频恢复和栈水位验收；构建成功不等于可直接无风险刷入。

---

## 📸 主界面

<img width="643" height="407" src="https://github.com/user-attachments/assets/2097c20d-58fc-4577-ba84-dbfc83876e03" />

---

## 🧭 如何进入 MAIN ONLY（仅主信道）

1. 按 Menu
2. 进入图标菜单第 3 项：**显示**
3. 选择第 2 项：**接收模式**
4. 选择：**仅主信道**
5. 退出菜单

---

## ⏱ 计时功能

Fusion 等默认编译预设已开启收发计时（`ENABLE_FEAT_F4HWN_RX_TX_TIMER`）。

如需在自定义编译中显式开启：

```bash
-DENABLE_FEAT_F4HWN_RX_TX_TIMER=ON
```

---

## 🔀 双 VFO 页面

<img width="643" height="407" alt="image" src="https://github.com/user-attachments/assets/d606de69-2cc5-4f27-871b-d4d28c5e967e" />

---

## 🌐 语言切换

Menu → 显示 → 显示语言 → 中文 / English

---

## 🌐 在线刷机

无需安装任何软件，浏览器直接刷入固件和字库。

👉 **https://ethanyan6.github.io/Dondji/**

| 功能 | 说明 |
|------|------|
| 刷固件 | 从 GitHub Releases 拉取最新固件，或选择本地 .bin 文件 |
| 刷字库 | 6766 个中文字符（GB2312 全部汉字）写入 SPI Flash，支持信道中文命名 |
| 备份 / 恢复校准 | 导出或恢复设备校准数据 |
| 备份 / 恢复配置 | 导出或恢复菜单与按键等配置 |
| 写频 | 信道与相关配置编程（配合本固件） |
| 开机画面 | 上传自定义 128×64 开机图片 |

**步骤：**
1. Chrome / Edge 打开 [刷机页面](https://ethanyan6.github.io/Dondji/)
2. **刷固件**：按住 PTT 开机 → USB 连电脑 → 点「刷固件」
3. **刷字库**：固件启动后 → USB 连电脑 → 点「刷字库」

> 刷字库需先刷入本固件。字库通过 USB 写入 SPI Flash，首次写入后固件启动时自动跳过重复写入。

**字库技术参数：**

| 参数 | 值 |
|------|------|
| 字库文件 | `docs/font/cn_font.bin` |
| SPI Flash 起始地址 | `0x024000` |
| 字符数量 | 6766 个（GB2312 全部汉字） |
| 字库大小 | 205,367 字节 (约 200 KB) |
| SPI Flash 占用 | 约 9.8% (总容量 2MB) |

---

## ⚠️ 注意

* 刷机存在风险
* 本项目为非官方固件

---

## ❤️ 支持

如果觉得不错，欢迎点个 ⭐

---

## 🙏 致谢

* BA4QHC
