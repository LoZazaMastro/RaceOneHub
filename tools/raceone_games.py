"""Game identities and output layout. Contains no game content."""
from dataclasses import dataclass


@dataclass(frozen=True)
class Game:
    id: str
    name: str
    package: str
    version: str
    title_id: str
    executable: str
    runtime: str
    sha256: str
    scenes: tuple[str, ...]
    sdk_libraries: tuple[str, ...]


GAMES = {
    'fm5': Game('fm5', 'Forza Motorsport 5', 'Forza', '1.0.0.20', '7b35d917',
        'RaceOne5.exe', 'runtime/pc',
        '94dad21851d335e9a33231dc2a1de2ecd4b93eefdf5f6772085bf6886bf47db7',
        ('FrontEnd.xml', 'MiniAutoVista.xml', 'PhotoMode.xml', 'PostRace.xml', 'PreRace_321.xml', 'Replay.xml'),
        ('xg.dll', 'xg_x.dll')),

}


def game_definition(game_id='fm5'):
    try:
        return GAMES[game_id]
    except (KeyError, TypeError):
        raise ValueError('Select Forza Motorsport 5.') from None
