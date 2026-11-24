"""Handling of unexpected frames and values from the bus."""

import pytest

pytestmark = pytest.mark.asyncio

ANTIFREEZE = "T-Frostschutz"
STATE = "Betriebsart"
BPV = "BPV"

ANTIFREEZE_KEY = "FA 0A 00"
STATE_KEY = "FA C0 F6"
BPV_KEY = "FA C0 FB"
READ_BPV = "31 00 FA C0 FB 00 00"
HPSU_RESPONSE_ID = 0x180


async def test_short_frame_is_ignored(device):
    await device.wait_state(ANTIFREEZE, "Aus")
    start = len(device.log)

    # an antifreeze response one byte short: the 7th byte would be read beyond the frame
    await device.send_frame(HPSU_RESPONSE_ID, "D2 00 FA 0A 00 00")
    # frames are handled in order: once the RoCon frame is seen, the short one was handled before
    await device.rocon_set(ANTIFREEZE_KEY, "FF D8")
    await device.wait_state(ANTIFREEZE, "-4 °C")

    # both frames may be handled in the same loop, so the API reports only the last state: check the log
    # (regular reads answer FF 60 / FF D8, only the short frame starts with 00 00)
    handled = [line for line in device.log[start:] if f"{ANTIFREEZE}<" in line and "data<D2 00 FA 0A 00 00" in line]
    assert handled == []


async def test_value_outside_range_starting_at_zero_is_dropped(device):
    await device.wait_state(BPV, "50.0")

    await device.hpsu_set(BPV_KEY, "FF FF")  # 65535 %, outside [0, 100]
    await device.wait_reads(device.mark(), READ_BPV)

    assert device.states[BPV] == "50.0"
    assert (BPV, "65535.0") not in device.history


async def test_idle_loop_does_not_flood_the_log(device):
    # hpsu_host.yaml has neither the regulated supply setpoint nor tv/tr (temperature spread)
    await device.wait_state(BPV, "50.0")
    start = len(device.log)
    await device.wait_reads(device.mark(), READ_BPV)

    window = device.log[start:]
    assert not [line for line in window if "Entity not found" in line]
    assert not [line for line in window if "[PID" in line]


async def test_update_of_unconfigured_entity_is_skipped(device):
    # a change of Betriebsart updates thermal_power, which hpsu_host.yaml does not have
    # (nor flow_rate, tv, tr): nothing to calculate, nothing to report
    await device.wait_state(STATE, "Heizen")
    start = len(device.log)

    await device.hpsu_set(STATE_KEY, "00 04")  # heating -> hot water
    await device.wait_state(STATE, "Warmwasserbereitung")
    await device.wait_reads(device.mark(), READ_BPV)

    assert not [line for line in device.log[start:] if "[E]" in line]


async def test_unknown_value_shows_raw_key(device):
    await device.wait_state(STATE, "Heizen")

    await device.hpsu_set(STATE_KEY, "00 63")  # no option for 0x63

    await device.wait_state(STATE, "INVALID<0x0063>")
