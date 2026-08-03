# Wiki gaps: tools, durability, and block breaking


The items wiki already documents max uses, material efficiency, weapon damage, armor uses, and shears/flint durability. The following behaviour was **not** on that page (or related wiki pages) and had to be taken from b1.7.3 code.

## Block break progress (`Block.blockStrength`)

Per game tick, dig progress is:

- `hardness < 0` → `0` (unbreakable)
- player **cannot** harvest → `1 / hardness / 100`
- player **can** harvest → `getCurrentPlayerStrVsBlock(block) / hardness / 30`

A block breaks when accumulated progress reaches `1.0`. If a single tick’s progress is `≥ 1.0`, the client breaks instantly on dig-start and does **not** send `DIGGING_FINISHED`.

## Harvest check (`InventoryPlayer.canHarvestBlock`)

Harvest is true if:

1. `block.material.isHarvestable` (`Material.func_31055_i`), **or**
2. the held item’s `canHarvestBlock(block)` returns true

Rock, iron, snow layer, snow block, cobweb, and piston materials are not harvestable by bare hand. That is what forces the slow `/100` path without the right tool.

## Tool strength vs block (`Item.getStrVsBlock` / tool subclasses)

| Item | Strength |
|------|----------|
| Default / bare hand | `1.0` |
| Pickaxe / axe / shovel on `blocksEffectiveAgainst` | material efficiency (`2/4/6/8/12`) |
| Pickaxe / axe / shovel otherwise | `1.0` |
| Sword vs cobweb | `15.0` |
| Sword otherwise | `1.5` |
| Shears vs cobweb or leaves | `15.0` |
| Shears vs wool | `5.0` |
| Shears otherwise | `1.0` |

Effective-against lists (b1.7.3):

- **Pickaxe:** cobble, double/single slab, stone, sandstone, mossy cobble, iron/gold/diamond/coal/lapis ore & blocks, ice, netherrack
- **Axe:** planks, bookshelf, log, chest
- **Shovel:** grass, dirt, sand, gravel, snow layer, snow block, clay, farmland

Pickaxe / shovel / sword / shears `canHarvestBlock` rules match the existing helpers in `tool_properties.cpp` (obsidian needs diamond, ores need stone/iron levels, shovel only snow, sword/shears only cobweb).

## Water / airborne penalty (`EntityPlayer.getCurrentPlayerStrVsBlock`)

Applied only on the **harvestable** (`/30`) path, by dividing strength:

- `/5` while inside water
- `/5` again while not on ground

These stack (`/25` if both). The non-harvestable (`/100`) path does **not** apply these penalties.

## Durability on block destroy (`onBlockDestroyed`)

| Item | Damage |
|------|--------|
| Pickaxe / axe / shovel | `+1` always |
| Sword | `+2` always |
| Shears | `+1` only for leaves or cobweb |
| Hoe | no mining damage (only on till use) |

Entity hit durability (for completeness; not on the wiki either): sword `+1`, pickaxe/axe/shovel `+2`.

## Mine Block packet notes (partially documented)

https://pixelbrush.dev/beta-wiki/networking/packets/014-mine-block says finished digging should be validated for time, and that dig-cancel is never sent. It does **not** document:

- instant break on dig-start when progress/tick `≥ 1`
- the hardness / harvest / efficiency formulae above
- that rejecting an early finish requires a Set Block resync because the client already removed the block locally

## Blocks wiki

https://pixelbrush.dev/beta-wiki/general/blocks still marks material types as missing (`MISSING` / TODO). Hardness values and harvestability are implemented in-repo under `block_properties` / `materials` and were cross-checked against the decompiled `Block` / `Material` tables where needed.
