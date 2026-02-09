# Contributing Guide ([中文](CONTRIBUTING_CN.md))

Thank you for your interest in contributing to the **esp-dev-kits** project!

We welcome community contributions—including bug fixes, documentation improvements, and feature suggestions. This document outlines the recommended process to help you contribute effectively.

> **Note:** This GitHub repository is a public mirror of Espressif's internal Git repository.
> Your Pull Request (PR) will be synchronized with our internal system for review, testing, and final approval before being merged publicly.


---

## Table of Contents

- [Before You Contribute](#before-you-contribute)
  - [Legal Requirements](#legal-requirements)
  - [Contribution Scope](#contribution-scope)
- [Documentation Style Guide](#documentation-style-guide)
- [Code Style Guide](#code-style-guide)
  - [General Principles](#general-principles)
  - [Key Conventions](#key-conventions)
- [Getting Started](#getting-started)
  - [Branch Naming Conventions](#branch-naming-conventions)
  - [Commit Message Conventions](#commit-message-conventions)
- [Review Process](#review-process)
- [Issue Reporting](#issue-reporting)


---

## Before You Contribute

Before submitting a PR, please ensure that:

- Your contribution is your own original work, or properly credited and licensed under terms compatible with this project.
- Do not submit any confidential, proprietary, or otherwise restricted content.
- Your content is clearly written in English or Chinese, with correct grammar and spelling.
- Your changes are logically grouped—avoid mixing unrelated fixes or updates in a single PR.
- You have reviewed existing [GitHub Issues](https://github.com/espressif/esp-dev-kits/issues) and PRs to avoid duplication.

Not sure about something? Submit your PR anyway—we're happy to provide feedback.

### Legal Requirements

**Contributor Agreement:**

Before we can accept your contribution, you must sign the [Espressif Contributor Agreement](http://docs.espressif.com/projects/esp-idf/en/stable/contribute/contributor-agreement.html). You will be prompted automatically when opening your first PR.

**Licensing:**

This repository uses a dual licensing model (Apache 2.0 for code, CC BY-SA 4.0 for documentation). By submitting a contribution, you agree that your contributions will be licensed accordingly. For details, see the [Open Source Licensing](README.md#open-source-licensing) section in the README.

### Contribution Scope

```text
esp-dev-kits/
├── docs/                  # Main documentation source files (.rst)
├── examples               # Example projects for various boards
├── CONTRIBUTING.md        # Contribution guide (English)
├── CONTRIBUTING_CN.md     # Contribution guide (Chinese)
├── README.md              # Project overview and documentation links (English)
├── README_CN.md           # Project overview and documentation links (Chinese)
```

This repository only accepts documentation-related and example contributions only. Specifically:

- Files under the `docs/` directory
- Files under the `examples/` directory
- Root-level documentation files such as `README.md`, `README_CN.md`, `CONTRIBUTING.md`, and `CONTRIBUTING_CN.md`


---

## Documentation Style Guide

When contributing to the `docs/` directory, please follow these guidelines:

1. **File Format**

   - All content under `docs/` must be written in reStructuredText (`.rst`) format.
   - Refer to the [reStructuredText Basic Syntax](https://docs.espressif.com/projects/esp-docs/en/latest/writing-documentation/basic-syntax.html) for key syntax rules.

2. **Building Documentation**

   - This project is built using [ESP-Docs](https://docs.espressif.com/projects/esp-docs/en/latest/index.html) (Espressif's Sphinx-based system).
   - Follow the instructions in [Building Documentation Locally](https://docs.espressif.com/projects/esp-docs/en/latest/building-documentation/building-documentation-locally.html) to preview your changes.

3. **Style Conventions**

Follow the [Espressif Manual of Style](https://mos.espressif.com/) for formatting and stylistic conventions.

4. **Spell Checking**

This project uses [codespell](https://github.com/codespell-project/codespell) for spell checking.

> Before submitting a PR, it is recommended to verify that your changes pass codespell checks and that the documentation builds successfully.


---

## Code Style Guide

When contributing code to the `examples/` directory, please follow these coding standards:

### General Principles

- Keep code simple, clear, and well-structured
- Maintain consistent style for easy maintenance
- Add sufficient comments for readability
- Inherit and follow the [ESP-IDF Style Guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/contribute/style-guide.html)
- Preserve existing style conventions for third-party code

### Key Conventions

**Naming Conventions:**

- **Functions**: Use `snake_case` (lowercase with underscores)
  - Example: `i2c_bus_create()`, `sensor_read_data()`
- **Variables**: Use `snake_case` (lowercase with underscores)
  - Static global variables: Add `g_` prefix (e.g., `g_device_count`)
  - Static local variables: Add `s_` prefix (e.g., `s_init_done`)
  - Always initialize variables when defining them
- **Types**: Use `snake_case` with `_t` suffix
  - Example: `typedef struct { ... } i2c_config_t;`
- **Macros/Constants**: Use `UPPER_CASE` with underscores

**Code Formatting:**

- **Indentation**: Use 4 spaces (no tabs)
- **Line length**: Maximum 120 characters
- **Braces**: 
  - Function definitions: Place opening brace on a new line
  - Control structures: Place opening brace on the same line
  
```c
// Function definition
void function_name(int arg)
{
    // code here
}

// Control structure
if (condition) {
    // code here
} else {
    // code here
}
```

**Comments:**

- Use `//` for single-line comments
- Use `/* */` or `//` on each line for multi-line comments
- Add Doxygen-style comments for function declarations in header files:

```c
/**
 * @brief Create an I2C bus instance
 * 
 * @param[in] port I2C port number
 * @param[in] conf Pointer to I2C configuration
 * @return i2c_bus_handle_t I2C bus handle if successful, NULL if failed
 */
i2c_bus_handle_t i2c_bus_create(i2c_port_t port, i2c_config_t *conf);
```

**File Organization:**

- Each `.c` file should have a corresponding `.h` header file when possible
- Header files must include include guards
- Add `extern "C"` blocks in headers for C++ compatibility

For complete style guidelines, refer to:
- [ESP-IDF Style Guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/contribute/style-guide.html)
- [ESP-IDF API Documentation Guidelines](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/contribute/documenting-code.html)


---

## Getting Started

To contribute:

1. **Fork** the repository
2. **Create a new branch** from `master`
3. **Make your changes** following the conventions below
4. **Submit a PR** with a clear, descriptive title

> Following the branch and commit message conventions below will help speed up the review process.


### Branch Naming Conventions

Use one of the following prefixes to name your branch:

- `docs/` – Documentation changes
- `feature/` – Example code updates
- `bugfix/` – Bug fixes, typos, or incorrect information

**Rules:**

- Use lowercase only (no uppercase letters)
- Replace spaces with underscores
- Keep one task per branch (e.g., create separate branches for fixing links vs. updating content)

**Examples**

- `docs/update_esp_prog_user_guide`
- `feature/update_esp32_p4_eye_demo`
- `bugfix/fix_broken_links`


### Commit Message Conventions

Follow this structure for all commits:

`<Type>: <Description>`

- **Type**: `docs`, `feature`, or `bugfix` (must match your branch prefix)
- **Description**: Capitalize the first word and use present tense

**Rules:**

- Start with the branch name prefix (`docs:`, `feature:`, or `bugfix:`)
- Summarize the purpose in one line
- Keep each commit focused on a single logical change
- Use present tense and capitalize the initial letter

**Examples**

- `docs: Add user guide for ESP32-S3-DevkitC-1`
- `feature: Update the demo for ESP32-P4-EYE`
- `bugfix: Fix broken links`

**Note:** Do not use the commit message auto-generated from the branch name (e.g., `docs/update_esp32_s3_devkitc_1_board_figure`)


---

## Review Process

1. Maintainers will review your PR and may request changes.
2. Once approved, your PR is synchronized with Espressif's internal Git system for automated testing.
3. After passing internal tests, your contribution will be published to the public GitHub repository on the next sync.


---

## Issue Reporting

To report issues or suggest improvements, please use [GitHub Issues](https://github.com/espressif/esp-dev-kits/issues):

1. Use a clear and descriptive title
2. Provide reproduction steps or relevant context
3. Tag the affected development board(s), if applicable


---

Thank you for helping improve the **esp-dev-kits** project! Your contributions support the developer community and help make Espressif's documentation better for everyone.
