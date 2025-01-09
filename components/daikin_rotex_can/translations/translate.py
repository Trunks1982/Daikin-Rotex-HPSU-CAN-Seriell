"""
    This module provides translation functionality for the Daikin Rotex CAN component.
"""

from .de import translations_de
from .en import translations_en
from .it import translations_it

import logging
import os

CONF_LANGUAGE = 'language'

SUPPORTED_LANGUAGES = {
    'de': 'Deutsch',
    'en': 'English',
    'it': 'Italiano',
}

translations = {
    "de": translations_de,
    "en": translations_en,
    "it": translations_it,
    # Add more languages here
}

class TranslationIntegrityError(Exception):
    """Exception raised when the translation dictionary is inconsistent."""
    def __init__(self, report):
        super().__init__(f"Translation Dictionary integrity error:\n{report}")

_LOGGER = logging.getLogger(__name__) 

# Current language
current_language = "de"
delayed_translate_tag = "DELAYED_TRANSLATE:"

def check_translations_integrity():
    """
    Checks that all translations have the same keys based on a unified abstract dictionary.

    """
    all_keys = set()
    for lang_keys in translations.values():
        all_keys.update(lang_keys.keys())
    
    discrepancies = {}
    for lang, lang_keys in translations.items():
        missing_keys = all_keys - set(lang_keys.keys())
        if missing_keys:
            discrepancies[lang] = missing_keys
    
    if discrepancies:
        report_lines = ["Discrepancies found in translations dictionary (missing keys):"]
        for lang, missing_keys in discrepancies.items():
            report_lines.append(f"  - {lang}: {', '.join(missing_keys)}")
        report = "\n".join(report_lines)
        raise TranslationIntegrityError(report)
    
    _LOGGER.info("[Translate] All translations are consistent!")

def set_language(lang):
    global current_language
    if lang in translations:
        _LOGGER.info("[Translate] Setting language to '%s'", lang)
        current_language = lang
    else:
        raise ValueError(f"[Translate] Language {lang} not found in dictionary", lang)

def delayed_translate(key: str) -> str:
    return delayed_translate_tag + key

def translate(key: str) -> str:

    global current_language
    lang_translations = translations.get(current_language, translations.get("en", {}))

    if key in lang_translations:
        translated = lang_translations[key]
        _LOGGER.info("[Translate] Key '%s' found in language '%s' -> '%s'",key, current_language, translated)
        return translated

    if "en" in translations and key in translations["en"]:
        _LOGGER.warning(
            "[Translate] Key '%s' not found in language '%s'. Falling back to English.", 
            key, current_language
        )
        return translations["en"][key]
    _LOGGER.error(
        "[Translate] Key '%s' not found in language '%s' or in fallback language 'en'. Returning error message.", 
        key, current_language
    )
    return f"ERROR: Key '{key}' not found"

def apply_delayed_translate(key: str) -> str:
    if isinstance(key, str) and key.startswith(delayed_translate_tag):
        stripped_key = key[len(delayed_translate_tag):]
        return translate(stripped_key)
    return key

def apply_translation_to_mapping(mapping: dict) -> dict:
    return {key: apply_delayed_translate(value) for key, value in mapping.items()}

# Generate translation.cpp, creating translation dictionary from python one
def generate_cpp_translations_for_language(translations, selected_language, keys_to_include=None):
    cpp_code = '#include "esphome/components/daikin_rotex_can/translations.h"\n'
    cpp_code += '#include "esphome/core/log.h"\n\n'
    cpp_code += '#include <string>\n'
    cpp_code += '#include <map>\n\n'
    cpp_code += 'namespace esphome {\nnamespace daikin_rotex_can {\n\n'

    # Check selected language
    if selected_language not in translations:
        raise ValueError(f"Selected language '{selected_language}' not found in translations dictionary.")

    _LOGGER.info(f"Building cpp translate dictionary for language: {selected_language}")

    selected_translations = translations[selected_language]
    if keys_to_include:
        selected_translations = {key: value for key, value in selected_translations.items() if key in keys_to_include}

    # Generate map
    cpp_code += 'static const std::map<std::string, std::string> translations = {\n'
    for key, value in selected_translations.items():
        cpp_code += f'    {{"{key}", "{value}"}},\n'
    cpp_code += '};\n\n'

    # Translation function
    cpp_code += (
        'std::string translate(const std::string &key) {\n'
        '    auto it = translations.find(key);\n'
        '    if (it != translations.end()) {\n'
        '        ESP_LOGD("translate", "Key \'%s\' translated -> \'%s\'", key.c_str(), it->second.c_str());\n'
        '        return it->second;\n'
        '    }\n'
        '    ESP_LOGW("TRANSLATE", "Key \'%s\' not found", key.c_str());\n'
        '    return "ERROR: Key \'" + key + "\' not found";\n'
        '}\n\n'
    )

    cpp_code += '}  // namespace daikin_rotex_can\n'
    cpp_code += '}  // namespace esphome\n'
    return cpp_code

# Write translations.cpp file
def write_cpp_file(output_dir):
    global current_language
    _LOGGER.info("Writing cpp translate file")
    cpp_code = generate_cpp_translations_for_language(translations, current_language)
    output_path = os.path.join(output_dir, "translations.cpp")
    with open(output_path, "w", encoding="utf-8") as f:
        f.write(cpp_code)
    _LOGGER.info(f"Generated {output_path}")  