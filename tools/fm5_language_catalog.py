"""Locale labels and payload names; contains no game content."""
from pathlib import Path

# Native FM5 localization prefixes, not the generic Windows language inventory.
TEXT_LANGUAGES = {
    'en-US': ('English (US)', 'EN'),
    'en-GB': ('English (UK)', 'GB'),
    'zh-TW': ('Chinese (Traditional)', 'CHT'),
    'cs-CZ': ('Czech', 'CZ'),
    'de-DE': ('German', 'DE'),
    'es-ES': ('Spanish (Spain)', 'ES'),
    'es-MX': ('Spanish (Mexico)', 'MX'),
    'fr-FR': ('French', 'FR'),
    'hu-HU': ('Hungarian', 'HU'),
    'it-IT': ('Italian', 'IT'),
    'ja-JP': ('Japanese', 'JP'),
    'ko-KR': ('Korean', 'KO'),
    'nl-NL': ('Dutch', 'NL'),
    'pl-PL': ('Polish', 'PL'),
    'pt-BR': ('Portuguese (Brazil)', 'BR'),
    'ru-RU': ('Russian', 'RU'),
    'zh-CN': ('Chinese (Simplified)', 'CHS'),
    'ar-SA': ('Arabic', 'AR'),
    'he-IL': ('Hebrew', 'HE'),
}


def text_labels(game_folder):
    directory = Path(game_folder) / 'Mount/media/stringtables'
    names = {p.name.casefold() for p in directory.glob('*.zip')} if directory.is_dir() else set()
    return {
        locale: label for locale, (label, prefix) in TEXT_LANGUAGES.items()
        if (prefix + '.zip').casefold() in names
    }
