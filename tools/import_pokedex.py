#!/usr/bin/env python3
"""Import pinned source caches; no network access and no wiki crawling.

python tools/import_pokedex.py --cache <directory-containing-source-caches>
Requires beautifulsoup4 and pypinyin; see docs/pokedex.md.
"""
import argparse
import collections
import csv
import hashlib
import json
import pathlib
import re
import unicodedata
from urllib.parse import quote

from bs4 import BeautifulSoup
from pypinyin import Style, lazy_pinyin

ROOT = pathlib.Path(__file__).resolve().parents[1]
GEN_LABELS = ['一', '二', '三', '四', '五', '六', '七', '八', '九']
TYPE_NAMES = ['', '一般', '格斗', '飞行', '毒', '地面', '岩石', '虫', '幽灵',
              '钢', '火', '水', '草', '电', '超能力', '冰', '龙', '恶', '妖精']
GROUP_NAMES = {18: '究极之日／究极之月', 20: '剑／盾', 23: '晶灿钻石／明亮珍珠',
               24: '传说 阿尔宙斯', 25: '朱／紫'}
DEFAULT_FORM = {
    412: '草木蓑衣', 413: '草木蓑衣', 487: '别种形态', 492: '陆上形态',
    585: '春天的样子', 586: '春天的样子', 641: '化身形态', 642: '化身形态',
    645: '化身形态', 666: '花园花纹', 902: '雄性的样子', 905: '化身形态',
    916: '雄性的样子', 925: '四只家庭', 931: '绿羽毛', 964: '平凡形态',
    978: '上弓姿势', 982: '二节形态', 999: '宝箱形态', 1007: '完全形态',
    1008: '完整模式', 1012: '冒牌货的样子', 1013: '凡作的样子',
    1017: '碧草面具', 1024: '普通形态',
}


def clean(text):
    return re.sub(r'\s+', '', unicodedata.normalize('NFKC', text)).strip('*')


def wiki_description(d):
    candidates = []
    for entry in d.get('pokedex_entries', []):
        gen = next((i + 1 for i, n in enumerate(GEN_LABELS) if n in entry['name']), 0)
        for v in entry.get('versions', []):
            if v['name'] == '传说 Z-A' or not v.get('text'):
                continue
            # Scarlet, then Violet; previous generation games in source order.
            rank = 100 if v['name'] == '朱' else 99 if v['name'] == '紫' else len(candidates)
            candidates.append((gen, rank, v))
    i = int(d['pokedex_id'])
    for _, _, v in sorted(candidates, key=lambda x: (x[0], x[1]), reverse=True):
        text = v['text']
        if '（' in text:
            form = DEFAULT_FORM.get(i)
            # Dialga/Palkia have unlabeled default + Origin text in one cell;
            # Ursaluna's SV entries exclusively describe Bloodmoon. Use an
            # earlier, unambiguous entry instead of guessing sentence boundaries.
            if not form:
                continue
            parts = re.findall(r'([^（）]+)（([^（）]+)）', text)
            text = next((t for t, label in parts if label == form), '')
            if not text:
                continue
        if text.strip():
            return clean(text), v['name']
    return None


