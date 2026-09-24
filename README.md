# Physical Caravans for Heroes III: HotA 1.8.0

An experimental mod that adds physical troop caravans to **Heroes of Might and Magic III: Horn of the Abyss 1.8.0**. Open one of your towns and press `K` to order creatures from owned external dwellings, another town's reserve, or another town's garrison.

The first click on **Send** displays the complete price. Resources are charged only after the second, confirming click. A caravan then appears on the adventure map and travels up to eight tiles per game day toward the selected town.

## Features

- In-game, Heroes III-styled selection window; no separate desktop application.
- Explicit confirmation before any resources are spent.
- Orders from captured dwellings and from other owned towns.
- Level-one creatures from external dwellings are free, matching the native dwelling rule; the same creatures remain paid in towns.
- A single **All** button selects every available creature the player can currently afford.
- A physical wagon on the adventure map.
- Heroes, monsters and garrisons block the route until the obstacle is removed.
- Friendly players can retrieve the troops; enemies can attack the caravan.
- An emptied caravan disappears automatically after the troop-transfer window closes.
- Configurable hotkey in `Caravan.ini`.
- Works without editing maps and does not modify the original game executables.

## Compatibility

This build supports only the exact, tested **HotA 1.8.0** executables:

| File | Required SHA-256 |
| --- | --- |
| `h3hota.exe` | `B5F2F793AF0986050FB41DF7209C25D861AE0F837AF52BB3BD6864BA4DE84F41` |
| `h3hota HD.exe` | `5AAAB925F06CCCF23BB09814767590A95B84A557EB33D244800520BE4F1F18DE` |

The installer refuses to patch any other executable. Vanilla SoD/Complete, ERA/WoG, VCMI and other HotA versions are not supported. Surface-to-underground routes are not implemented. Multiplayer and hot-seat have not been verified.

## Installation

1. Download `Caravan_HotA_1.8.0.zip` from the `release` folder.
2. Extract the `Caravan_HotA_1.8.0` folder directly into the HotA game directory, beside `h3hota.exe`.
3. Run `Установить.bat`.
4. Launch `h3hota Caravan HD.exe`.

The installer creates separate launchers and leaves the original EXE files unchanged. It also copies the 32-bit Windows `version.dll` as `version_real.dll`; that system DLL is not distributed in this repository.

To uninstall, run `Удалить.bat` from the extracted folder. During this experimental stage, use a separate save file.

## Hotkey

Edit `Caravan.ini` in the game directory, for example:

```ini
[Caravan]
Hotkey=F8
```

Supported values are `A`-`Z`, `0`-`9`, `F1`-`F24`, `SPACE`, `TAB`, `HOME`, `END`, `INSERT`, `DELETE`, `UP`, `DOWN`, `LEFT`, `RIGHT`, `PAGEUP` and `PAGEDOWN`. Restart the game after changing it.

## Building

The source is a 32-bit Windows C++17 DLL. With MinGW-w64 installed:

```powershell
.\build.ps1
```

If the compiler is not on `PATH`:

```powershell
.\build.ps1 -Compiler 'C:\path\to\g++.exe'
```

The result is written to `dist\Caravan.dll`. No Heroes III or HotA files are required to compile the DLL. See [TECHNICAL.md](TECHNICAL.md) for implementation and review notes.

## Repository contents

- `src/` — DLL source and export definition.
- `installer/` — auditable installer plus the tested DLL payload.
- `release/` — ready-to-install archive and checksums.
- `TECHNICAL.md` — compatibility assumptions and areas that need additional review.

This is an unofficial community project and is not affiliated with Ubisoft, New World Computing, 3DO or the HotA team. The repository contains no game executables, maps, saves or extracted game assets.

## License

The mod's original source code and project files are available under the [MIT License](LICENSE). This license does not apply to Heroes III, Horn of the Abyss, HD Mod, or any assets and trademarks owned by their respective rights holders.
