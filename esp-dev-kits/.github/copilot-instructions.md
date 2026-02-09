# AI Coding Agent Instructions for esp-dev-kits

This repository provides development documentation and example applications for Espressif development boards (ESP32 family).

## Project Overview

**esp-dev-kits** is a curated collection of user guides, hardware resources, firmware, and demo code for Espressif development boards. The project uses **ESP-IDF** as its foundational framework and accepts two main types of contributions:
- **Documentation** (`.rst` format in `docs/`)
- **Example code** (ESP-IDF projects in `examples/`)

## Repository Structure

```
esp-dev-kits/
├── docs/                      # Sphinx/ESP-Docs documentation (reStructuredText .rst)
│   ├── en/                    # English documentation
│   ├── zh_CN/                 # Chinese documentation
│   └── conf_common.py         # Shared Sphinx configuration
├── examples/                  # ESP-IDF example projects by board
│   ├── esp32-p4-eye/
│   ├── esp32-p4-function-ev-board/
│   ├── esp32-s3-lcd-ev-board/
│   └── [11 more board examples]
├── tools/
│   ├── build_apps.py          # CI build script using idf_build_apps
│   └── generateFiles.py       # Generates firmware TOML/JSON metadata
└── CONTRIBUTING.md            # Full contribution guidelines
```

## Critical Knowledge for Example Projects

### Build System Architecture
All examples use **CMake** (not Make) with ESP-IDF's build system:
- Every project requires: `cmake_minimum_required(VERSION 3.5)` + `include($ENV{IDF_PATH}/tools/cmake/project.cmake)`
- Components register via `idf_component_register()` in their `CMakeLists.txt`
- External components specified via `set(EXTRA_COMPONENT_DIRS ...)` before `project()` call
- **Reference**: [esp32-s3-usb-otg examples](examples/esp32-s3-usb-otg/examples/factory/CMakeLists.txt#L8) shows correct ordering

### Typical Project Layout
```
<board-example>/
├── examples/                  # Multiple subprojects per board
│   ├── factory_demo/          # Main demo (often factory firmware)
│   ├── get-started/
│   └── [feature-specific examples]
├── common_components/         # Shared BSP/driver code
│   ├── brookesia_system_core/ # Advanced systems (GUI, AI)
│   └── [board-specific drivers]
├── components/
│   ├── bsp_<board_name>/      # Board Support Package
│   ├── display_screen/        # LVGL screen management
│   └── [peripheral drivers]
└── README.md                  # Board-specific setup docs
```

### Key Build Patterns
- **Compile Flags**: Suppress warnings aggressively (`-Wno-unused-function`, `-Wno-deprecated-declarations`)
- **Component Suppression**: Use `idf_component_get_property()` to selectively suppress warnings per component
- **Build Apps Discovery**: CI uses `idf_build_apps.find_apps()` which auto-discovers projects by `CMakeLists.txt` location

## Code Style Requirements

### Naming Conventions (Mandatory for examples)
Follow [ESP-IDF Style Guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/contribute/style-guide.html):
- **Functions**: `snake_case` (e.g., `i2c_bus_create()`, `sensor_read_data()`)
- **Variables**: `snake_case` with prefixes:
  - Global static: `g_device_count`
  - Local static: `s_init_done`
  - Always initialize on declaration
- **Types**: `snake_case_t` suffix (e.g., `i2c_config_t`)
- **Macros/Constants**: `UPPER_CASE` (e.g., `MAX_BUFFER_SIZE`)

### Code Formatting
- **Indentation**: 4 spaces (no tabs)
- **Line length**: Max 120 characters
- **Braces**:
  - Functions: opening brace on new line
  - Control flow: opening brace on same line (`if (x) { ... }`)

## Documentation Workflow

### Format & Build System
- **Source format**: reStructuredText (`.rst`)
- **Build system**: ESP-Docs (Sphinx-based)
- **Build command**: Documentation built via CI; see `docs/README.md` for local builds
- **Spell check**: Project uses [codespell](https://github.com/codespell-project/codespell)

### Documentation Organization
- English docs in `docs/en/` with board-specific subdirectories
- Chinese docs in `docs/zh_CN/` mirroring English structure
- Common config in `docs/conf_common.py` applied to both languages
- Custom Sphinx roles available: `:dev-kits_file:`, `:example:`, `:component:` (see Makefile in docs/)

## Common Development Tasks

### Building an Example

⚠️ **CRITICAL**: You must navigate INTO the example project directory (where CMakeLists.txt exists) before running any `idf.py` commands. The repo root has no CMakeLists.txt.

```bash
# Navigate to a specific example project (e.g., factory demo)
cd examples/esp32-p4-eye/examples/factory_demo

# Set target for that board
idf.py set-target esp32p4

# Build, flash, and monitor
idf.py build
idf.py flash -p /dev/ttyUSB0
idf.py monitor -p /dev/ttyUSB0
```

**Example project paths** (all have CMakeLists.txt):
- `examples/esp32-p4-eye/examples/factory_demo/` → ESP32-P4 target
- `examples/esp32-s3-usb-otg/examples/factory/` → ESP32-S3 target
- `examples/esp32-s2-hmi-devkit-1/examples/get-started/hello_world/` → ESP32-S2 target

### Discovering Example Projects in CI
- Tool: `tools/build_apps.py` (uses `idf_build_apps`)
- Scans `examples/` recursively for `CMakeLists.txt` files
- Builds configurable via `upload_project_config.yml` (board/target/sdkconfig combinations)
- Generates firmware metadata via `tools/generateFiles.py` → TOML for ESP Launchpad

### Adding a New Example
1. Create `examples/<board>/<feature>/CMakeLists.txt` with proper project setup
2. Create `main/` component with entry point
3. Include shared components via `set(EXTRA_COMPONENT_DIRS ...)`
4. Update `upload_project_config.yml` if CI should build it
5. Add board-specific documentation reference in example `README.md`

## Integration Points & Dependencies

- **ESP-IDF dependency**: Projects specify via `$IDF_PATH` environment variable; CI tools invoke `_get_idf_version()` for compatibility
- **External components**: Common imports:
  - `lvgl__lvgl` (LVGL GUI framework)
  - `espressif__esp-dl` (AI/edge computing library)
  - Board-specific BSP components
- **Factory firmware pattern**: Many boards include a `factory_demo/` showing all peripherals; this is reference implementation

## Documentation Conventions

- **Dual language policy**: Every user guide must have English and Chinese versions
- **Cross-references**: Use Sphinx roles to reference files across examples/components
- **Copyright header**: SPDX-License-Identifier comments required (Apache 2.0 for code, CC-BY-SA for docs)
- **See also**: Link to [CONTRIBUTING.md](../CONTRIBUTING.md) for detailed style rules

## Quick Checklist for AI Agents

When adding code to `examples/`:
- [ ] Use CMake (not Make); validate `include($ENV{IDF_PATH}/tools/cmake/project.cmake)` ordering
- [ ] Follow `snake_case` naming; initialize all variables
- [ ] Set line max 120 chars; 4-space indentation
- [ ] Suppress non-critical warnings per component
- [ ] Reference shared components via `EXTRA_COMPONENT_DIRS`
- [ ] Update `upload_project_config.yml` if new build target needed
- [ ] Add board setup docs if creating new board example

When updating `docs/`:
- [ ] Use reStructuredText (`.rst`) format
- [ ] Keep English & Chinese versions in sync
- [ ] Run `codespell` to validate
- [ ] Test build locally via `docs/README.md` instructions
- [ ] Use Sphinx role conventions for cross-references