def import_sources(cache):
    def rows(name):
        with (cache / 'pokeapi-snapshot' / (name + '.csv')).open(encoding='utf8', newline='') as f:
            yield from csv.DictReader(f)

    groups = {int(r['id']): r for r in rows('version_groups')}
    versions = {int(r['id']): r for r in rows('versions')}
    pokemon = {int(r['species_id']): r for r in rows('pokemon')
               if r['is_default'] == '1' and int(r['species_id']) <= 1025}
    pid_to_id = {int(p['id']): i for i, p in pokemon.items()}
    species = {int(r['id']): r for r in rows('pokemon_species')}
    wiki = {int(d['pokedex_id']): d for f in (cache / '52poke-snapshot').glob('*.json')
            for d in [json.loads(f.read_text(encoding='utf8'))]}
    old = {int(d['id']): d for d in json.loads((cache / 'nightcat.json').read_text(encoding='utf8'))}
    descriptions = {}
    for r in rows('pokemon_species_flavor_text'):
        if r['language_id'] != '12':
            continue
        i, v = int(r['species_id']), int(r['version_id'])
        order = int(groups[int(versions[v]['version_group_id'])]['order'])
        if (order, v) > descriptions.get(i, ((-1, -1),))[0]:
            descriptions[i] = ((order, v), clean(r['flavor_text']), versions[v]['identifier'])
    version_zh = {'shield': '盾', 'sword': '剑', 'ultra-sun': '究极之日',
                  'ultra-moon': '究极之月', 'sun': '太阳', 'moon': '月亮',
                  'lets-go-pikachu': "Let's Go! 皮卡丘", 'lets-go-eevee': "Let's Go! 伊布"}
    relations = collections.defaultdict(lambda: collections.defaultdict(set))
    for r in rows('pokemon_moves'):
        pid = int(r['pokemon_id'])
        if pid in pid_to_id:
            relations[int(r['version_group_id'])][pid_to_id[pid]].add(int(r['move_id']))
    order = [25] + sorted((g for g in groups if int(groups[g]['generation_id']) < 9),
                          key=lambda g: int(groups[g]['order']), reverse=True)
    selected = {i: next(g for g in order if relations[g].get(i)) for i in pokemon}
    # PokeAPI's PLA snapshot has sparse move-shop records. Supplement the
    # selected default forms from the game's personal-table permission bits.
    # Table format and the ordered 61 move identifiers are independently
    # documented by PKHeX PersonalInfo8LA (pinned in the source manifest).
    move_ids = {re.sub('[^a-z0-9]', '', r['identifier']): int(r['id']) for r in rows('moves')}
    shop_source = (cache / 'pkhex/PersonalInfo8LA.cs').read_text(encoding='utf8')
    shop_block = shop_source.split('MoveShopMoves =>')[1].split('];')[0]
    shop = [move_ids[name.lower()] for name in re.findall(r'\(int\)Move\.([A-Za-z0-9]+)', shop_block)]
    assert len(shop) == 61
    personal = (cache / 'pkhex/personal_la').read_bytes()
    shop_added = 0
    for i, group in selected.items():
        if group != 24:
            continue
        record = personal[i * 0xB0:(i + 1) * 0xB0]
        assert int.from_bytes(record[0x5C:0x5E], 'little') == i
        flags = int.from_bytes(record[0xA8:0xB0], 'little')
        before = len(relations[24][i])
        relations[24][i].update(move for bit, move in enumerate(shop) if flags & (1 << bit))
        shop_added += len(relations[24][i]) - before
    stats = collections.defaultdict(dict)
    types = collections.defaultdict(dict)
    for r in rows('pokemon_stats'):
        if int(r['pokemon_id']) in pid_to_id:
            stats[pid_to_id[int(r['pokemon_id'])]][int(r['stat_id'])] = int(r['base_stat'])
    for r in rows('pokemon_types'):
        if int(r['pokemon_id']) in pid_to_id:
            types[pid_to_id[int(r['pokemon_id'])]][int(r['slot'])] = int(r['type_id'])

    # PLA has a distinct battle system; do not apply ordinary Gen 8 values.
    soup = BeautifulSoup((cache / 'pla-attacks.html').read_bytes(), 'html.parser')
    pla = {}
    for tr in soup.select('tr'):
        cells = tr.find_all('td', recursive=False)
        if len(cells) != 7 or not cells[0].find('a'):
            continue
        raw = [c.get_text(' ', strip=True) for c in cells]
        if not raw[3].isdigit():
            continue
        name = re.sub(r'[^a-z0-9]', '', raw[0].lower())
        image = cells[1].find('img')
        type_name = pathlib.PurePosixPath(image['src']).stem.lower()
        pla[name] = {'pp': int(raw[3]), 'power': int(raw[4]) if raw[4].isdigit() else -1,
                     'accuracy': int(raw[5]) if raw[5].isdigit() and int(raw[5]) <= 100 else -1,
                     'type_slug': type_name}
    assert len(pla) > 170, len(pla)
    type_slugs = ['', 'normal', 'fighting', 'flying', 'poison', 'ground', 'rock', 'bug',
                  'ghost', 'steel', 'fire', 'water', 'grass', 'electric', 'psychic', 'ice',
                  'dragon', 'dark', 'fairy']
    names = {int(r['move_id']): r['name'] for r in rows('move_names') if r['local_language_id'] == '12'}
    moves = {int(r['id']): r for r in rows('moves')}
    changes = collections.defaultdict(list)
    for r in rows('move_changelog'):
        changes[int(r['move_id'])].append(r)
    variants = []
    variant_map = {}
    for i in sorted(pokemon):
        g = selected[i]
        for m in sorted(relations[g][i]):
            if (m, g) in variant_map:
                continue
            raw = moves[m]
            values = {f: int(raw[f]) if raw[f] else -1 for f in ('type_id', 'power', 'accuracy', 'pp')}
            for change in sorted(changes[m], key=lambda c: int(groups[int(c['changed_in_version_group_id'])]['order']), reverse=True):
                if int(groups[int(change['changed_in_version_group_id'])]['order']) > int(groups[g]['order']):
                    values.update({f: int(change[f]) for f in values if change[f]})
            if g == 24:
                key = re.sub(r'[^a-z0-9]', '', raw['identifier'])
                assert key in pla, (m, raw['identifier'])
                values.update({f: pla[key][f] for f in ('power', 'accuracy', 'pp')})
                values['type_id'] = type_slugs.index(pla[key]['type_slug'])
            variant_map[m, g] = len(variants)
            variants.append({'id': m, 'group': g, 'name': clean(names[m]), **values})
    result = []
    for i in sorted(pokemon):
        d, p, g = wiki[i], pokemon[i], selected[i]
        name = d['name_zh']
        if i in descriptions:
            _, desc, ver = descriptions[i]
            ver = version_zh[ver]
            source = 'PokeAPI'
        else:
            match = wiki_description(d)
            if match:
                desc, ver = match
                source = '52poke snapshot'
            else:
                desc, ver, source = clean(old[i]['desc']), '版本未标注', 'NightCatSama/pokedex'
        # In these species names, this character means "long", never "grow".
        # Dictionary segmentation otherwise yields incorrect ZWGS/ZET aliases.
        initials = ''.join(lazy_pinyin(name.replace('长', '常'), style=Style.FIRST_LETTER)).upper()
        initials = re.sub('[^A-Z0-9]', '', unicodedata.normalize('NFKC', initials))
        result.append({'id': i, 'name': name, 'initials': initials,
                       'gen': int(species[i]['generation_id']),
                       'types': [types[i][s] for s in sorted(types[i])],
                       'height': int(p['height']), 'weight': int(p['weight']),
                       'stats': [stats[i][s] for s in range(1, 7)],
                       'description': desc, 'description_version': ver,
                       'description_source': source,
                       'wiki_url': 'https://wiki.52poke.com/wiki/' + quote(name),
                       'move_gen': int(groups[g]['generation_id']), 'move_group': g,
                       'move_indices': [variant_map[m, g] for m in sorted(relations[g][i])]})
    assert len(result) == 1025 and all(d['description'] and d['initials'] for d in result)
    return {'schema': 1, 'source_commits': {
        'pokeapi': (cache / 'pokeapi-snapshot/commit.txt').read_text().strip(),
        '52poke_export': (cache / '52poke-snapshot/commit.txt').read_text().strip(),
        'pkhex_pla_shop': (cache / 'pkhex/commit.txt').read_text().strip()},
        'source_hashes': {name: hashlib.sha256((cache / name).read_bytes()).hexdigest()
                          for name in ('pla-attacks.html', 'nightcat.json')},
        'pla_source': 'https://www.serebii.net/legendsarceus/attacks.shtml',
        'pla_shop_relations_added': shop_added,
        'types': TYPE_NAMES, 'groups': GROUP_NAMES, 'pokemon': result, 'moves': variants}


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--cache', type=pathlib.Path, required=True)
    args = parser.parse_args()
    data = import_sources(args.cache)
    dest = ROOT / 'assets/pokedex/catalog.json'
    dest.write_text(json.dumps(data, ensure_ascii=False, indent=2) + '\n', encoding='utf8')
    print('Species:', len(data['pokemon']), 'Move variants:', len(data['moves']))
    print('Descriptions:', dict(collections.Counter(p['description_source'] for p in data['pokemon'])))
    print('Generations:', dict(collections.Counter(p['move_gen'] for p in data['pokemon'])))
