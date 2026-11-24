"""Optimized defrosting (Abtau-Optimierung) and T-Frostschutz.

- Optimized defrosting and T-Frostschutz exclude each other, whichever side changes
  (Home Assistant or the RoCon control panel).
- While defrosting, optimized defrosting switches the operating mode to summer
  (bypass valve 100 %) and restores it afterwards.
"""

import pytest

pytestmark = pytest.mark.asyncio

OPT = "Abtau-Optimierung"
ANTIFREEZE = "T-Frostschutz"
MODE = "Betriebsmodus"
STATE = "Betriebsart"

ANTIFREEZE_KEY = "FA 0A 00"
STATE_KEY = "FA C0 F6"
READ_MODE = "31 00 FA 01 12 00 00"

SET_ANTIFREEZE_OFF = "30 00 FA 0A 00 FF 60"     # -16.0 °C = off
SET_ANTIFREEZE_MINUS_4 = "30 00 FA 0A 00 FF D8"
SET_MODE_SUMMER = "30 00 FA 01 12 05 00"
SET_MODE_HEATING = "30 00 FA 01 12 03 00"


async def start(device, *, antifreeze: str = "Aus") -> None:
    await device.wait_state(MODE, "Heizen")
    await device.wait_state(STATE, "Heizen")
    if antifreeze == "-4 °C":
        await device.hpsu_set(ANTIFREEZE_KEY, "FF D8")
    await device.wait_state(ANTIFREEZE, antifreeze)
    await device.wait_state(OPT, "Aus")


async def enable_optimization(device) -> None:
    mark = device.mark()
    device.select(OPT, "An")
    await device.wait_state(OPT, "An")
    # the set may be queued (delay between requests): wait until the HPSU has it
    await device.wait_for(
        lambda: f"set {ANTIFREEZE_KEY} = FF 60 from esp: accepted" in device.bus[mark:],
        "antifreeze off accepted",
    )
    await device.wait_state(ANTIFREEZE, "Aus")


# ---------- interlock ----------

async def test_enabling_optimization_sets_antifreeze_off(device):
    await start(device, antifreeze="-4 °C")
    mark = device.mark()

    device.select(OPT, "An")

    # the raw map key is sent unscaled (was 30 00 FA 0A 00 F9 C0 = -160 °C, dropped by the HPSU)
    await device.wait_frame(mark, SET_ANTIFREEZE_OFF)
    await device.wait_state(ANTIFREEZE, "Aus")
    assert f"set {ANTIFREEZE_KEY} = FF 60 from esp: accepted" in device.bus[mark:]
    assert device.states[OPT] == "An"


async def test_antifreeze_selected_in_ha_disables_optimization(device):
    await start(device)
    await enable_optimization(device)
    mark = device.mark()

    device.select(ANTIFREEZE, "-4 °C")

    await device.wait_frame(mark, SET_ANTIFREEZE_MINUS_4)
    await device.wait_state(OPT, "Aus")
    assert f"set {ANTIFREEZE_KEY} = FF D8 from esp: accepted" in device.bus[mark:]


async def test_antifreeze_set_on_rocon_disables_optimization(device):
    await start(device)
    await enable_optimization(device)

    await device.rocon_set(ANTIFREEZE_KEY, "FF D8")

    await device.wait_state(ANTIFREEZE, "-4 °C")
    await device.wait_state(OPT, "Aus")


async def test_antifreeze_changed_by_hpsu_disables_optimization(device):
    await start(device)
    await enable_optimization(device)

    await device.hpsu_set(ANTIFREEZE_KEY, "FF D8")

    await device.wait_state(ANTIFREEZE, "-4 °C")
    await device.wait_state(OPT, "Aus")


async def test_disabling_optimization_keeps_antifreeze(device):
    await start(device)
    await enable_optimization(device)
    mark = device.mark()

    device.select(OPT, "Aus")
    await device.wait_state(OPT, "Aus")
    await device.wait_reads(mark, READ_MODE)

    assert not [f for f in device.frames_since(mark) if f.startswith("30 ")]
    assert device.states[ANTIFREEZE] == "Aus"


# ---------- defrosting ----------

async def test_defrosting_switches_to_summer_and_back(device):
    await start(device)
    await enable_optimization(device)
    mark = device.mark()

    await device.hpsu_set(STATE_KEY, "00 03")  # heating -> defrosting
    await device.wait_state(STATE, "Abtauen")
    await device.wait_frame(mark, SET_MODE_SUMMER)
    await device.wait_state(MODE, "Sommer")

    mark = device.mark()
    await device.hpsu_set(STATE_KEY, "00 01")  # defrosting -> heating
    await device.wait_state(STATE, "Heizen")
    await device.wait_frame(mark, SET_MODE_HEATING)
    await device.wait_state(MODE, "Heizen")


async def test_defrosting_then_hot_water_restores_mode(device):
    await start(device)
    await enable_optimization(device)

    await device.hpsu_set(STATE_KEY, "00 03")  # heating -> defrosting
    await device.wait_state(MODE, "Sommer")

    mark = device.mark()
    await device.hpsu_set(STATE_KEY, "00 04")  # defrosting -> hot water
    await device.wait_state(STATE, "Warmwasserbereitung")
    await device.wait_frame(mark, SET_MODE_HEATING)
    await device.wait_state(MODE, "Heizen")


async def test_defrosting_without_optimization_keeps_mode(device):
    await start(device)
    mark = device.mark()

    await device.hpsu_set(STATE_KEY, "00 03")  # heating -> defrosting
    await device.wait_state(STATE, "Abtauen")
    await device.wait_reads(mark, READ_MODE)

    assert SET_MODE_SUMMER not in device.frames_since(mark)
    assert device.states[MODE] == "Heizen"
