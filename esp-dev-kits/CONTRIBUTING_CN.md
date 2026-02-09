# 贡献指南 ([English](CONTRIBUTING.md))

感谢您有兴趣为 **esp-dev-kits** 项目做出贡献！

我们欢迎社区各种形式的贡献，包括问题修复、文档完善以及功能建议。本指南旨在帮助您更高效地参与项目。

> **注意：** 此 GitHub 仓库是乐鑫内部 Git 仓库的公共镜像。
> 您提交的 Pull Request (PR) 将同步至我们的内部系统进行审核和测试，并在获得最终批准后合并到公共仓库。


---

## 目录

- [贡献前须知](#贡献前须知)
  - [法律要求](#法律要求)
  - [贡献范围](#贡献范围)
- [文档风格指南](#文档风格指南)
- [代码风格指南](#代码风格指南)
  - [总体原则](#总体原则)
  - [关键规范](#关键规范)
- [快速开始](#快速开始)
  - [分支命名规范](#分支命名规范)
  - [提交信息规范](#提交信息规范)
- [审核流程](#审核流程)
- [报告问题](#报告问题)


---

## 贡献前须知

在提交 PR 之前，请确保：

- 您的贡献内容为原创作品，或已正确标注来源且采用与本项目兼容的授权方式。
- 请勿提交任何机密、专有或其他受限制公开的内容。
- 贡献内容采用中文或英文编写，语言表达准确清晰。
- 修改内容逻辑合理，避免在单个 PR 中包含无关变更。
- 已查阅并确认未重复提交相关 [GitHub Issues](https://github.com/espressif/esp-dev-kits/issues) 和 PR。

如有疑问，也欢迎直接提交 PR，我们将为您提供反馈和建议。

### 法律要求

**贡献者协议：**

在我们接受您的贡献之前，需要您签署 [Espressif Contributor Agreement（乐鑫贡献者协议）](http://docs.espressif.com/projects/esp-idf/zh_CN/stable/contribute/contributor-agreement.html)。首次提交 PR 时，系统会自动提示您完成该协议签署。

**许可协议：**

本仓库采用双许可协议模式（代码采用 Apache 2.0，文档采用 CC BY-SA 4.0）。提交贡献即表示您同意您的贡献将按照相应的许可协议授权。详情请参阅 README 中的[开源许可协议](README_CN.md#开源许可协议)部分。

### 贡献范围

```text
esp-dev-kits/
├── docs/                  # 主文档源文件 (.rst 格式)
├── examples               # 各种开发板的示例项目
├── CONTRIBUTING.md        # 贡献指南（英文）
├── CONTRIBUTING_CN.md     # 贡献指南（中文）
├── README.md              # 项目概述与文档链接（英文）
├── README_CN.md           # 项目概述与文档链接（中文）
```

本仓库仅接受文档相关和示例相关的贡献，具体包括：

- `docs/` 目录下的文档文件
- `examples/` 目录下的示例文件
- 仓库根目录下的文档文件，如 `README.md`、`README_CN.md`、`CONTRIBUTING.md` 和 `CONTRIBUTING_CN.md`


---

## 文档风格指南

在为 `docs/` 目录贡献文档时，请遵循以下指南：

1. **文档格式**

- `docs/` 目录下的所有文档均应使用 reStructuredText（`.rst`）格式编写
- 主要语法请参考 [reStructuredText 基础语法](https://docs.espressif.com/projects/esp-docs/en/latest/writing-documentation/basic-syntax.html)

2. **文档构建**

- 本项目采用 [ESP-Docs](https://docs.espressif.com/projects/esp-docs/en/latest/index.html) 构建（乐鑫基于 Sphinx 开发的文档系统）
- 本地预览可参考 [本地构建文档指南](https://docs.espressif.com/projects/esp-docs/en/latest/building-documentation/building-documentation-locally.html)

3. **风格规范**

请遵循 [Espressif Manual of Style（乐鑫文档风格手册）](https://mos.espressif.com/) 中的格式和风格规范

4. **拼写检查**

本项目采用 [codespell](https://github.com/codespell-project/codespell) 进行拼写检查

> 建议在提交 PR 前，确保文档可以正常构建并通过 codespell 检查。


---

## 代码风格指南

在为 `examples/` 目录贡献代码时，请遵循以下编码规范：

### 总体原则

- 简洁明了，结构清晰
- 统一风格，易于维护
- 充分注释，易于理解
- 继承 [ESP-IDF 代码风格指南](https://docs.espressif.com/projects/esp-idf/zh_CN/latest/esp32/contribute/style-guide.html)
- 保留第三方代码已有规范

### 关键规范

**命名规范：**

- **函数名**：使用 `snake_case` 格式（小写加下划线）
  - 示例：`i2c_bus_create()`、`sensor_read_data()`
- **变量名**：使用 `snake_case` 格式（小写加下划线）
  - 静态全局变量：添加 `g_` 前缀（例如 `g_device_count`）
  - 静态局部变量：添加 `s_` 前缀（例如 `s_init_done`）
  - 定义变量时必须赋初值
- **类型定义**：使用 `snake_case` 格式加 `_t` 后缀
  - 示例：`typedef struct { ... } i2c_config_t;`
- **宏定义/常量**：使用 `UPPER_CASE` 格式（大写加下划线）

**代码格式：**

- **缩进**：使用 4 个空格（不使用制表符）
- **行长度**：最大 120 个字符
- **大括号**：
  - 函数定义：左大括号单独成行
  - 控制结构：左大括号与语句同行

```c
// 函数定义
void function_name(int arg)
{
    // 代码
}

// 控制结构
if (condition) {
    // 代码
} else {
    // 代码
}
```

**注释规范：**

- 单行注释使用 `//`
- 多行注释使用 `/* */` 或每行使用 `//`
- 头文件中的函数声明需添加 Doxygen 风格注释：

```c
/**
 * @brief 创建 I2C 总线实例
 * 
 * @param[in] port I2C 端口号
 * @param[in] conf I2C 配置指针
 * @return i2c_bus_handle_t 成功返回 I2C 总线句柄，失败返回 NULL
 */
i2c_bus_handle_t i2c_bus_create(i2c_port_t port, i2c_config_t *conf);
```

**文件组织：**

- 每个 `.c` 文件应尽量对应一个同名的 `.h` 头文件
- 头文件必须包含防重复引用的宏定义
- 头文件需添加 `extern "C"` 块以支持 C++ 混合编程

完整的代码风格指南请参考：
- [ESP-IDF 代码风格指南](https://docs.espressif.com/projects/esp-idf/zh_CN/latest/esp32/contribute/style-guide.html)
- [ESP-IDF API 文档编写指南](https://docs.espressif.com/projects/esp-idf/zh_CN/latest/esp32/contribute/documenting-code.html)


---

## 快速开始

参与贡献的流程如下：

1. Fork 本仓库
2. 从 `master` 分支创建一个新的分支
3. 按照下文规范完成修改
4. 提交 PR，并撰写清晰描述的标题

> 遵循以下分支命名和提交信息规范有助于加快审核进程。


### 分支命名规范

请使用以下前缀之一为分支命名：

- `docs/` – 文档更改
- `feature/` - 示例更新
- `bugfix/` – 问题修复、错别字或错误信息

**规则：**

- 仅使用小写字母（不使用大写字母）
- 使用下划线替换空格
- 每个分支只做一项任务（例如，修复链接和更新内容应创建不同的分支）

**示例**

- `docs/update_esp_prog_user_guide`
- `feature/update_esp32_p4_eye_demo`
- `bugfix/fix_broken_links`


### 提交信息规范

提交信息格式如下：

`<类型>: <描述>`

- **类型**：`docs`、`feature` 或 `bugfix`（必须与分支前缀匹配）
- **描述**：首字母大写，使用现在时态

**规则：**

- 以分支名称前缀开头（`docs:`、`feature:` 或 `bugfix:`）
- 用一行总结提交目的
- 每次提交专注于单一逻辑变更
- 使用现在时态并首字母大写

**示例**

- `docs: Add user guide for ESP32-S3-DevkitC-1`
- `feature: Update the demo for ESP32-P4-EYE`
- `bugfix: Fix broken links`

**注意：** 不要使用从分支名自动生成的提交信息（例如 `docs/update_esp32_s3_devkitc_1_board_figure`）


---

## 审核流程

1. 项目维护者将审核您的 PR， 并可能要求您做进一步的修改。
2. 审核通过后，PR 将同步至乐鑫内部 Git 系统进行测试。
3. 测试通过后，您的贡献将在下一次同步时发布至 GitHub 公共仓库。


---

## 报告问题

若需报告问题或提出建议，请使用 [GitHub Issues](https://github.com/espressif/esp-dev-kits/issues)：

1. 请使用清晰、描述明确的标题
2. 提供复现步骤或相关背景信息
3. 标注受影响的开发板（如适用）


---

感谢您愿意为 **esp-dev-kits** 项目做出贡献！您的贡献将帮助完善乐鑫文档，推动开发者社区的发展。
