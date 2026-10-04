#!/usr/bin/env python3
"""Reject merged writes or layouts that would damage preserved device regions."""
import sys
import unittest
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from verify_pokedex_layout import check_preserved_regions
from test_verify_firmware import sample_table

ENTRIES = ((0, 0, 0x10000, 0x300000, 'factory'),
           (1, 2, 0x356000, 0x4000, 'cardid'),
           (0, 32, 0x700000, 0x100000, 'recovery'))
HOOK = b'UP held: booting permanent recovery'


class PreservationTest(unittest.TestCase):
    def test_valid_boundary(self):
        check_preserved_regions(sample_table(ENTRIES), 0x356000, HOOK)

    def test_rejects_oversize_write(self):
        with self.assertRaises(ValueError):
            check_preserved_regions(sample_table(ENTRIES), 0x356001, HOOK)

    def test_rejects_missing_hook(self):
        with self.assertRaises(ValueError):
            check_preserved_regions(sample_table(ENTRIES), 0x200000, b'')

    def test_rejects_moved_or_missing_regions(self):
        for i in range(3):
            with self.subTest(region=i):
                changed = list(ENTRIES)
                k, s, offset, size, label = changed[i]
                changed[i] = (k, s, offset + 0x1000, size, label)
                with self.assertRaises(ValueError):
                    check_preserved_regions(sample_table(tuple(changed)), 0x200000, HOOK)
                with self.assertRaises(ValueError):
                    check_preserved_regions(sample_table(ENTRIES[:i] + ENTRIES[i + 1:]), 0x200000, HOOK)


if __name__ == '__main__':
    unittest.main()
