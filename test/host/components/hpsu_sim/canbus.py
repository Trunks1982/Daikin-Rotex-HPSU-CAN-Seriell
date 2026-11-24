"""canbus platform `hpsu_sim`: a simulated HPSU for host tests.

The simulator answers read requests from a register table, applies set requests and
drops values outside a register's limits (as the HPSU does). Every frame is logged with
the tag `hpsu_sim`, so tests can check the exact bytes on the bus.

Register keys are the addressing bytes of a request: "FA XX YY" for extended
(0xFA) registers, otherwise the single byte at position 2 (e.g. "61").
`data` holds the bytes after the key: 2 bytes for "FA XX YY", 4 bytes otherwise.
"""

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import canbus
from esphome.const import CONF_ID

DEPENDENCIES = ["host"]

CONF_REGISTERS = "registers"
CONF_KEY = "key"
CONF_DATA = "data"
CONF_MIN = "min"
CONF_MAX = "max"
CONF_SIGNED = "signed"

hpsu_sim_ns = cg.esphome_ns.namespace("hpsu_sim")
HpsuSim = hpsu_sim_ns.class_("HpsuSim", canbus.CanbusComponent)


def hex_bytes(value):
    value = cv.string_strict(value)
    try:
        result = [int(token, 16) for token in value.replace("0x", "").split()]
    except ValueError as err:
        raise cv.Invalid(f"invalid hex bytes: {value}") from err
    if not result or any(b < 0 or b > 0xFF for b in result):
        raise cv.Invalid(f"invalid hex bytes: {value}")
    return result


def validate_register(config):
    key = config[CONF_KEY]
    extended = key[0] == 0xFA
    if extended and len(key) != 3:
        raise cv.Invalid("an extended key needs 3 bytes: FA XX YY")
    if not extended and len(key) != 1:
        raise cv.Invalid("a non-extended key is one byte")
    expected = 2 if extended else 4
    if len(config[CONF_DATA]) != expected:
        raise cv.Invalid(f"data needs {expected} bytes for this key")
    if (CONF_MIN in config) != (CONF_MAX in config):
        raise cv.Invalid("set min and max together")
    if CONF_MIN in config and not extended:
        raise cv.Invalid("limits are only supported for extended registers")
    return config


REGISTER_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.Required(CONF_KEY): hex_bytes,
            cv.Required(CONF_DATA): hex_bytes,
            cv.Optional(CONF_MIN): cv.int_,
            cv.Optional(CONF_MAX): cv.int_,
            cv.Optional(CONF_SIGNED, default=False): cv.boolean,
        }
    ),
    validate_register,
)

CONFIG_SCHEMA = canbus.CANBUS_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(HpsuSim),
        cv.Optional(CONF_REGISTERS, default=[]): cv.ensure_list(REGISTER_SCHEMA),
    }
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    try:
        await canbus.register_canbus(var, config, reports_status=False)
    except TypeError:  # ESPHome before reports_status
        await canbus.register_canbus(var, config)

    for reg in config[CONF_REGISTERS]:
        limited = CONF_MIN in reg
        cg.add(
            var.add_register(
                reg[CONF_KEY],
                reg[CONF_DATA],
                limited,
                reg.get(CONF_MIN, 0),
                reg.get(CONF_MAX, 0),
                reg[CONF_SIGNED],
            )
        )
