# PS4 Overlay Framework

A GoldHEN-compatible plugin framework: a movable tabbed overlay, Circle + Triangle toggle,
an FPS counter, a configurable crosshair, a config-file system, and a pluggable
game-info interface. It contains no game-specific or gameplay-advantage features and is
intended for homebrew development and testing on titles you own.

## Layout

    src/            C sources (one module per subsystem)
    include/        headers
    config/         ovl_config.ini (settings), offsets.ini (per-title offsets)
    install/        INSTALL.txt shipped with the package
    .github/        build workflow
    Makefile        OpenOrbis build and packaging

Modules: input, draw, ui, features, config, offsets, hooks, main (entry points).
Function names are prefixed by subsystem: `input_`, `draw_`, `ui_`, `features_`,
`hooks_`, `ovl_`, plus `ovl_config_` and `ovl_settings_` in config.

## Building locally

1. Build or install the OpenOrbis PS4 toolchain and export its path:

       export OO_PS4_TOOLCHAIN=/path/to/OpenOrbis/PS4Toolchain

2. Build the module and the package:

       make
       make package

Output: `dist/ps4_overlay_plugin.zip` containing the `.sprx`, the config files, and install notes.

## Building in GitHub Actions

`.github/workflows/build.yml` runs on push and on manual dispatch. It installs host tools,
restores or builds the OpenOrbis toolchain (cached), builds the `.sprx`, packages the zip,
and uploads it as the workflow artifact `ps4_overlay_plugin`.

## Verification status

The source was written without access to the PS4 SDK or hardware, so the following must be
checked before relying on the build:

- **Toolchain paths, compiler flags, and library names** in the Makefile match your OpenOrbis version.
- **SPRX generation step** (`PRX_GEN` in the Makefile) uses the tool name shipped by your toolchain.
- **Toolchain build command** in the workflow matches the version you pin.
- **Pad reader** is a stub (`input_default_reader`). It must be bound to the pad API on hardware.
- **Render hook backend** is not implemented. It is target-specific code patching and is left as
  a supplied function (`hooks_set_backend`).
- **Module base** must be resolved at runtime for the target; it is read from `offsets.ini` for now.
- **Draw backend** is not bound, so commands are recorded but not rendered until a renderer is set
  with `draw_set_backend`.
- **Entity data provider** in `features.c` is a placeholder for per-title reading.

Nothing in this framework has been run on a console. Treat the first build as a compile check and
test on your own title before expecting any on-screen output.
