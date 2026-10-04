#!/usr/bin/env python3
"""Application-specific preservation checks, in addition to generic image checks."""
import sys
from pathlib import Path
from verify_firmware import parse_partition_table


def check_preserved_regions(table: bytes, merged_size: int, bootloader: bytes) -> None:
    partitions, _ = parse_partition_table(table)
    by_label = {p.label: p for p in partitions}
    expected = {'factory': (0x10000, 0x300000), 'cardid': (0x356000, 0x4000),
                'recovery': (0x700000, 0x100000)}
    for label, bounds in expected.items():
        p = by_label.get(label)
        if p is None or (p.offset, p.size) != bounds:
            raise ValueError(f'Pokédex requires preserved {label} bounds {bounds}')
    if merged_size > 0x356000:
        raise ValueError('Merged write would reach device identity or Recovery')
    if b'UP held: booting permanent recovery' not in bootloader:
        raise ValueError('Recovery hook is missing from the linked bootloader')


if __name__ == '__main__':
    build = Path(sys.argv[1])
    check_preserved_regions((build / 'partition_table/partition-table.bin').read_bytes(),
                            (build / 'FoloToy-AI-Passport-full.bin').stat().st_size,
                            (build / 'bootloader/bootloader.bin').read_bytes())
    print('Pokédex identity/Recovery bounds and linked UP hook: PASS')
