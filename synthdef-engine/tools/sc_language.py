"""Compile against stock SC classes plus pinned MI and FM1 fixed-model classes."""
import json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def language_command(executable,script,directory):
    executable=Path(executable)
    standard=executable.parent.parent/'Resources/SCClassLibrary'
    if not standard.is_dir():raise RuntimeError('Expected the macOS SuperCollider application class library: '+str(standard))
    config=Path(directory)/'sclang-library.yaml'
    paths=[standard,ROOT/'reference/mi-ugens/sc/Classes',ROOT/'sc/Classes']
    config.write_text('includePaths:\n'+''.join('  - '+json.dumps(str(p))+'\n' for p in paths)+'excludePaths: []\nexcludeDefaultPaths: true\npostInlineWarnings: false\n')
    return [str(executable),'-l',str(config),'-D',str(script)]
