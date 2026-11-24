# Tests

Two kinds of tests, both run without hardware, locally and in GitHub Actions
(`.github/workflows/test.yml`).

## Unit tests (GoogleTest)

Helper classes (`bidi_map`, `pid`, `scheduler`, `utils`) against the headers of a fixed
ESPHome release (`ESPHOME_GIT_TAG` in `CMakeLists.txt`).

Needs `cmake`, a C++20 compiler and `git`:

```sh
cd test
cmake -S . -B build
cmake --build build -j
ctest --test-dir build --output-on-failure
```

## Host tests (simulated HPSU)

`test/host/` builds the whole component with the ESPHome host platform as a Linux program.
Instead of the CAN driver it uses `hpsu_sim`, a simulated HPSU on a virtual bus
(`test/host/components/hpsu_sim`): it answers read requests from a register table, applies
set requests, drops values outside a register's limits (as the HPSU does) and logs every frame.

The pytest tests start the program, talk to it through the native API like Home Assistant
(select an option, wait for states) and check the frames on the bus. Two API actions let a test
play the other side: `hpsu_set` (the HPSU changes a value on its own, e.g. heating ->
defrosting) and `rocon_set` (the RoCon control panel writes a value).

```sh
python3 -m venv .venv && . .venv/bin/activate
pip install esphome==2026.11.0 pytest pytest-asyncio
cd test/host
pytest
```

`pytest` must be the one from the venv (the system pytest has no `aioesphomeapi`).

The first run compiles the program (a few minutes), later runs reuse the build.
By default the build goes to `test/host/.esphome/`. On a network mount (sshfs, Samba) put it on
the local disk instead and keep Python from writing bytecode next to the sources:

```sh
export ESPHOME_DATA_DIR=/tmp/hpsu-build PYTHONDONTWRITEBYTECODE=1
pytest
```

To skip compiling: `HPSU_HOST_BINARY=<path to program> pytest`.

The ESPHome build regenerates `components/daikin_rotex_can/translations.h/.cpp` (ignored by git,
the same files every ESPHome build of the component writes).

New scenario: add the registers it needs to `hpsu_host.yaml` (key, data, optional limits) and the
entities to the `entities:` section, then write the test like `test_optimized_defrosting.py`.
