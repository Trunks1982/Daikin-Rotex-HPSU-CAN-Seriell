"""
    Manifest for external consumers (e.g. Sunflow).

    The manifest maps the stable keys of this component (e.g. "tv", "max_target_flow_temp") to the
    entities Home Assistant sees: their domain and the name the user gave them in the YAML. Names and
    option texts depend on the user and the language; the keys and raw option values do not.

    The manifest is built completely at compile time and served by a native API action with a
    response ("rotex_can_manifest", integer argument "page"), which the component registers itself:
    no change to the user's YAML is needed. Large manifests are split into pages; every page repeats
    the header and states "page" and "pages", so a client reads page 0 and, if "pages" > 1, the rest.
    It is only enabled when the YAML has an "api:" section and ESPHome supports action responses for
    components (2026.1.0 or newer).
"""

import json
import logging

import esphome.codegen as cg
from esphome.const import (
    CONF_INTERNAL,
    CONF_NAME,
    CONF_UNIT_OF_MEASUREMENT,
    __version__ as ESPHOME_VERSION,
)
from esphome.core import CORE

from .translations.translate import delayed_translate_tag, translate

_LOGGER = logging.getLogger(__name__)

# Identifies this component; never changed. There is deliberately no version: a consumer checks the
# keys, domains, units and option values it uses each time it reads the manifest.
MANIFEST_ID = "rotex_hpsu_can"
MANIFEST_NAME = "Rotex/Daikin HPSU – ESPHome CAN"
ACTION_NAME = "rotex_can_manifest"

# First ESPHome version whose API lets a component register an action with a response
# (UserServiceDescriptor with call id, StringRef name/error message).
MIN_ESPHOME_VERSION = (2026, 1, 0)

# Upper bound of one page (the ESP accepts native API messages up to 32 KiB, an encrypted frame
# carries up to ~64 KiB). Pages keep every response far below both, however much the component grows.
PAGE_BYTES = 16 * 1024

CONF_DEVICE_ID = "device_id"


def _version_tuple(version: str) -> tuple[int, ...]:
    parts = []
    for part in version.split(".")[:3]:
        digits = "".join(c for c in part if c.isdigit())
        parts.append(int(digits) if digits else 0)
    return tuple(parts)


def option_values(mapping: dict) -> list[list]:
    """Options of a map as [value, text] or [value, text, translation key].

    value is the raw/semantic value of the component (language independent), text what the entity
    shows (depends on language and user), the translation key is present for translated texts.
    """
    values = []
    for value, text in mapping.items():
        key = None
        if isinstance(text, str) and text.startswith(delayed_translate_tag):
            key = text[len(delayed_translate_tag):]
            text = translate(key)
        if isinstance(value, float) and value.is_integer():
            value = int(value)
        values.append([value, text, key] if key else [value, text])
    return values


class ManifestBuilder:
    """Collects the entities of the component while code is generated."""

    def __init__(self) -> None:
        self.entities: list[dict] = []
        self.language: str | None = None
        self.git: str | None = None

    def add(self, key: str, domain: str, conf: dict, unit=None, values=None,
            min_value=None, max_value=None, step=None) -> None:
        entry = {"key": key, "domain": domain, "name": conf.get(CONF_NAME)}
        unit = conf.get(CONF_UNIT_OF_MEASUREMENT, unit)
        if unit:
            entry["unit"] = unit
        if min_value is not None:
            entry["min"] = min_value
        if max_value is not None:
            entry["max"] = max_value
        if step is not None:
            entry["step"] = step
        if values:
            entry["values"] = values
        if conf.get(CONF_INTERNAL):
            entry["internal"] = True
        if device_id := conf.get(CONF_DEVICE_ID):
            entry["device"] = str(getattr(device_id, "id", device_id))
        self.entities.append(entry)

    def _header(self) -> dict:
        return {
            "id": MANIFEST_ID,
            "name": MANIFEST_NAME,
            "git": self.git,
            "language": self.language,
        }

    def to_pages(self) -> list[str]:
        """JSON per page, each at most PAGE_BYTES (a single entity larger than that gets its own page)."""

        def dump(obj) -> str:
            return json.dumps(obj, ensure_ascii=False, separators=(",", ":"))

        # Room for "page"/"pages" with up to 3 digits each
        fixed = len(dump({**self._header(), "page": 999, "pages": 999, "entities": []}).encode("utf-8"))
        chunks: list[list[dict]] = [[]]
        size = fixed
        for entity in self.entities:
            entity_size = len(dump(entity).encode("utf-8")) + 1  # + separating comma
            if chunks[-1] and size + entity_size > PAGE_BYTES:
                chunks.append([])
                size = fixed
            chunks[-1].append(entity)
            size += entity_size

        pages = len(chunks)
        return [dump({**self._header(), "page": i, "pages": pages, "entities": chunk})
                for i, chunk in enumerate(chunks)]


def manifest_supported() -> bool:
    """The YAML has an API and ESPHome can serve the manifest."""
    if "api" not in CORE.config:
        _LOGGER.info("[Manifest] No 'api:' configured, manifest action disabled")
        return False
    if _version_tuple(ESPHOME_VERSION) < MIN_ESPHOME_VERSION:
        _LOGGER.warning(
            "[Manifest] ESPHome %s is too old for the manifest action (needs %s), disabled",
            ESPHOME_VERSION, ".".join(map(str, MIN_ESPHOME_VERSION)),
        )
        return False
    return True


def register_manifest(var, builder: ManifestBuilder) -> None:
    """Enable the API parts the manifest action needs and hand the manifest to the component."""
    if not manifest_supported():
        return

    cg.add_define("USE_API_USER_DEFINED_ACTIONS")
    cg.add_define("USE_API_CUSTOM_SERVICES")
    cg.add_define("USE_API_USER_DEFINED_ACTION_RESPONSES")
    cg.add_define("USE_API_USER_DEFINED_ACTION_RESPONSES_JSON")
    cg.add_define("USE_DAIKIN_ROTEX_CAN_MANIFEST")

    pages = builder.to_pages()
    sizes = [len(page.encode("utf-8")) for page in pages]
    _LOGGER.info(
        "[Manifest] %d entities, %d bytes in %d page(s) (largest %d bytes), action '%s'",
        len(builder.entities), sum(sizes), len(pages), max(sizes), ACTION_NAME,
    )

    cg.add(var.set_manifest_action_name(ACTION_NAME))
    for page in pages:
        cg.add(var.add_manifest_page(page))
