"""Host tests: the component runs as a Linux program against a simulated HPSU.

The firmware is built from hpsu_host.yaml (ESPHome host platform). Each test starts a fresh
process with empty preferences and talks to it through the native API like Home Assistant.
Bus traffic is taken from the simulator log (tag `hpsu_sim`).

Set HPSU_HOST_BINARY to use an already built program instead of compiling.
"""

from __future__ import annotations

import asyncio
import math
import os
import re
import shutil
import subprocess
import sys
from collections.abc import Callable
from pathlib import Path

import pytest
import pytest_asyncio
from aioesphomeapi import APIClient

HERE = Path(__file__).parent
YAML = HERE / "hpsu_host.yaml"
API_PORT = 16053
TIMEOUT_S = 20.0

ANSI = re.compile(r"\x1b\[[0-9;]*m")
UNKNOWN = "unknown"  # state without a value (not read yet, or NaN)
SIM_LINE = re.compile(r"\[hpsu_sim[^\]]*\]: (.*)$")


@pytest.fixture(scope="session")
def binary() -> Path:
    if path := os.environ.get("HPSU_HOST_BINARY"):
        return Path(path)
    esphome = shutil.which("esphome") or str(Path(sys.executable).parent / "esphome")
    subprocess.run([esphome, "compile", str(YAML)], cwd=HERE, check=True)
    # ESPHOME_DATA_DIR moves the build output elsewhere (e.g. off a network mount)
    data_dir = Path(os.environ.get("ESPHOME_DATA_DIR", HERE / ".esphome"))
    return data_dir / "build/hpsu-host/.pioenvs/hpsu-host/program"


class Device:
    """Running host program plus API client; every wait is event driven with a timeout."""

    def __init__(self, process: asyncio.subprocess.Process) -> None:
        self.process = process
        self.client: APIClient | None = None
        self.log: list[str] = []          # every output line of the program
        self.bus: list[str] = []          # simulator log lines, e.g. "rx 0x680: 30 00 FA 0A 00 FF 60"
        self.states: dict[str, str] = {}  # entity name -> current state as text
        self.history: list[tuple[str, str]] = []  # every state received, in order: (name, state)
        self._keys: dict[str, int] = {}
        self._names: dict[int, str] = {}
        self._services: dict = {}
        self._changed = asyncio.Event()
        self._reader = asyncio.create_task(self._read_output())

    async def _read_output(self) -> None:
        assert self.process.stdout is not None
        while line := await self.process.stdout.readline():
            text = ANSI.sub("", line.decode(errors="replace")).rstrip()
            self.log.append(text)
            if match := SIM_LINE.search(text):
                self.bus.append(match.group(1))
                self._changed.set()

    async def wait_for(self, predicate: Callable[[], bool], what: str, timeout: float = TIMEOUT_S) -> None:
        async def loop() -> None:
            while not predicate():
                self._changed.clear()
                await self._changed.wait()

        try:
            await asyncio.wait_for(loop(), timeout)
        except TimeoutError:
            pytest.fail(f"timeout waiting for {what}; states={self.states}; bus tail={self.bus[-15:]}")

    async def connect(self) -> None:
        # The simulator logs as soon as the component polls, the API is up by then
        await self.wait_for(lambda: any(line.startswith("rx ") for line in self.bus), "first bus request")
        self.client = APIClient("127.0.0.1", API_PORT, None)
        await self.client.connect(login=True)
        entities, services = await self.client.list_entities_services()
        for entity in entities:
            self._keys[entity.name] = entity.key
            self._names[entity.key] = entity.name
        self._services = {service.name: service for service in services}
        self.client.subscribe_states(self._on_state)

    def _on_state(self, state) -> None:
        name = self._names.get(state.key)
        if name is None:
            return
        value = state.state
        if getattr(state, "missing_state", False) or (isinstance(value, float) and math.isnan(value)):
            self.states[name] = UNKNOWN
        else:
            self.states[name] = ("An" if value else "Aus") if isinstance(value, bool) else str(value)
        self.history.append((name, self.states[name]))
        self._changed.set()

    # ---- actions (like Home Assistant / the HPSU) ----

    def select(self, name: str, option: str) -> None:
        assert self.client is not None
        self.client.select_command(self._keys[name], option)

    async def hpsu_set(self, key: str, data: str) -> None:
        """The HPSU changes a value on its own."""
        assert self.client is not None
        await self.client.execute_service(self._services["hpsu_set"], {"key": key, "data": data})

    async def rocon_set(self, key: str, data: str) -> None:
        """The RoCon control panel writes a value."""
        assert self.client is not None
        await self.client.execute_service(self._services["rocon_set"], {"key": key, "data": data})

    async def send_frame(self, can_id: int, data: str) -> None:
        """Any device puts a frame on the bus as given (any length)."""
        assert self.client is not None
        await self.client.execute_service(self._services["send_frame"], {"can_id": can_id, "data": data})

    # ---- waits and checks ----

    async def wait_state(self, name: str, value: str) -> None:
        await self.wait_for(lambda: self.states.get(name) == value, f"{name} == {value}")

    def mark(self) -> int:
        """Position in the bus log; use with frames_since()."""
        return len(self.bus)

    def frames_since(self, mark: int, prefix: str = "rx 0x680: ") -> list[str]:
        return [line[len(prefix):] for line in self.bus[mark:] if line.startswith(prefix)]

    async def wait_frame(self, mark: int, frame: str) -> None:
        await self.wait_for(lambda: frame in self.frames_since(mark), f"frame {frame}")

    async def wait_reads(self, mark: int, command: str, count: int = 2) -> None:
        """Wait until the ESP has read a register `count` times since `mark` (e.g. "31 00 FA 01 12 00 00")."""
        await self.wait_for(lambda: self.frames_since(mark).count(command) >= count, f"{count} reads {command}")

    async def close(self) -> None:
        if self.client is not None:
            await self.client.disconnect(force=True)
        if self.process.returncode is None:
            self.process.terminate()
            try:
                await asyncio.wait_for(self.process.wait(), 5)
            except TimeoutError:
                self.process.kill()
                await self.process.wait()
        self._reader.cancel()


@pytest_asyncio.fixture
async def device(binary: Path, tmp_path: Path):
    env = {**os.environ, "ESPHOME_PREFDIR": str(tmp_path)}  # fresh preferences for each test
    process = await asyncio.create_subprocess_exec(
        str(binary), stdout=asyncio.subprocess.PIPE, stderr=asyncio.subprocess.STDOUT, env=env
    )
    dev = Device(process)
    try:
        await dev.connect()
        yield dev
    finally:
        await dev.close()
