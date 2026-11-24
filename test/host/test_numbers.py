"""Numbers set from Home Assistant."""

import pytest

pytestmark = pytest.mark.asyncio

COOLING_ADJ = "Kühlen Sollwert Korrektur"
COOLING_ADJ_KEY = "FA 13 59"


@pytest.mark.parametrize("value, data, shown", [
    (-2.5, "FF E7", "-2.5"),   # issue #151: negative values were not accepted
    (-5.0, "FF CE", "-5.0"),
    (3.0, "00 1E", "3.0"),
])
async def test_signed_number_is_set(device, value, data, shown):
    await device.wait_state(COOLING_ADJ, "0.0")
    mark = device.mark()

    device.number(COOLING_ADJ, value)

    await device.wait_frame(mark, f"30 00 {COOLING_ADJ_KEY} {data}")
    await device.wait_for(
        lambda: f"set {COOLING_ADJ_KEY} = {data} from esp: accepted" in device.bus[mark:],
        f"HPSU accepts {data}",
    )
    await device.wait_state(COOLING_ADJ, shown)
