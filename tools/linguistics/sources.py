"""Pinned, hash-checked sources. No repository edits or DB connections."""
import hashlib
import json
import os
from pathlib import Path
import tarfile
import tempfile
from urllib.request import urlopen

HERE = Path(__file__).resolve().parent
DEFAULT_CACHE = HERE.parents[1] / 'sources/data/linguistics'


def sha256(data):
    return hashlib.sha256(data).hexdigest()


class Sources:
    def __init__(self, cache=DEFAULT_CACHE):
        self.cache = Path(cache)
        self.lock = json.loads((HERE / 'sources.lock.json').read_text())

    def path(self, name):
        path = self.cache / name
        if not path.is_file():
            raise ValueError(f'Missing pinned source {name}; run fetch first')
        if sha256(path.read_bytes()) != self.lock['files'][name]['sha256']:
            raise ValueError(f'SHA256 mismatch for {name}; refusing source')
        return path

    def fetch(self):
        self.cache.mkdir(parents=True, exist_ok=True)
        for name, spec in self.lock['files'].items():
            target = self.cache / name
            target.parent.mkdir(parents=True, exist_ok=True)
            if target.exists():
                self.path(name)  # A corrupt cache fails, rather than changing the pin.
                continue
            with tempfile.NamedTemporaryFile(dir=self.cache, delete=False) as out:
                tmp = Path(out.name)
                try:
                    digest = hashlib.sha256()
                    with urlopen(spec['url'], timeout=60) as src:
                        while block := src.read(1024 * 1024):
                            digest.update(block)
                            out.write(block)
                    out.flush()
                    if digest.hexdigest() != spec['sha256']:
                        raise ValueError(f'SHA256 mismatch downloading {name}')
                    os.replace(tmp, target)
                finally:
                    tmp.unlink(missing_ok=True)

    def cldr_files(self):
        # Read only the LDML locale files and main inheritance metadata. Do not
        # extract paths or execute anything from the archive. ~93 MB at this pin.
        files = {}
        prefix = 'cldr-' + self.lock['cldr_commit'] + '/'
        with tarfile.open(self.path('cldr.tar.gz'), 'r:gz') as archive:
            for item in archive:
                if not item.isfile() or not item.name.startswith(prefix):
                    continue
                name = item.name[len(prefix):]
                if ((name.startswith('common/main/') and name.endswith('.xml'))
                        or name == 'common/supplemental/supplementalData.xml'):
                    files[name] = archive.extractfile(item).read()
        if 'common/main/root.xml' not in files:
            raise ValueError('Pinned CLDR archive has no root inventory')
        return files

    def provenance(self):
        return self.lock
