# Changelog

## v1.1.2 — 2026-09-26

- Fixed a race condition that could crash HotA shortly after an empty caravan was removed.
- Caravan movement and empty-wagon cleanup now run on the game's main message thread instead of the mod's background polling thread.
- Caravans now prefer a longer clear detour around heroes, monsters and garrisons. They wait only when no clear route exists.

## v1.1.1 — 2026-09-25

- Fixed level-one creatures from external dwellings being charged at their normal town price.
- The **All** button now treats those dwelling recruits as free and selects the full available stack.

## v1.1.0 — 2026-09-25

- Added an **All** button that selects the maximum available and currently affordable quantity.
- Empty caravans now disappear automatically shortly after all troops are collected.
- Preserved the two-click price confirmation: selecting all creatures does not spend resources by itself.
