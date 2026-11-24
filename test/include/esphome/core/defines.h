#pragma once

// Minimal replacement for the generated defines.h, used by the GoogleTest build only.
// ESPHome's own esphome/core/defines.h is a developer file that enables every feature
// (JSON, all platforms, ...) and does not compile without those libraries.

#define USE_HOST
#define ESPHOME_LOG_LEVEL ESPHOME_LOG_LEVEL_NONE
