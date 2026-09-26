#define UNICODE
#define _UNICODE
#include <windows.h>
#include <commctrl.h>
#include <stdint.h>
#include <stdio.h>
#include <vector>
#include <string>
#include <algorithm>
#include <queue>
#include <cstring>
#include <cstdlib>
#include <cwctype>

extern "C" IMAGE_DOS_HEADER __ImageBase;

namespace {

constexpr uintptr_t kImageBase = 0x400000;
constexpr uintptr_t kGamePointerVa = 0x699538;
constexpr uintptr_t kTownManagerPointerVa = 0x69954C;
constexpr uintptr_t kCreatureTraitsPointerVa = 0x6747B0;
constexpr uintptr_t kAdventureManagerPointerVa = 0x6992B8;
constexpr size_t kCreatureTraitsSize = 0x74;
constexpr size_t kCreatureTownOffset = 0x00;
constexpr size_t kCreatureLevelOffset = 0x04;
constexpr size_t kCreatureNameOffset = 0x18;
constexpr size_t kCreatureCostOffset = 0x20;
constexpr size_t kCreatureGrowthOffset = 0x44;
constexpr size_t kCreatureHealthOffset = 0x4C;
constexpr size_t kCreatureSpeedOffset = 0x50;
constexpr size_t kCreatureAttackOffset = 0x54;
constexpr size_t kCreatureDefenseOffset = 0x58;
constexpr size_t kCreatureDamageLowOffset = 0x5C;
constexpr size_t kCreatureDamageHighOffset = 0x60;
constexpr size_t kGamePlayersOffset = 0x20AD0;
constexpr size_t kGameTownPoolOffset = 0x21610;
constexpr size_t kGameGeneratorPoolOffset = 0x4E398;
constexpr size_t kGameGarrisonPoolOffset = 0x4E3A8;
constexpr size_t kGameDateOffset = 0x1F63E;
constexpr size_t kWorldMapOffset = 0x1FB70;
constexpr size_t kWorldMapCellsOffset = 0xD0;
constexpr size_t kWorldMapSizeOffset = 0xD4;
constexpr size_t kWorldMapTwoLevelsOffset = 0xD8;
constexpr size_t kMapCellSize = 0x26;
constexpr size_t kPlayerSize = 0x168;
constexpr size_t kPlayerResourcesOffset = 0x9C;
constexpr size_t kTownSize = 0x168;
constexpr size_t kTownOwnerOffset = 0x01;
constexpr size_t kTownTypeOffset = 0x04;
constexpr size_t kTownXOffset = 0x05;
constexpr size_t kTownYOffset = 0x06;
constexpr size_t kTownZOffset = 0x07;
constexpr size_t kTownPopulationOffset = 0x16;
constexpr size_t kTownArmyOffset = 0xE0;
constexpr size_t kGeneratorSize = 0x5C;
constexpr size_t kGeneratorTypesOffset = 0x04;
constexpr size_t kGeneratorPopulationOffset = 0x14;
constexpr size_t kGeneratorXOffset = 0x54;
constexpr size_t kGeneratorYOffset = 0x55;
constexpr size_t kGeneratorZOffset = 0x56;
constexpr size_t kGeneratorOwnerOffset = 0x57;

constexpr int kObjectGarrison = 33;
constexpr int kObjectHero = 34;
constexpr int kObjectMonster = 54;
constexpr int kObjectWagon = 105;
constexpr int kObjectGarrisonHorizontal = 219;
constexpr int kObjectNone = -1;
constexpr int kCaravanTilesPerDay = 8;

constexpr int kIdSources = 1001;
constexpr int kIdList = 1002;
constexpr int kIdQuantity = 1003;
constexpr int kIdSpin = 1004;
constexpr int kIdSend = 1005;
constexpr int kIdClose = 1006;
constexpr int kIdPrice = 1007;
constexpr UINT_PTR kOverlayTimer = 1008;
constexpr int kIdTitle = 1009;
constexpr int kIdFilterAll = 1010;
constexpr int kIdFilterDwellings = 1011;
constexpr int kIdFilterReserves = 1012;
constexpr int kIdFilterGarrisons = 1013;
constexpr int kIdBuyAll = 1014;

enum class SourceKind { Dwelling, TownPool, TownGarrison };

// Heroes III vectors contain a four-byte allocator field before the three
// pointers.  Treating the allocator as begin() makes small values such as 2
// look like an address and was the cause of the first K-key crash.
struct ExeVector { void* allocator; uint8_t* begin; uint8_t* end; uint8_t* capacity; };
struct Army { int32_t type[7]; int32_t count[7]; };
#pragma pack(push, 1)
struct PhysicalGarrison {
    int8_t owner;
    uint8_t markerA;
    uint8_t markerB;
    uint8_t destinationTownId;
    Army army;
    uint8_t removable;
    uint8_t x;
    uint8_t y;
    uint8_t z;
};
#pragma pack(pop)
static_assert(sizeof(PhysicalGarrison) == 0x40, "Physical garrison layout mismatch");

struct CaravanRow {
    SourceKind kind;
    uint8_t* source;
    void* amount;
    bool amount16;
    int creature;
    int x;
    int y;
    int z;
    std::wstring sourceText;
    std::wstring creatureText;
};

uintptr_t g_moduleBase = 0;
uint8_t* g_destinationTown = nullptr;
uint8_t* g_lastMapCells = nullptr;
uint32_t g_lastGameDay = UINT32_MAX;
HWND g_dialog = nullptr;
HWND g_list = nullptr;
HWND g_filter = nullptr;
HWND g_filterButtons[4]{};
HWND g_quantity = nullptr;
HWND g_price = nullptr;
HWND g_send = nullptr;
HWND g_gameWindow = nullptr;
HWND g_title = nullptr;
int g_pendingRow = -1;
int g_pendingQuantity = 0;
int g_filterSelection = 0;
int g_hotkey = 'K';
std::wstring g_hotkeyName = L"K";
HFONT g_gameFont = nullptr;
HFONT g_titleFont = nullptr;
HFONT g_smallFont = nullptr;
HFONT g_cardTitleFont = nullptr;
HBITMAP g_dialogTexture = nullptr;
HBITMAP g_buttonTexture = nullptr;
HBRUSH g_parchmentBrush = nullptr;
HBRUSH g_labelBrush = nullptr;
HBRUSH g_titleBrush = nullptr;
HIMAGELIST g_cardImages = nullptr;
std::vector<CaravanRow> g_rows;
#ifdef CARAVAN_PHYSICS
std::vector<uint32_t> g_emptyCaravanSince;
HHOOK g_gameThreadHook = nullptr;
HWND g_physicalGameWindow = nullptr;
UINT g_physicalTickMessage = 0;
volatile LONG g_physicalTickPending = 0;
#endif

static void Log(const char* message) {
    FILE* file = fopen("Caravan.log", "a");
    if (!file) return;
    SYSTEMTIME time{};
    GetLocalTime(&time);
    fprintf(file, "%02u:%02u:%02u %s\n", time.wHour, time.wMinute, time.wSecond, message);
    fclose(file);
}

static std::wstring GamePath(const wchar_t* relative) {
    wchar_t executable[MAX_PATH]{};
    const DWORD length = GetModuleFileNameW(nullptr, executable, MAX_PATH);
    std::wstring path(executable, length);
    const size_t slash = path.find_last_of(L"\\/");
    if (slash != std::wstring::npos) path.resize(slash + 1);
    path += relative;
    return path;
}

static int ParseHotkeyName(std::wstring value) {
    value.erase(std::remove_if(value.begin(), value.end(), [](wchar_t ch) { return ch == L' ' || ch == L'\t'; }), value.end());
    std::transform(value.begin(), value.end(), value.begin(), [](wchar_t ch) { return static_cast<wchar_t>(towupper(ch)); });
    if (value.size() == 1 && ((value[0] >= L'A' && value[0] <= L'Z') || (value[0] >= L'0' && value[0] <= L'9')))
        return static_cast<int>(value[0]);
    if (value.size() >= 2 && value[0] == L'F') {
        const int number = _wtoi(value.c_str() + 1);
        if (number >= 1 && number <= 24) return VK_F1 + number - 1;
    }
    struct NamedKey { const wchar_t* name; int code; };
    const NamedKey named[] = {
        {L"SPACE", VK_SPACE}, {L"TAB", VK_TAB}, {L"HOME", VK_HOME}, {L"END", VK_END},
        {L"INSERT", VK_INSERT}, {L"DELETE", VK_DELETE}, {L"UP", VK_UP}, {L"DOWN", VK_DOWN},
        {L"LEFT", VK_LEFT}, {L"RIGHT", VK_RIGHT}, {L"PAGEUP", VK_PRIOR}, {L"PAGEDOWN", VK_NEXT}
    };
    for (const auto& key : named) if (value == key.name) return key.code;
    return 0;
}

static void LoadHotkey() {
    wchar_t value[64]{};
    const std::wstring path = GamePath(L"Caravan.ini");
    GetPrivateProfileStringW(L"Caravan", L"Hotkey", L"K", value, 64, path.c_str());
    const int parsed = ParseHotkeyName(value);
    if (parsed) {
        g_hotkey = parsed;
        g_hotkeyName = value;
        std::transform(g_hotkeyName.begin(), g_hotkeyName.end(), g_hotkeyName.begin(),
                       [](wchar_t ch) { return static_cast<wchar_t>(towupper(ch)); });
    } else {
        g_hotkey = 'K';
        g_hotkeyName = L"K";
        Log("Invalid Hotkey in Caravan.ini; using K.");
    }
}

static void LoadGameUiResources() {
    const std::wstring fontPath = GamePath(L"_HD3_Data\\Common\\ncs75.otf");
    AddFontResourceExW(fontPath.c_str(), FR_PRIVATE, nullptr);
    g_gameFont = CreateFontW(-17, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, RUSSIAN_CHARSET,
                            OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY,
                            DEFAULT_PITCH | FF_ROMAN, L"NewCenturySchoolbookC");
    g_titleFont = CreateFontW(-23, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, RUSSIAN_CHARSET,
                             OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY,
                             DEFAULT_PITCH | FF_ROMAN, L"NewCenturySchoolbookC");
    g_smallFont = CreateFontW(-14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, RUSSIAN_CHARSET,
                             OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY,
                             DEFAULT_PITCH | FF_ROMAN, L"NewCenturySchoolbookC");
    g_cardTitleFont = CreateFontW(-16, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, RUSSIAN_CHARSET,
                                 OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY,
                                 DEFAULT_PITCH | FF_ROMAN, L"NewCenturySchoolbookC");
    const std::wstring backgroundPath = GamePath(L"_HD3_Data\\Common\\DlgDBlBk.bmp");
    const std::wstring buttonPath = GamePath(L"_HD3_Data\\Common\\GldBtn2.bmp");
    g_dialogTexture = static_cast<HBITMAP>(LoadImageW(nullptr, backgroundPath.c_str(), IMAGE_BITMAP, 0, 0,
                                                      LR_LOADFROMFILE | LR_CREATEDIBSECTION));
    g_buttonTexture = static_cast<HBITMAP>(LoadImageW(nullptr, buttonPath.c_str(), IMAGE_BITMAP, 0, 0,
                                                      LR_LOADFROMFILE | LR_CREATEDIBSECTION));
    g_parchmentBrush = CreateSolidBrush(RGB(238, 224, 174));
    g_labelBrush = CreateSolidBrush(RGB(20, 39, 86));
    g_titleBrush = CreateSolidBrush(RGB(111, 24, 25));
}

static bool IsReadable(const void* address, size_t bytes) {
    if (!address || !bytes) return false;
    MEMORY_BASIC_INFORMATION info{};
    if (!VirtualQuery(address, &info, sizeof(info))) return false;
    const uintptr_t start = reinterpret_cast<uintptr_t>(address);
    if (start < 0x10000 || bytes > UINTPTR_MAX - start) return false;
    const uintptr_t finish = reinterpret_cast<uintptr_t>(info.BaseAddress) + info.RegionSize;
    return info.State == MEM_COMMIT && !(info.Protect & (PAGE_NOACCESS | PAGE_GUARD)) && start + bytes <= finish;
}

static size_t VectorCount(const ExeVector* vector, size_t itemSize, size_t maximum) {
    if (!vector || !itemSize) return 0;
    const uintptr_t first = reinterpret_cast<uintptr_t>(vector->begin);
    const uintptr_t last = reinterpret_cast<uintptr_t>(vector->end);
    const uintptr_t capacity = reinterpret_cast<uintptr_t>(vector->capacity);
    if (!first && !last) return 0;
    if (first < 0x10000 || last < first || capacity < last) return 0;
    const size_t bytes = last - first;
    if (bytes % itemSize != 0 || bytes / itemSize > maximum) return 0;
    if (bytes && !IsReadable(vector->begin, bytes)) return 0;
    return bytes / itemSize;
}

static uint8_t* ReadPointer(uintptr_t virtualAddress) {
    auto slot = reinterpret_cast<uint8_t**>(g_moduleBase + virtualAddress - kImageBase);
    return IsReadable(slot, sizeof(*slot)) ? *slot : nullptr;
}

static uint8_t* Game() { return ReadPointer(kGamePointerVa); }

static ExeVector* VectorAt(size_t offset) {
    uint8_t* game = Game();
    if (!game) return nullptr;
    auto* value = reinterpret_cast<ExeVector*>(game + offset);
    return IsReadable(value, sizeof(*value)) ? value : nullptr;
}

#ifdef CARAVAN_PHYSICS
struct MapAccess {
    uint8_t* cells;
    int size;
    int levels;
};

struct RouteResult {
    bool found;
    int nextX;
    int nextY;
    int distance;
};

static uintptr_t GameAddress(uintptr_t virtualAddress) {
    return g_moduleBase + virtualAddress - kImageBase;
}

static bool GetMapAccess(MapAccess& map) {
    uint8_t* game = Game();
    if (!game) return false;
    uint8_t* world = game + kWorldMapOffset;
    if (!IsReadable(world + kWorldMapCellsOffset, kWorldMapTwoLevelsOffset - kWorldMapCellsOffset + 1)) return false;
    map.cells = *reinterpret_cast<uint8_t**>(world + kWorldMapCellsOffset);
    map.size = *reinterpret_cast<int32_t*>(world + kWorldMapSizeOffset);
    map.levels = world[kWorldMapTwoLevelsOffset] ? 2 : 1;
    const size_t bytes = static_cast<size_t>(map.size) * map.size * map.levels * kMapCellSize;
    return map.cells && map.size > 0 && map.size <= 252 && IsReadable(map.cells, bytes);
}

static uint8_t* MapCell(const MapAccess& map, int x, int y, int z) {
    if (x < 0 || y < 0 || z < 0 || z >= map.levels || x >= map.size || y >= map.size) return nullptr;
    return map.cells + (static_cast<size_t>(x) + static_cast<size_t>(map.size) *
           (static_cast<size_t>(y) + static_cast<size_t>(z) * map.size)) * kMapCellSize;
}

static int CellObjectType(const uint8_t* cell) {
    return cell ? static_cast<int>(*reinterpret_cast<const int16_t*>(cell + 0x1E)) : kObjectNone;
}

static bool CellHasNoObject(const uint8_t* cell) {
    const int type = CellObjectType(cell);
    // SoD headers use -1 for OBJECT_NONE, while the tested HotA 1.8.0
    // runtime stores 0 in ordinary empty adventure-map cells.
    return type == kObjectNone || type == 0;
}

static bool CellHasDrawing(const uint8_t* cell) {
    if (!cell) return false;
    // H3MapItem::objectDrawing is an ExeVector at +0x0E. Its begin/end
    // pointers are at +0x12/+0x16; decorative bridges live in this vector.
    uint32_t first = 0, last = 0;
    memcpy(&first, cell + 0x12, sizeof(first));
    memcpy(&last, cell + 0x16, sizeof(last));
    return first >= 0x10000u && last > first;
}

static bool BaseCellWalkable(const uint8_t* cell) {
    if (!cell) return false;
    const int terrain = static_cast<int8_t>(cell[0x04]);
    if (terrain < 0 || terrain == 9) return false;
    // Ordinary water is forbidden, but a road or a passable decorative
    // object over water can be a bridge. The previous implementation
    // rejected every water tile and therefore broke valid island routes.
    if (terrain == 8) return cell[0x08] != 0 || (CellHasDrawing(cell) && (cell[0x0D] & 0x01) == 0);
    return true;
}

static bool DynamicObject(int type) {
    return type == kObjectHero || type == kObjectMonster || type == kObjectGarrison || type == kObjectGarrisonHorizontal;
}

static bool PlanCellWalkable(const uint8_t* cell, bool allowDynamicBlockers) {
    if (!BaseCellWalkable(cell)) return false;
    const int type = CellObjectType(cell);
    // Heroes, monsters and garrisons mark their occupied tile inaccessible.
    // A first pass treats them as walls and therefore prefers a longer clear
    // detour. A fallback pass allows them only when no clear route exists, so
    // the caravan can wait for a genuinely unavoidable temporary obstacle.
    if (DynamicObject(type)) return allowDynamicBlockers;
    return CellHasNoObject(cell) && (cell[0x0D] & 0x01) == 0;
}

static bool CellEmptyForCaravan(const uint8_t* cell) {
    return BaseCellWalkable(cell) && (cell[0x0D] & 0x01) == 0 && CellHasNoObject(cell);
}

static void LogCellDiagnostics(const MapAccess& map, int centerX, int centerY, int z, int destinationX, int destinationY) {
    int valid = 0, base = 0, accessClear = 0, typeNone = 0, drawings = 0, empty = 0;
    int terrain[16]{};
    int commonTypes[6]{}; // -1, 0, hero, monster, garrison, other
    for (int y = centerY - 8; y <= centerY + 8; ++y) {
        for (int x = centerX - 8; x <= centerX + 8; ++x) {
            const uint8_t* cell = MapCell(map, x, y, z);
            if (!cell) continue;
            ++valid;
            const int land = static_cast<int8_t>(cell[0x04]);
            if (land >= 0 && land < 16) ++terrain[land];
            if (BaseCellWalkable(cell)) ++base;
            if ((cell[0x0D] & 0x01) == 0) ++accessClear;
            const int type = CellObjectType(cell);
            if (type == kObjectNone) { ++typeNone; ++commonTypes[0]; }
            else if (type == 0) { ++typeNone; ++commonTypes[1]; }
            else if (type == kObjectHero) ++commonTypes[2];
            else if (type == kObjectMonster) ++commonTypes[3];
            else if (type == kObjectGarrison || type == kObjectGarrisonHorizontal) ++commonTypes[4];
            else ++commonTypes[5];
            if (CellHasDrawing(cell)) ++drawings;
            if (CellEmptyForCaravan(cell)) ++empty;
        }
    }
    const uint8_t* source = MapCell(map, centerX, centerY, z);
    const uint8_t* destination = MapCell(map, destinationX, destinationY, z);
    char message[700]{};
    snprintf(message, sizeof(message),
             "Cell diagnostics: valid=%d base=%d accessClear=%d typeNone=%d drawings=%d empty=%d; terrain[0..9]=%d,%d,%d,%d,%d,%d,%d,%d,%d,%d; types[-1,0,hero,monster,garrison,other]=%d,%d,%d,%d,%d,%d; source[t=%d road=%d access=%u type=%d draw=%d]; destination[t=%d road=%d access=%u type=%d draw=%d].",
             valid, base, accessClear, typeNone, drawings, empty,
             terrain[0], terrain[1], terrain[2], terrain[3], terrain[4], terrain[5], terrain[6], terrain[7], terrain[8], terrain[9],
             commonTypes[0], commonTypes[1], commonTypes[2], commonTypes[3], commonTypes[4], commonTypes[5],
             source ? static_cast<int8_t>(source[0x04]) : -99, source ? static_cast<int8_t>(source[0x08]) : -99,
             source ? source[0x0D] : 0, CellObjectType(source), CellHasDrawing(source) ? 1 : 0,
             destination ? static_cast<int8_t>(destination[0x04]) : -99, destination ? static_cast<int8_t>(destination[0x08]) : -99,
             destination ? destination[0x0D] : 0, CellObjectType(destination), CellHasDrawing(destination) ? 1 : 0);
    Log(message);
}

static RouteResult FindRoute(const MapAccess& map, int startX, int startY, int z,
                             int destinationX, int destinationY, bool allowDynamicBlockers) {
    RouteResult result{false, startX, startY, 0};
    if (!MapCell(map, startX, startY, z)) return result;
    const int total = map.size * map.size;
    const int start = startX + startY * map.size;
    std::vector<int> previous(static_cast<size_t>(total), -2);
    std::vector<int> distance(static_cast<size_t>(total), -1);
    std::queue<int> open;
    previous[start] = -1;
    distance[start] = 0;
    open.push(start);
    int goal = -1;
    const int directions[8][2] = {{1,0},{-1,0},{0,1},{0,-1},{1,1},{1,-1},{-1,1},{-1,-1}};

    while (!open.empty()) {
        const int current = open.front(); open.pop();
        const int x = current % map.size;
        const int y = current / map.size;
        if (std::max(abs(x - destinationX), abs(y - destinationY)) <= 1 &&
            (current == start || CellEmptyForCaravan(MapCell(map, x, y, z)))) {
            goal = current;
            break;
        }
        for (const auto& direction : directions) {
            const int nx = x + direction[0];
            const int ny = y + direction[1];
            uint8_t* nextCell = MapCell(map, nx, ny, z);
            if (!nextCell || !PlanCellWalkable(nextCell, allowDynamicBlockers)) continue;
            const int next = nx + ny * map.size;
            if (previous[next] != -2) continue;
            previous[next] = current;
            distance[next] = distance[current] + 1;
            open.push(next);
        }
    }

    if (goal < 0) return result;
    int first = goal;
    while (previous[first] >= 0 && previous[first] != start) first = previous[first];
    result.found = true;
    result.nextX = first % map.size;
    result.nextY = first / map.size;
    result.distance = distance[goal];
    return result;
}

static RouteResult FindPreferredRoute(const MapAccess& map, int startX, int startY, int z,
                                      int destinationX, int destinationY) {
    RouteResult route = FindRoute(map, startX, startY, z, destinationX, destinationY, false);
    return route.found ? route : FindRoute(map, startX, startY, z, destinationX, destinationY, true);
}

static uint8_t* FindTownById(uint8_t id) {
    ExeVector* towns = VectorAt(kGameTownPoolOffset);
    const size_t count = VectorCount(towns, kTownSize, 256);
    for (size_t index = 0; index < count; ++index) {
        uint8_t* town = towns->begin + index * kTownSize;
        if (town[0] == id) return town;
    }
    return nullptr;
}

static bool IsPhysicalCaravan(const PhysicalGarrison& value) {
    return value.markerA == 0xCA && value.markerB == 0x47;
}

static bool ArmyIsEmpty(const Army& army) {
    for (int slot = 0; slot < 7; ++slot)
        if (army.type[slot] >= 0 && army.count[slot] > 0) return false;
    return true;
}

static bool MergeArmy(Army& destination, const Army& source, bool apply) {
    Army merged = destination;
    for (int sourceSlot = 0; sourceSlot < 7; ++sourceSlot) {
        if (source.type[sourceSlot] < 0 || source.count[sourceSlot] <= 0) continue;
        int target = -1;
        for (int slot = 0; slot < 7; ++slot) if (merged.type[slot] == source.type[sourceSlot]) { target = slot; break; }
        if (target < 0) for (int slot = 0; slot < 7; ++slot) if (merged.type[slot] < 0 || merged.count[slot] <= 0) { target = slot; break; }
        if (target < 0) return false;
        if (merged.type[target] < 0 || merged.count[target] <= 0) {
            merged.type[target] = source.type[sourceSlot];
            merged.count[target] = source.count[sourceSlot];
        } else {
            const int64_t total = static_cast<int64_t>(merged.count[target]) + source.count[sourceSlot];
            if (total > INT32_MAX) return false;
            merged.count[target] = static_cast<int32_t>(total);
        }
    }
    if (apply) destination = merged;
    return true;
}

static PhysicalGarrison* AppendPhysicalGarrison(size_t& index) {
    ExeVector* vector = VectorAt(kGameGarrisonPoolOffset);
    if (!vector) return nullptr;
    size_t count = 0, capacity = 0;
    if (vector->begin || vector->end || vector->capacity) {
        const uintptr_t first = reinterpret_cast<uintptr_t>(vector->begin);
        const uintptr_t last = reinterpret_cast<uintptr_t>(vector->end);
        const uintptr_t cap = reinterpret_cast<uintptr_t>(vector->capacity);
        if (first < 0x10000 || last < first || cap < last ||
            (last - first) % sizeof(PhysicalGarrison) || (cap - first) % sizeof(PhysicalGarrison)) return nullptr;
        count = (last - first) / sizeof(PhysicalGarrison);
        capacity = (cap - first) / sizeof(PhysicalGarrison);
    }
    if (count > 4096 || capacity > 8192) return nullptr;
    if (count == capacity) {
        const size_t newCapacity = std::max<size_t>(10, capacity ? capacity * 2 : 10);
        using MallocFn = void* (__cdecl *)(unsigned int);
        using FreeFn = void (__cdecl *)(void*);
        auto allocate = reinterpret_cast<MallocFn>(GameAddress(0x617492));
        auto release = reinterpret_cast<FreeFn>(GameAddress(0x60B0F0));
        uint8_t* replacement = static_cast<uint8_t*>(allocate(static_cast<unsigned int>(newCapacity * sizeof(PhysicalGarrison))));
        if (!replacement) return nullptr;
        if (count) memcpy(replacement, vector->begin, count * sizeof(PhysicalGarrison));
        if (vector->begin) release(vector->begin);
        vector->begin = replacement;
        vector->end = replacement + count * sizeof(PhysicalGarrison);
        vector->capacity = replacement + newCapacity * sizeof(PhysicalGarrison);
    }
    index = count;
    auto* result = reinterpret_cast<PhysicalGarrison*>(vector->end);
    memset(result, 0, sizeof(*result));
    vector->end += sizeof(PhysicalGarrison);
    return result;
}

static uint32_t PackedPoint(int x, int y, int z) {
    return (static_cast<uint32_t>(x) & 0x3FFu) |
           ((static_cast<uint32_t>(y) & 0x3FFu) << 16u) |
           ((static_cast<uint32_t>(z) & 0x0Fu) << 26u);
}

static void InsertGarrisonObject(int x, int y, int z, size_t index) {
    using InsertFn = void (__thiscall *)(void*, int, int, int, int, int, int);
    auto insert = reinterpret_cast<InsertFn>(GameAddress(0x4C9550));
    // Let the engine create and draw its native wagon object, then retain
    // that drawing while exposing the tile as a garrison for interaction.
    insert(Game(), x, y, z, kObjectWagon, 0, static_cast<int>(index));
    MapAccess map{};
    uint8_t* cell = GetMapAccess(map) ? MapCell(map, x, y, z) : nullptr;
    if (!cell) return;
    *reinterpret_cast<uint32_t*>(cell) = static_cast<uint32_t>(index);
    *reinterpret_cast<int16_t*>(cell + 0x1E) = static_cast<int16_t>(kObjectGarrison);
    *reinterpret_cast<int16_t*>(cell + 0x22) = 0;
}

static void EraseObjectAt(const MapAccess& map, int x, int y, int z) {
    uint8_t* manager = ReadPointer(kAdventureManagerPointerVa);
    uint8_t* cell = MapCell(map, x, y, z);
    if (!manager || !cell) return;
    using EraseFn = void (__thiscall *)(void*, void*, uint32_t, int);
    auto erase = reinterpret_cast<EraseFn>(GameAddress(0x4AA820));
    erase(manager, cell, PackedPoint(x, y, z), 0);
}

static bool FindSpawnCell(const CaravanRow& row, uint8_t* destinationTown, int& spawnX, int& spawnY, int& distance) {
    MapAccess map{};
    if (!GetMapAccess(map) || row.z != destinationTown[kTownZOffset]) return false;
    const int dx = destinationTown[kTownXOffset];
    const int dy = destinationTown[kTownYOffset];
    int best = INT32_MAX;
    int emptyCandidates = 0;

    // A dwelling stores the coordinates of its entrance object, not always
    // an empty neighbouring tile. Try the first map step from that entrance
    // before searching around the whole object footprint.
    RouteResult entranceRoute = FindPreferredRoute(map, row.x, row.y, row.z, dx, dy);
    if (entranceRoute.found &&
        CellEmptyForCaravan(MapCell(map, entranceRoute.nextX, entranceRoute.nextY, row.z))) {
        spawnX = entranceRoute.nextX;
        spawnY = entranceRoute.nextY;
        distance = std::max(0, entranceRoute.distance - 1);
        return true;
    }

    // Large dwelling DEFs and their blocked decorative footprint can extend
    // farther than three tiles from the stored entrance coordinate.
    for (int radius = 1; radius <= 8; ++radius) {
        for (int y = row.y - radius; y <= row.y + radius; ++y) {
            for (int x = row.x - radius; x <= row.x + radius; ++x) {
                if (std::max(abs(x - row.x), abs(y - row.y)) != radius) continue;
                if (!CellEmptyForCaravan(MapCell(map, x, y, row.z))) continue;
                ++emptyCandidates;
                RouteResult route = FindPreferredRoute(map, x, y, row.z, dx, dy);
                if (route.found && route.distance < best) {
                    best = route.distance; spawnX = x; spawnY = y;
                }
            }
        }
        if (best != INT32_MAX) break;
    }
    if (best == INT32_MAX) {
        char message[256]{};
        snprintf(message, sizeof(message), "No physical route: source=(%d,%d,%d), destination=(%d,%d,%d), map=%d, empty candidates=%d, entrance route=%d.",
                 row.x, row.y, row.z, dx, dy, destinationTown[kTownZOffset], map.size,
                 emptyCandidates, entranceRoute.found ? entranceRoute.distance : -1);
        Log(message);
        LogCellDiagnostics(map, row.x, row.y, row.z, dx, dy);
        return false;
    }
    distance = best;
    return true;
}

static bool SpawnPhysicalCaravan(const CaravanRow& row, int quantity, int spawnX, int spawnY) {
    size_t index = 0;
    PhysicalGarrison* caravan = AppendPhysicalGarrison(index);
    if (!caravan) return false;
    caravan->owner = static_cast<int8_t>(g_destinationTown[kTownOwnerOffset]);
    caravan->markerA = 0xCA;
    caravan->markerB = 0x47;
    caravan->destinationTownId = g_destinationTown[0];
    for (int slot = 0; slot < 7; ++slot) { caravan->army.type[slot] = -1; caravan->army.count[slot] = 0; }
    caravan->army.type[0] = row.creature;
    caravan->army.count[0] = quantity;
    caravan->removable = 1;
    caravan->x = static_cast<uint8_t>(spawnX);
    caravan->y = static_cast<uint8_t>(spawnY);
    caravan->z = static_cast<uint8_t>(row.z);
    InsertGarrisonObject(spawnX, spawnY, row.z, index);
    MapAccess map{};
    if (!GetMapAccess(map) || CellObjectType(MapCell(map, spawnX, spawnY, row.z)) != kObjectGarrison) {
        caravan->markerA = caravan->markerB = 0;
        caravan->owner = -1;
        for (int slot = 0; slot < 7; ++slot) { caravan->army.type[slot] = -1; caravan->army.count[slot] = 0; }
        caravan->x = caravan->y = caravan->z = 0xFF;
        return false;
    }
    return true;
}

static void DeactivateCaravan(const MapAccess& map, PhysicalGarrison& caravan, bool eraseObject) {
    if (eraseObject) EraseObjectAt(map, caravan.x, caravan.y, caravan.z);
    caravan.markerA = caravan.markerB = 0;
    caravan.owner = -1;
    caravan.x = caravan.y = caravan.z = 0xFF;
}

static void RestoreCaravanInteractionTypes(const MapAccess& map) {
    ExeVector* vector = VectorAt(kGameGarrisonPoolOffset);
    const size_t count = VectorCount(vector, sizeof(PhysicalGarrison), 4096);
    for (size_t index = 0; index < count; ++index) {
        auto& caravan = *reinterpret_cast<PhysicalGarrison*>(vector->begin + index * sizeof(PhysicalGarrison));
        if (!IsPhysicalCaravan(caravan)) continue;
        uint8_t* cell = MapCell(map, caravan.x, caravan.y, caravan.z);
        if (!cell) continue;
        *reinterpret_cast<uint32_t*>(cell) = static_cast<uint32_t>(index);
        *reinterpret_cast<int16_t*>(cell + 0x1E) = static_cast<int16_t>(kObjectGarrison);
        *reinterpret_cast<int16_t*>(cell + 0x22) = 0;
    }
}

static void CleanupEmptyPhysicalCaravans(const MapAccess& map) {
    ExeVector* vector = VectorAt(kGameGarrisonPoolOffset);
    const size_t count = VectorCount(vector, sizeof(PhysicalGarrison), 4096);
    if (g_emptyCaravanSince.size() < count) g_emptyCaravanSince.resize(count, 0);
    const uint32_t now = GetTickCount();
    for (size_t index = 0; index < count; ++index) {
        auto& caravan = *reinterpret_cast<PhysicalGarrison*>(vector->begin + index * sizeof(PhysicalGarrison));
        if (!IsPhysicalCaravan(caravan) || !ArmyIsEmpty(caravan.army)) {
            g_emptyCaravanSince[index] = 0;
            continue;
        }
        // Give the native garrison dialog time to finish after the player
        // transfers the last stack. The record stays allocated, so the
        // dialog keeps a valid address while the map object is removed soon
        // after it closes instead of lingering until the following day.
        if (!g_emptyCaravanSince[index]) {
            g_emptyCaravanSince[index] = now ? now : 1;
            continue;
        }
        if (now - g_emptyCaravanSince[index] < 2000u) continue;
        uint8_t* cell = MapCell(map, caravan.x, caravan.y, caravan.z);
        const bool objectStillPresent = cell && CellObjectType(cell) == kObjectGarrison &&
                                        *reinterpret_cast<uint32_t*>(cell) == index;
        DeactivateCaravan(map, caravan, objectStillPresent);
        g_emptyCaravanSince[index] = 0;
        Log("Empty caravan removed after troop collection.");
    }
}

static void ProcessPhysicalCaravans() {
    MapAccess map{};
    if (!GetMapAccess(map)) return;
    ExeVector* vector = VectorAt(kGameGarrisonPoolOffset);
    const size_t count = VectorCount(vector, sizeof(PhysicalGarrison), 4096);
    for (size_t index = 0; index < count; ++index) {
        auto& caravan = *reinterpret_cast<PhysicalGarrison*>(vector->begin + index * sizeof(PhysicalGarrison));
        if (!IsPhysicalCaravan(caravan)) continue;
        uint8_t* destinationTown = FindTownById(caravan.destinationTownId);
        if (!destinationTown || caravan.owner != static_cast<int8_t>(destinationTown[kTownOwnerOffset])) {
            caravan.markerA = caravan.markerB = 0;
            continue;
        }
        if (ArmyIsEmpty(caravan.army)) {
            DeactivateCaravan(map, caravan, true);
            continue;
        }
        const int destinationX = destinationTown[kTownXOffset];
        const int destinationY = destinationTown[kTownYOffset];
        const int destinationZ = destinationTown[kTownZOffset];
        if (caravan.z != destinationZ) continue;

        for (int step = 0; step < kCaravanTilesPerDay; ++step) {
            if (std::max(abs(static_cast<int>(caravan.x) - destinationX), abs(static_cast<int>(caravan.y) - destinationY)) <= 1) {
                Army* townArmy = reinterpret_cast<Army*>(destinationTown + kTownArmyOffset);
                if (MergeArmy(*townArmy, caravan.army, false)) {
                    MergeArmy(*townArmy, caravan.army, true);
                    DeactivateCaravan(map, caravan, true);
                }
                break;
            }
            RouteResult route = FindPreferredRoute(map, caravan.x, caravan.y, caravan.z, destinationX, destinationY);
            if (!route.found || (route.nextX == caravan.x && route.nextY == caravan.y)) break;
            uint8_t* nextCell = MapCell(map, route.nextX, route.nextY, caravan.z);
            if (!CellEmptyForCaravan(nextCell)) break;
            const int oldX = caravan.x, oldY = caravan.y;
            EraseObjectAt(map, oldX, oldY, caravan.z);
            caravan.x = static_cast<uint8_t>(route.nextX);
            caravan.y = static_cast<uint8_t>(route.nextY);
            InsertGarrisonObject(caravan.x, caravan.y, caravan.z, index);
            if (CellObjectType(MapCell(map, caravan.x, caravan.y, caravan.z)) != kObjectGarrison) {
                caravan.x = static_cast<uint8_t>(oldX); caravan.y = static_cast<uint8_t>(oldY);
                InsertGarrisonObject(oldX, oldY, caravan.z, index);
                break;
            }
        }
    }
}

static uint32_t CurrentGameDay(uint8_t* game) {
    if (!game || !IsReadable(game + kGameDateOffset, 6)) return UINT32_MAX;
    const uint16_t day = *reinterpret_cast<uint16_t*>(game + kGameDateOffset);
    const uint16_t week = *reinterpret_cast<uint16_t*>(game + kGameDateOffset + 2);
    const uint16_t month = *reinterpret_cast<uint16_t*>(game + kGameDateOffset + 4);
    if (day < 1 || day > 7 || week < 1 || week > 4 || month < 1) return UINT32_MAX;
    return 28u * (month - 1u) + 7u * (week - 1u) + day;
}

static void TickPhysicalCaravans() {
    uint8_t* game = Game();
    MapAccess map{};
    const uint32_t day = CurrentGameDay(game);
    if (!game || day == UINT32_MAX || !GetMapAccess(map)) return;
    if (map.cells != g_lastMapCells) {
        g_lastMapCells = map.cells;
        g_lastGameDay = day;
        g_emptyCaravanSince.clear();
        RestoreCaravanInteractionTypes(map);
        return;
    }
    CleanupEmptyPhysicalCaravans(map);
    if (day == g_lastGameDay) return;
    const bool nextDay = g_lastGameDay != UINT32_MAX && day == g_lastGameDay + 1;
    g_lastGameDay = day;
    RestoreCaravanInteractionTypes(map);
    if (nextDay) {
        Sleep(500);
        ProcessPhysicalCaravans();
    }
}

static LRESULT CALLBACK GameThreadMessageHook(int code, WPARAM wParam, LPARAM lParam) {
    if (code >= 0 && lParam && g_physicalTickMessage) {
        MSG* message = reinterpret_cast<MSG*>(lParam);
        if (message->message == g_physicalTickMessage && message->hwnd == g_physicalGameWindow) {
            TickPhysicalCaravans();
            InterlockedExchange(&g_physicalTickPending, 0);
        }
    }
    return CallNextHookEx(g_gameThreadHook, code, wParam, lParam);
}

static bool InstallGameThreadHook(HWND gameWindow) {
    if (!gameWindow || !IsWindow(gameWindow)) return false;
    DWORD process = 0;
    const DWORD thread = GetWindowThreadProcessId(gameWindow, &process);
    if (!thread || process != GetCurrentProcessId()) return false;
    if (g_gameThreadHook && g_physicalGameWindow == gameWindow) return true;
    if (g_gameThreadHook) {
        UnhookWindowsHookEx(g_gameThreadHook);
        g_gameThreadHook = nullptr;
    }
    g_physicalGameWindow = gameWindow;
    if (!g_physicalTickMessage)
        g_physicalTickMessage = RegisterWindowMessageW(L"H3Caravan.PhysicalTick.1");
    InterlockedExchange(&g_physicalTickPending, 0);
    g_gameThreadHook = SetWindowsHookExW(WH_GETMESSAGE, GameThreadMessageHook, nullptr, thread);
    if (!g_gameThreadHook) {
        g_physicalGameWindow = nullptr;
        Log("Could not install the main-thread caravan tick hook.");
        return false;
    }
    Log("Main-thread caravan tick hook installed.");
    return true;
}

static void RequestPhysicalCaravanTick() {
    if (!g_gameThreadHook || !g_physicalGameWindow || !IsWindow(g_physicalGameWindow)) return;
    if (InterlockedCompareExchange(&g_physicalTickPending, 1, 0) != 0) return;
    if (!PostMessageW(g_physicalGameWindow, g_physicalTickMessage, 0, 0))
        InterlockedExchange(&g_physicalTickPending, 0);
}
#endif

static uint8_t* CreatureTraits(int creature) {
    if (creature < 0 || creature > 1023) return nullptr;
    uint8_t* table = ReadPointer(kCreatureTraitsPointerVa);
    if (!table) return nullptr;
    uint8_t* traits = table + static_cast<size_t>(creature) * kCreatureTraitsSize;
    return IsReadable(traits, kCreatureTraitsSize) ? traits : nullptr;
}

static int CreatureCapacity() {
    uint8_t* table = ReadPointer(kCreatureTraitsPointerVa);
    if (!table) return 0;
    MEMORY_BASIC_INFORMATION info{};
    if (!VirtualQuery(table, &info, sizeof(info))) return 0;
    const uintptr_t end = reinterpret_cast<uintptr_t>(info.BaseAddress) + info.RegionSize;
    return static_cast<int>(std::min<size_t>(512, (end - reinterpret_cast<uintptr_t>(table)) / kCreatureTraitsSize));
}

static std::wstring FromGameText(const char* text) {
    if (!text || !IsReadable(text, 1)) return L"Неизвестное существо";
    size_t length = 0;
    while (length < 128 && IsReadable(text + length, 1) && text[length]) ++length;
    if (!length) return L"Неизвестное существо";
    int needed = MultiByteToWideChar(1251, 0, text, static_cast<int>(length), nullptr, 0);
    std::wstring result(needed, L'\0');
    MultiByteToWideChar(1251, 0, text, static_cast<int>(length), result.data(), needed);
    return result;
}

static std::wstring CreatureName(int creature) {
    uint8_t* traits = CreatureTraits(creature);
    if (!traits) return L"Неизвестное существо";
    return FromGameText(*reinterpret_cast<const char**>(traits + kCreatureNameOffset));
}

static int Available(const CaravanRow& row) {
    if (!row.amount) return 0;
    if (!IsReadable(row.amount, row.amount16 ? sizeof(int16_t) : sizeof(int32_t))) return 0;
    return row.amount16 ? static_cast<int>(*reinterpret_cast<int16_t*>(row.amount)) : *reinterpret_cast<int32_t*>(row.amount);
}

static void ChangeAvailable(const CaravanRow& row, int delta) {
    if (!row.amount || !IsReadable(row.amount, row.amount16 ? sizeof(int16_t) : sizeof(int32_t))) return;
    if (row.amount16) *reinterpret_cast<int16_t*>(row.amount) = static_cast<int16_t>(Available(row) + delta);
    else *reinterpret_cast<int32_t*>(row.amount) += delta;
}

static bool IsPaid(const CaravanRow& row) {
    if (row.kind == SourceKind::TownGarrison) return false;
    if (row.kind == SourceKind::Dwelling) {
        uint8_t* traits = CreatureTraits(row.creature);
        if (traits && *reinterpret_cast<int*>(traits + kCreatureLevelOffset) == 0) return false;
    }
    return true;
}

static int* PlayerResources(int owner) {
    uint8_t* game = Game();
    if (!game || owner < 0 || owner > 7) return nullptr;
    int* resources = reinterpret_cast<int*>(game + kGamePlayersOffset + owner * kPlayerSize + kPlayerResourcesOffset);
    return IsReadable(resources, sizeof(int) * 7) ? resources : nullptr;
}

static bool CostFor(int creature, int quantity, int total[7]) {
    uint8_t* traits = CreatureTraits(creature);
    if (!traits || quantity <= 0) return false;
    const int* perUnit = reinterpret_cast<int*>(traits + kCreatureCostOffset);
    for (int i = 0; i < 7; ++i) {
        const int64_t value = static_cast<int64_t>(perUnit[i]) * quantity;
        if (value < 0 || value > INT32_MAX) return false;
        total[i] = static_cast<int>(value);
    }
    return true;
}

static std::wstring CostText(int creature, int quantity, bool paid) {
    if (!paid) return L"Цена: уже оплачено (перевод гарнизона)";
    int cost[7]{};
    if (!CostFor(creature, quantity, cost)) return L"Цена: —";
    const wchar_t* names[7] = {L"дер.", L"рт.", L"руды", L"серы", L"крист.", L"самоцв.", L"зол."};
    std::wstring result = L"Цена:";
    bool any = false;
    for (int i = 0; i < 7; ++i) {
        if (!cost[i]) continue;
        wchar_t part[64];
        swprintf(part, 64, L" %d %ls", cost[i], names[i]);
        result += part;
        any = true;
    }
    return any ? result : L"Цена: бесплатно";
}

static int FindTownCreature(int townType, int level, int upgradeIndex) {
    std::vector<int> candidates;
    for (int id = 0, capacity = CreatureCapacity(); id < capacity; ++id) {
        uint8_t* traits = CreatureTraits(id);
        if (traits && *reinterpret_cast<int*>(traits + kCreatureTownOffset) == townType &&
            *reinterpret_cast<int*>(traits + kCreatureLevelOffset) == level)
            candidates.push_back(id);
    }
    std::sort(candidates.begin(), candidates.end());
    return upgradeIndex >= 0 && upgradeIndex < static_cast<int>(candidates.size()) ? candidates[upgradeIndex] : -1;
}

static void AddRow(SourceKind kind, uint8_t* source, void* amount, bool amount16, int creature, int x, int y, int z, const wchar_t* prefix) {
    if (creature < 0 || !amount) return;
    CaravanRow row{kind, source, amount, amount16, creature, x, y, z, L"", CreatureName(creature)};
    wchar_t sourceText[128];
    swprintf(sourceText, 128, L"%ls (%d,%d)", prefix, x, y);
    row.sourceText = sourceText;
    if (Available(row) > 0) g_rows.push_back(std::move(row));
}

static void CollectRows() {
    g_rows.clear();
    if (!g_destinationTown) return;
    const int owner = static_cast<int8_t>(g_destinationTown[kTownOwnerOffset]);

    if (ExeVector* generators = VectorAt(kGameGeneratorPoolOffset)) {
        const size_t count = VectorCount(generators, kGeneratorSize, 4096);
        for (size_t index = 0; index < count; ++index) {
            uint8_t* gen = generators->begin + index * kGeneratorSize;
            if (static_cast<int8_t>(gen[kGeneratorOwnerOffset]) != owner) continue;
            for (int slot = 0; slot < 4; ++slot) {
                int creature = *reinterpret_cast<int*>(gen + kGeneratorTypesOffset + slot * 4);
                AddRow(SourceKind::Dwelling, gen, gen + kGeneratorPopulationOffset + slot * 2, true, creature,
                       gen[kGeneratorXOffset], gen[kGeneratorYOffset], gen[kGeneratorZOffset], L"Жилище");
            }
        }
    }

    if (ExeVector* towns = VectorAt(kGameTownPoolOffset)) {
        const size_t count = VectorCount(towns, kTownSize, 256);
        for (size_t index = 0; index < count; ++index) {
            uint8_t* town = towns->begin + index * kTownSize;
            if (town == g_destinationTown || static_cast<int8_t>(town[kTownOwnerOffset]) != owner) continue;
            const int townType = static_cast<int8_t>(town[kTownTypeOffset]);
            for (int upgraded = 0; upgraded < 2; ++upgraded) {
                for (int level = 0; level < 7; ++level) {
                    int creature = FindTownCreature(townType, level, upgraded);
                    AddRow(SourceKind::TownPool, town, town + kTownPopulationOffset + (upgraded * 7 + level) * 2,
                           true, creature, town[kTownXOffset], town[kTownYOffset], town[kTownZOffset], L"Город: резерв");
                }
            }
            auto* army = reinterpret_cast<Army*>(town + kTownArmyOffset);
            for (int slot = 0; slot < 7; ++slot)
                AddRow(SourceKind::TownGarrison, town, &army->count[slot], false, army->type[slot],
                       town[kTownXOffset], town[kTownYOffset], town[kTownZOffset], L"Город: гарнизон");
        }
    }
}

static int CurrentFilter() { return g_filterSelection; }
static bool MatchesFilter(const CaravanRow& row, int filter) {
    return filter == 0 || (filter == 1 && row.kind == SourceKind::Dwelling) ||
           (filter == 2 && row.kind == SourceKind::TownPool) || (filter == 3 && row.kind == SourceKind::TownGarrison);
}

static void FillList() {
    ListView_DeleteAllItems(g_list);
    const int filter = CurrentFilter();
    int visible = 0;
    for (size_t i = 0; i < g_rows.size(); ++i) {
        const CaravanRow& row = g_rows[i];
        if (!MatchesFilter(row, filter) || Available(row) <= 0) continue;
        LVITEMW item{};
        item.mask = LVIF_TEXT | LVIF_PARAM;
        item.iItem = visible++;
        item.pszText = const_cast<wchar_t*>(L"");
        item.lParam = static_cast<LPARAM>(i);
        ListView_InsertItem(g_list, &item);
    }
    if (visible > 0) {
        ListView_SetItemState(g_list, 0, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
    }
}

static int SelectedRowIndex() {
    const int selected = ListView_GetNextItem(g_list, -1, LVNI_SELECTED);
    if (selected < 0) return -1;
    LVITEMW item{};
    item.mask = LVIF_PARAM;
    item.iItem = selected;
    return ListView_GetItem(g_list, &item) ? static_cast<int>(item.lParam) : -1;
}

static int Quantity() {
    wchar_t text[32]{};
    GetWindowTextW(g_quantity, text, 31);
    const int value = _wtoi(text);
    return value > 0 ? value : 1;
}

static int MaximumOrderQuantity(const CaravanRow& row) {
    int maximum = std::max(0, Available(row));
    if (!maximum || !IsPaid(row)) return maximum;
    const int owner = g_destinationTown ? static_cast<int8_t>(g_destinationTown[kTownOwnerOffset]) : -1;
    int* resources = PlayerResources(owner);
    uint8_t* traits = CreatureTraits(row.creature);
    if (!resources || !traits) return 0;
    const int* perUnit = reinterpret_cast<int*>(traits + kCreatureCostOffset);
    for (int resource = 0; resource < 7; ++resource) {
        if (perUnit[resource] < 0) return 0;
        if (perUnit[resource] > 0)
            maximum = std::min(maximum, resources[resource] / perUnit[resource]);
    }
    return std::max(0, maximum);
}

static void UpdatePrice() {
    const int index = SelectedRowIndex();
    std::wstring value = L"Выберите существ в списке.";
    if (index >= 0 && index < static_cast<int>(g_rows.size()))
        value = CostText(g_rows[index].creature, Quantity(), IsPaid(g_rows[index]));
    SetWindowTextW(g_price, value.c_str());
}

static void CancelConfirmation() {
    g_pendingRow = -1;
    g_pendingQuantity = 0;
    if (g_send) SetWindowTextW(g_send, L"Отправить");
}

static void ShowStatus(const std::wstring& value) {
    if (g_price) SetWindowTextW(g_price, value.c_str());
}

static void SelectMaximumQuantity() {
    const int index = SelectedRowIndex();
    if (index < 0 || index >= static_cast<int>(g_rows.size())) {
        CancelConfirmation();
        ShowStatus(L"Сначала выберите отряд в списке.");
        return;
    }
    const int maximum = MaximumOrderQuantity(g_rows[index]);
    if (maximum <= 0) {
        CancelConfirmation();
        ShowStatus(L"Недостаточно ресурсов для покупки выбранных существ.");
        return;
    }
    const std::wstring value = std::to_wstring(maximum);
    SetWindowTextW(g_quantity, value.c_str());
    CancelConfirmation();
    UpdatePrice();
}

#ifndef CARAVAN_PHYSICS
static int DestinationSlot(int creature) {
    auto* army = reinterpret_cast<Army*>(g_destinationTown + kTownArmyOffset);
    int empty = -1;
    for (int i = 0; i < 7; ++i) {
        if (army->type[i] == creature) return i;
        if (army->type[i] == -1 && empty < 0) empty = i;
    }
    return empty;
}
#endif

static void SendCaravan(HWND window) {
    (void)window;
    const int index = SelectedRowIndex();
    if (index < 0 || index >= static_cast<int>(g_rows.size())) {
        CancelConfirmation();
        ShowStatus(L"Сначала выберите отряд в таблице.");
        return;
    }
    CaravanRow& row = g_rows[index];
    const int quantity = Quantity();
    if (quantity <= 0 || quantity > Available(row)) {
        CancelConfirmation();
        ShowStatus(L"Такого количества сейчас нет в источнике.");
        return;
    }
    const int owner = static_cast<int8_t>(g_destinationTown[kTownOwnerOffset]);
    const int sourceOwner = row.kind == SourceKind::Dwelling ? static_cast<int8_t>(row.source[kGeneratorOwnerOffset]) : static_cast<int8_t>(row.source[kTownOwnerOffset]);
    if (owner < 0 || owner > 7 || sourceOwner != owner) {
        CancelConfirmation();
        CollectRows(); FillList();
        ShowStatus(L"Владелец источника изменился. Список обновлён.");
        return;
    }
#ifndef CARAVAN_PHYSICS
    const int slot = DestinationSlot(row.creature);
    if (slot < 0) {
        CancelConfirmation();
        ShowStatus(L"В гарнизоне назначения нет свободного места.");
        return;
    }
#endif

#ifdef CARAVAN_PHYSICS
    int spawnX = 0, spawnY = 0, distance = 0;
    if (!FindSpawnCell(row, g_destinationTown, spawnX, spawnY, distance)) {
        CancelConfirmation();
        ShowStatus(L"Не найден свободный сухопутный путь. Ничего не списано.");
        return;
    }
    const int estimatedDays = std::max(1, (distance + kCaravanTilesPerDay - 1) / kCaravanTilesPerDay);
#endif

    int cost[7]{};
    int* resources = PlayerResources(owner);
    if (IsPaid(row)) {
        if (!resources || !CostFor(row.creature, quantity, cost)) {
            CancelConfirmation(); ShowStatus(L"Не удалось рассчитать стоимость."); return;
        }
        for (int i = 0; i < 7; ++i) if (resources[i] < cost[i]) {
            CancelConfirmation();
            ShowStatus(L"Недостаточно ресурсов. Ничего не списано.");
            return;
        }
    }

    if (g_pendingRow != index || g_pendingQuantity != quantity) {
        g_pendingRow = index;
        g_pendingQuantity = quantity;
        if (g_send) SetWindowTextW(g_send, L"Подтвердить");
        std::wstring confirmation = L"Ещё раз нажмите «Подтвердить»: " + std::to_wstring(quantity) + L" × " +
                                    row.creatureText + L"; " + CostText(row.creature, quantity, IsPaid(row));
#ifdef CARAVAN_PHYSICS
        confirmation += L"; путь примерно " + std::to_wstring(estimatedDays) + L" дн.";
#endif
        ShowStatus(confirmation);
        return;
    }

#ifdef CARAVAN_PHYSICS
    // The map object is created first. Any placement failure leaves the
    // source stack and the player's resources untouched.
    if (!SpawnPhysicalCaravan(row, quantity, spawnX, spawnY)) {
        CancelConfirmation();
        ShowStatus(L"Не удалось поставить караван на карту. Ничего не списано.");
        return;
    }
#endif
    if (IsPaid(row)) for (int i = 0; i < 7; ++i) resources[i] -= cost[i];
    ChangeAvailable(row, -quantity);
#ifndef CARAVAN_PHYSICS
    auto* destination = reinterpret_cast<Army*>(g_destinationTown + kTownArmyOffset);
    if (destination->type[slot] == -1) { destination->type[slot] = row.creature; destination->count[slot] = quantity; }
    else destination->count[slot] += quantity;
#endif
    if (row.kind == SourceKind::TownGarrison && Available(row) == 0) {
        auto* sourceArmy = reinterpret_cast<Army*>(row.source + kTownArmyOffset);
        for (int i = 0; i < 7; ++i) if (&sourceArmy->count[i] == row.amount) sourceArmy->type[i] = -1;
    }
    CancelConfirmation();
    FillList();
#ifdef CARAVAN_PHYSICS
    ShowStatus(L"Караван вышел на карту. Расчётное время: " + std::to_wstring(estimatedDays) + L" дн.");
#else
    ShowStatus(L"Караван прибыл. Списана только подтверждённая стоимость.");
#endif
}

static void DrawCardText(HDC dc, HFONT font, COLORREF color, const std::wstring& text, RECT area, UINT format) {
    HGDIOBJ oldFont = font ? SelectObject(dc, font) : nullptr;
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, color);
    DrawTextW(dc, text.c_str(), -1, &area, format | DT_NOPREFIX);
    if (oldFont) SelectObject(dc, oldFont);
}

static void DrawCreatureCard(HDC dc, RECT card, int rowIndex, bool selected) {
    if (rowIndex < 0 || rowIndex >= static_cast<int>(g_rows.size())) return;
    const CaravanRow& row = g_rows[rowIndex];

    InflateRect(&card, -5, -5);
    HBRUSH body = CreateSolidBrush(selected ? RGB(69, 48, 34) : RGB(48, 39, 30));
    HBRUSH header = CreateSolidBrush(selected ? RGB(145, 39, 31) : RGB(105, 25, 26));
    HPEN outer = CreatePen(PS_SOLID, selected ? 3 : 2, selected ? RGB(255, 215, 102) : RGB(161, 120, 51));
    HGDIOBJ oldBrush = SelectObject(dc, body);
    HGDIOBJ oldPen = SelectObject(dc, outer);
    Rectangle(dc, card.left, card.top, card.right, card.bottom);

    RECT title{card.left + 2, card.top + 2, card.right - 2, card.top + 25};
    FillRect(dc, &title, header);
    RECT titleText = title;
    InflateRect(&titleText, -5, 0);
    DrawCardText(dc, g_cardTitleFont, RGB(255, 230, 167), row.creatureText,
                 titleText, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

    const int dividerX = card.left + (card.right - card.left) * 55 / 100;
    HPEN divider = CreatePen(PS_SOLID, 1, RGB(151, 112, 52));
    SelectObject(dc, divider);
    MoveToEx(dc, dividerX, title.bottom + 3, nullptr);
    LineTo(dc, dividerX, card.bottom - 4);

    RECT sourceArea{card.left + 9, title.bottom + 5, dividerX - 7, title.bottom + 24};
    DrawCardText(dc, g_smallFont, RGB(231, 211, 155), row.sourceText,
                 sourceArea, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
    RECT amountArea{card.left + 9, title.bottom + 25, dividerX - 7, title.bottom + 44};
    DrawCardText(dc, g_gameFont, RGB(255, 235, 164),
                 L"Доступно: " + std::to_wstring(Available(row)), amountArea,
                 DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    std::wstring price = CostText(row.creature, 1, IsPaid(row));
    RECT priceArea{card.left + 9, title.bottom + 47, dividerX - 7, card.bottom - 4};
    DrawCardText(dc, g_smallFont, RGB(210, 190, 137), price,
                 priceArea, DT_LEFT | DT_TOP | DT_WORDBREAK | DT_END_ELLIPSIS);

    uint8_t* traits = CreatureTraits(row.creature);
    if (traits) {
        const int attack = *reinterpret_cast<int*>(traits + kCreatureAttackOffset);
        const int defense = *reinterpret_cast<int*>(traits + kCreatureDefenseOffset);
        const int damageLow = *reinterpret_cast<int*>(traits + kCreatureDamageLowOffset);
        const int damageHigh = *reinterpret_cast<int*>(traits + kCreatureDamageHighOffset);
        const int health = *reinterpret_cast<int*>(traits + kCreatureHealthOffset);
        const int speed = *reinterpret_cast<int*>(traits + kCreatureSpeedOffset);
        const int growth = *reinterpret_cast<int*>(traits + kCreatureGrowthOffset);
        wchar_t stats[192]{};
        swprintf(stats, 192, L"Атака: %d     Защита: %d\nУрон: %d–%d     Здоровье: %d\nСкорость: %d     Прирост: %d",
                 attack, defense, damageLow, damageHigh, health, speed, growth);
        RECT statsArea{dividerX + 8, title.bottom + 6, card.right - 7, card.bottom - 5};
        DrawCardText(dc, g_smallFont, RGB(244, 225, 169), stats,
                     statsArea, DT_LEFT | DT_TOP | DT_WORDBREAK);
    }

    SelectObject(dc, oldPen);
    SelectObject(dc, oldBrush);
    DeleteObject(divider);
    DeleteObject(outer);
    DeleteObject(header);
    DeleteObject(body);
}

static BOOL CALLBACK ApplyGameFont(HWND child, LPARAM) {
    if (g_gameFont) SendMessageW(child, WM_SETFONT, reinterpret_cast<WPARAM>(g_gameFont), TRUE);
    return TRUE;
}

static void PaintDialog(HDC dc, HWND window) {
    RECT area{};
    GetClientRect(window, &area);
    if (g_dialogTexture) {
        BITMAP bitmap{};
        GetObjectW(g_dialogTexture, sizeof(bitmap), &bitmap);
        HDC memory = CreateCompatibleDC(dc);
        HGDIOBJ old = SelectObject(memory, g_dialogTexture);
        for (int y = 0; y < area.bottom; y += bitmap.bmHeight)
            for (int x = 0; x < area.right; x += bitmap.bmWidth)
                BitBlt(dc, x, y, bitmap.bmWidth, bitmap.bmHeight, memory, 0, 0, SRCCOPY);
        SelectObject(memory, old);
        DeleteDC(memory);
    } else {
        HBRUSH fallback = CreateSolidBrush(RGB(17, 35, 82));
        FillRect(dc, &area, fallback);
        DeleteObject(fallback);
    }

    HBRUSH hollow = static_cast<HBRUSH>(GetStockObject(HOLLOW_BRUSH));
    HGDIOBJ oldBrush = SelectObject(dc, hollow);
    HPEN dark = CreatePen(PS_SOLID, 3, RGB(74, 45, 13));
    HPEN gold = CreatePen(PS_SOLID, 2, RGB(224, 177, 70));
    HPEN light = CreatePen(PS_SOLID, 1, RGB(255, 229, 142));
    HGDIOBJ oldPen = SelectObject(dc, dark);
    Rectangle(dc, 1, 1, area.right - 1, area.bottom - 1);
    SelectObject(dc, gold);
    Rectangle(dc, 5, 5, area.right - 5, area.bottom - 5);
    SelectObject(dc, light);
    Rectangle(dc, 8, 8, area.right - 8, area.bottom - 8);
    SelectObject(dc, oldPen);
    SelectObject(dc, oldBrush);
    DeleteObject(dark); DeleteObject(gold); DeleteObject(light);

    RECT titleBand{12, 10, area.right - 12, 43};
    HBRUSH titleFill = CreateSolidBrush(RGB(111, 24, 25));
    HPEN titleEdge = CreatePen(PS_SOLID, 1, RGB(235, 190, 77));
    oldBrush = SelectObject(dc, titleFill);
    oldPen = SelectObject(dc, titleEdge);
    Rectangle(dc, titleBand.left, titleBand.top, titleBand.right, titleBand.bottom);
    SelectObject(dc, oldPen);
    SelectObject(dc, oldBrush);
    DeleteObject(titleEdge);
    DeleteObject(titleFill);
}

static void DrawGoldButton(const DRAWITEMSTRUCT* item) {
    RECT area = item->rcItem;
    const bool pressed = (item->itemState & ODS_SELECTED) != 0;
    if (g_buttonTexture) {
        BITMAP bitmap{};
        GetObjectW(g_buttonTexture, sizeof(bitmap), &bitmap);
        HDC memory = CreateCompatibleDC(item->hDC);
        HGDIOBJ old = SelectObject(memory, g_buttonTexture);
        SetStretchBltMode(item->hDC, HALFTONE);
        StretchBlt(item->hDC, area.left, area.top, area.right - area.left, area.bottom - area.top,
                   memory, 0, 0, bitmap.bmWidth, bitmap.bmHeight, SRCCOPY);
        SelectObject(memory, old);
        DeleteDC(memory);
    } else {
        HBRUSH fill = CreateSolidBrush(RGB(213, 177, 87));
        FillRect(item->hDC, &area, fill);
        DeleteObject(fill);
    }

    HPEN shadow = CreatePen(PS_SOLID, 2, RGB(74, 45, 13));
    HPEN shine = CreatePen(PS_SOLID, 1, RGB(255, 238, 166));
    HGDIOBJ oldPen = SelectObject(item->hDC, pressed ? shine : shadow);
    HGDIOBJ oldBrush = SelectObject(item->hDC, GetStockObject(HOLLOW_BRUSH));
    Rectangle(item->hDC, area.left, area.top, area.right, area.bottom);
    SelectObject(item->hDC, pressed ? shadow : shine);
    MoveToEx(item->hDC, area.left + 2, area.bottom - 2, nullptr);
    LineTo(item->hDC, area.right - 2, area.bottom - 2);
    LineTo(item->hDC, area.right - 2, area.top + 2);

    wchar_t text[64]{};
    GetWindowTextW(item->hwndItem, text, 63);
    if (pressed) OffsetRect(&area, 1, 1);
    SetBkMode(item->hDC, TRANSPARENT);
    SetTextColor(item->hDC, RGB(55, 33, 11));
    HGDIOBJ oldFont = g_gameFont ? SelectObject(item->hDC, g_gameFont) : nullptr;
    DrawTextW(item->hDC, text, -1, &area, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    if (oldFont) SelectObject(item->hDC, oldFont);
    SelectObject(item->hDC, oldPen);
    SelectObject(item->hDC, oldBrush);
    DeleteObject(shadow); DeleteObject(shine);
}

static void DrawFilterButton(const DRAWITEMSTRUCT* item) {
    const int filter = static_cast<int>(item->CtlID) - kIdFilterAll;
    const bool selected = filter == g_filterSelection;
    RECT area = item->rcItem;
    HBRUSH fill = CreateSolidBrush(selected ? RGB(119, 27, 27) : RGB(222, 191, 111));
    HPEN edge = CreatePen(PS_SOLID, selected ? 2 : 1, selected ? RGB(255, 218, 105) : RGB(78, 46, 16));
    HGDIOBJ oldBrush = SelectObject(item->hDC, fill);
    HGDIOBJ oldPen = SelectObject(item->hDC, edge);
    Rectangle(item->hDC, area.left, area.top, area.right, area.bottom);
    wchar_t label[64]{};
    GetWindowTextW(item->hwndItem, label, 63);
    SetBkMode(item->hDC, TRANSPARENT);
    SetTextColor(item->hDC, selected ? RGB(255, 228, 148) : RGB(55, 33, 11));
    HGDIOBJ oldFont = g_smallFont ? SelectObject(item->hDC, g_smallFont) : nullptr;
    DrawTextW(item->hDC, label, -1, &area, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
    if (oldFont) SelectObject(item->hDC, oldFont);
    SelectObject(item->hDC, oldPen);
    SelectObject(item->hDC, oldBrush);
    DeleteObject(edge);
    DeleteObject(fill);
}

static void PositionOverlay() {
    if (!g_dialog || !g_gameWindow || !IsWindow(g_gameWindow)) return;
    RECT client{};
    POINT origin{0, 0};
    if (!GetClientRect(g_gameWindow, &client) || !ClientToScreen(g_gameWindow, &origin)) return;
    const int width = 860, height = 620;
    const int clientWidth = client.right - client.left;
    const int clientHeight = client.bottom - client.top;
    const int x = origin.x + std::max(0, (clientWidth - width) / 2);
    const int y = origin.y + std::max(0, (clientHeight - height) / 2);
    SetWindowPos(g_dialog, HWND_TOPMOST, x, y, width, height, SWP_NOACTIVATE | SWP_SHOWWINDOW);
}

static LRESULT CALLBACK DialogProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_MOUSEACTIVATE:
        return MA_NOACTIVATE;
    case WM_CREATE: {
        const std::wstring title = L"Караваны  [" + g_hotkeyName + L"]";
        g_title = CreateWindowW(L"STATIC", title.c_str(), WS_CHILD | WS_VISIBLE | SS_CENTER, 20, 14, 820, 25, window, reinterpret_cast<HMENU>(kIdTitle), nullptr, nullptr);
        CreateWindowW(L"STATIC", L"Источник:", WS_CHILD | WS_VISIBLE, 24, 51, 88, 22, window, nullptr, nullptr, nullptr);
        g_filterSelection = 0;
        const wchar_t* filters[] = {L"Все", L"Жилища", L"Резервы городов", L"Гарнизоны"};
        const int filterIds[] = {kIdFilterAll, kIdFilterDwellings, kIdFilterReserves, kIdFilterGarrisons};
        const int filterX[] = {112, 250, 388, 602};
        const int filterW[] = {130, 130, 206, 230};
        for (int i = 0; i < 4; ++i)
            g_filterButtons[i] = CreateWindowW(L"BUTTON", filters[i], WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
                                                filterX[i], 46, filterW[i], 28, window,
                                                reinterpret_cast<HMENU>(filterIds[i]), nullptr, nullptr);
        g_list = CreateWindowW(WC_LISTVIEWW, L"", WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL |
                               LVS_REPORT | LVS_OWNERDRAWFIXED | LVS_NOCOLUMNHEADER | LVS_SINGLESEL | LVS_SHOWSELALWAYS,
                               24, 82, 812, 406, window, reinterpret_cast<HMENU>(kIdList), nullptr, nullptr);
        ListView_SetExtendedListViewStyle(g_list, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
        ListView_SetBkColor(g_list, RGB(31, 25, 22));
        ListView_SetTextBkColor(g_list, RGB(31, 25, 22));
        ListView_SetTextColor(g_list, RGB(255, 231, 157));
        LVCOLUMNW cardColumn{};
        cardColumn.mask = LVCF_WIDTH;
        cardColumn.cx = 790;
        ListView_InsertColumn(g_list, 0, &cardColumn);
        g_cardImages = ImageList_Create(1, 104, ILC_COLOR32 | ILC_MASK, 1, 1);
        if (g_cardImages) {
            HBITMAP blank = CreateBitmap(1, 104, 1, 32, nullptr);
            ImageList_AddMasked(g_cardImages, blank, RGB(255, 0, 255));
            DeleteObject(blank);
            ListView_SetImageList(g_list, g_cardImages, LVSIL_SMALL);
        }
        CreateWindowW(L"STATIC", L"Количество:", WS_CHILD | WS_VISIBLE, 24, 505, 105, 24, window, nullptr, nullptr, nullptr);
        g_quantity = CreateWindowW(L"EDIT", L"1", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER | ES_RIGHT, 132, 501, 90, 27, window, reinterpret_cast<HMENU>(kIdQuantity), nullptr, nullptr);
        HWND spin = CreateWindowW(UPDOWN_CLASSW, L"", WS_CHILD | WS_VISIBLE | UDS_ALIGNRIGHT | UDS_SETBUDDYINT | UDS_ARROWKEYS, 0, 0, 0, 0, window, reinterpret_cast<HMENU>(kIdSpin), nullptr, nullptr);
        SendMessageW(spin, UDM_SETBUDDY, reinterpret_cast<WPARAM>(g_quantity), 0); SendMessageW(spin, UDM_SETRANGE32, 1, 32767);
        CreateWindowW(L"BUTTON", L"Все", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 232, 500, 82, 30, window, reinterpret_cast<HMENU>(kIdBuyAll), nullptr, nullptr);
        g_price = CreateWindowW(L"STATIC", L"Выберите существ в списке.", WS_CHILD | WS_VISIBLE, 326, 500, 510, 42, window, reinterpret_cast<HMENU>(kIdPrice), nullptr, nullptr);
        g_send = CreateWindowW(L"BUTTON", L"Отправить", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 596, 558, 112, 36, window, reinterpret_cast<HMENU>(kIdSend), nullptr, nullptr);
        CreateWindowW(L"BUTTON", L"Закрыть", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 724, 558, 112, 36, window, reinterpret_cast<HMENU>(kIdClose), nullptr, nullptr);
#ifdef CARAVAN_PHYSICS
        CreateWindowW(L"STATIC", L"Скорость: 8 клеток в день. Преграда — ожидание.", WS_CHILD | WS_VISIBLE | SS_CENTERIMAGE, 24, 563, 540, 24, window, nullptr, nullptr, nullptr);
#else
        CreateWindowW(L"STATIC", L"Списание только после подтверждения. Доставка мгновенная.", WS_CHILD | WS_VISIBLE | SS_CENTERIMAGE, 24, 563, 540, 24, window, nullptr, nullptr, nullptr);
#endif
        EnumChildWindows(window, ApplyGameFont, 0);
        if (g_title && g_titleFont) SendMessageW(g_title, WM_SETFONT, reinterpret_cast<WPARAM>(g_titleFont), TRUE);
        FillList();
        if (g_rows.empty()) ShowStatus(L"Нет существ в захваченных жилищах или других своих городах.");
        else UpdatePrice();
        SetTimer(window, kOverlayTimer, 150, nullptr);
        return 0;
    }
    case WM_PAINT: {
        PAINTSTRUCT paint{};
        HDC dc = BeginPaint(window, &paint);
        PaintDialog(dc, window);
        EndPaint(window, &paint);
        return 0;
    }
    case WM_ERASEBKGND:
        return 1;
    case WM_CTLCOLORSTATIC: {
        HDC dc = reinterpret_cast<HDC>(wParam);
        SetBkMode(dc, OPAQUE);
        const bool title = reinterpret_cast<HWND>(lParam) == g_title;
        SetBkColor(dc, title ? RGB(111, 24, 25) : RGB(20, 39, 86));
        SetTextColor(dc, reinterpret_cast<HWND>(lParam) == g_title ? RGB(255, 222, 117) : RGB(248, 232, 180));
        HBRUSH brush = title ? g_titleBrush : g_labelBrush;
        return reinterpret_cast<LRESULT>(brush ? brush : GetStockObject(DKGRAY_BRUSH));
    }
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX: {
        HDC dc = reinterpret_cast<HDC>(wParam);
        SetBkColor(dc, RGB(238, 224, 174));
        SetTextColor(dc, RGB(55, 33, 11));
        return reinterpret_cast<LRESULT>(g_parchmentBrush ? g_parchmentBrush : GetStockObject(WHITE_BRUSH));
    }
    case WM_DRAWITEM:
        if (wParam == kIdList) {
            const auto* item = reinterpret_cast<DRAWITEMSTRUCT*>(lParam);
            if (item->itemID != static_cast<UINT>(-1)) {
                LVITEMW listItem{};
                listItem.mask = LVIF_PARAM;
                listItem.iItem = static_cast<int>(item->itemID);
                if (ListView_GetItem(g_list, &listItem))
                    DrawCreatureCard(item->hDC, item->rcItem, static_cast<int>(listItem.lParam),
                                     (item->itemState & ODS_SELECTED) != 0);
            }
            return TRUE;
        }
        if (wParam >= kIdFilterAll && wParam <= kIdFilterGarrisons) {
            DrawFilterButton(reinterpret_cast<DRAWITEMSTRUCT*>(lParam));
            return TRUE;
        }
        if (wParam == kIdSend || wParam == kIdClose || wParam == kIdBuyAll) {
            DrawGoldButton(reinterpret_cast<DRAWITEMSTRUCT*>(lParam));
            return TRUE;
        }
        break;
    case WM_TIMER:
        if (wParam == kOverlayTimer && g_gameWindow) {
            DWORD foregroundProcess = 0;
            GetWindowThreadProcessId(GetForegroundWindow(), &foregroundProcess);
            const bool shouldShow = !IsIconic(g_gameWindow) && foregroundProcess == GetCurrentProcessId();
            if (!shouldShow) ShowWindow(window, SW_HIDE);
            // Repositioning the non-activating parent closes the native
            // ComboBox popup. Leave the overlay still while it is expanded.
            else PositionOverlay();
            return 0;
        }
        break;
    case WM_COMMAND:
        if (LOWORD(wParam) == kIdClose) { DestroyWindow(window); return 0; }
        if (LOWORD(wParam) == kIdSend) { SendCaravan(window); return 0; }
        if (LOWORD(wParam) == kIdBuyAll && HIWORD(wParam) == BN_CLICKED) { SelectMaximumQuantity(); return 0; }
        if (LOWORD(wParam) >= kIdFilterAll && LOWORD(wParam) <= kIdFilterGarrisons && HIWORD(wParam) == BN_CLICKED) {
            g_filterSelection = LOWORD(wParam) - kIdFilterAll;
            CancelConfirmation(); FillList(); UpdatePrice();
            for (HWND button : g_filterButtons) if (button) InvalidateRect(button, nullptr, TRUE);
            return 0;
        }
        if (LOWORD(wParam) == kIdQuantity && HIWORD(wParam) == EN_CHANGE) { CancelConfirmation(); UpdatePrice(); return 0; }
        break;
    case WM_NOTIFY:
        if (reinterpret_cast<NMHDR*>(lParam)->idFrom == kIdList &&
            (reinterpret_cast<NMHDR*>(lParam)->code == LVN_ITEMCHANGED || reinterpret_cast<NMHDR*>(lParam)->code == NM_DBLCLK)) {
            CancelConfirmation(); UpdatePrice();
            if (reinterpret_cast<NMHDR*>(lParam)->code == NM_DBLCLK) SendCaravan(window);
            return 0;
        }
        break;
    case WM_CLOSE: DestroyWindow(window); return 0;
    case WM_DESTROY:
        KillTimer(window, kOverlayTimer);
        if (g_cardImages) { ImageList_Destroy(g_cardImages); g_cardImages = nullptr; }
        g_dialog = nullptr; g_list = nullptr; g_filter = nullptr; g_quantity = nullptr; g_price = nullptr; g_send = nullptr; g_title = nullptr;
        for (HWND& button : g_filterButtons) button = nullptr;
        CancelConfirmation(); return 0;
    }
    return DefWindowProcW(window, message, wParam, lParam);
}

static bool IsOurForegroundWindow(HWND* result) {
    HWND foreground = GetForegroundWindow();
    if (!foreground) return false;
    DWORD process = 0;
    GetWindowThreadProcessId(foreground, &process);
    if (process != GetCurrentProcessId()) return false;
    *result = foreground;
    return true;
}

static uint8_t* CurrentTown() {
    uint8_t* manager = ReadPointer(kTownManagerPointerVa);
    if (!manager || !IsReadable(manager + 0x38, sizeof(void*))) return nullptr;
    uint8_t* town = *reinterpret_cast<uint8_t**>(manager + 0x38);
    if (!town || !IsReadable(town, kTownSize)) return nullptr;
    const int owner = static_cast<int8_t>(town[kTownOwnerOffset]);
    return owner >= 0 && owner <= 7 && PlayerResources(owner) ? town : nullptr;
}

static void ShowDialog(HWND gameWindow, uint8_t* destinationTown) {
    g_destinationTown = destinationTown;
    g_gameWindow = gameWindow;
    CollectRows();
    WNDCLASSEXW cls{};
    cls.cbSize = sizeof(cls); cls.lpfnWndProc = DialogProc; cls.hInstance = reinterpret_cast<HINSTANCE>(&__ImageBase);
    cls.hCursor = LoadCursor(nullptr, IDC_ARROW); cls.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    cls.hbrBackground = nullptr; cls.lpszClassName = L"H3CaravanDialog";
    RegisterClassExW(&cls);
    // OpenGL can paint over native child windows.  A non-activating overlay
    // is composed above the renderer but leaves Heroes as the foreground
    // window, preventing HD Mod's minimize-on-focus-loss behaviour.
    g_dialog = CreateWindowExW(WS_EX_CONTROLPARENT | WS_EX_CLIENTEDGE | WS_EX_TOOLWINDOW | WS_EX_TOPMOST | WS_EX_NOACTIVATE,
                              cls.lpszClassName, L"Караваны — выбор существ",
                              WS_POPUP | WS_BORDER | WS_DLGFRAME | WS_CLIPCHILDREN,
                              0, 0, 780, 520, nullptr, nullptr, cls.hInstance, nullptr);
    if (!g_dialog) { g_destinationTown = nullptr; g_gameWindow = nullptr; return; }
    PositionOverlay(); UpdateWindow(g_dialog);
    MSG msg{};
    while (g_dialog && GetMessageW(&msg, nullptr, 0, 0) > 0) {
        if (!IsDialogMessageW(g_dialog, &msg)) { TranslateMessage(&msg); DispatchMessageW(&msg); }
    }
    g_destinationTown = nullptr; g_gameWindow = nullptr;
}

static DWORD WINAPI Worker(void*) {
    for (int i = 0; i < 300 && !GetModuleHandleA("HotA.dll"); ++i) Sleep(100);
    if (!GetModuleHandleA("HotA.dll")) { Log("HotA.dll was not loaded; caravan UI is inactive."); return 0; }
    Sleep(1500);
    g_moduleBase = reinterpret_cast<uintptr_t>(GetModuleHandleA(nullptr));
    INITCOMMONCONTROLSEX controls{sizeof(controls), ICC_LISTVIEW_CLASSES | ICC_UPDOWN_CLASS};
    InitCommonControlsEx(&controls);
    LoadGameUiResources();
    LoadHotkey();
#ifdef CARAVAN_PHYSICS
    Log("Experimental physical caravan UI loaded. Open a town and press K.");
#else
    Log("Caravan UI loaded. Open a town and press K.");
#endif
    bool wasDown = false;
    for (;;) {
        const bool down = (GetAsyncKeyState(g_hotkey) & 0x8000) != 0;
        if (down && !wasDown && !g_dialog) {
            HWND gameWindow = nullptr;
            uint8_t* town = CurrentTown();
            if (town && IsOurForegroundWindow(&gameWindow)) {
#ifdef CARAVAN_PHYSICS
                InstallGameThreadHook(gameWindow);
#endif
                ShowDialog(gameWindow, town);
            }
        }
        wasDown = down;
#ifdef CARAVAN_PHYSICS
        if (!g_gameThreadHook || !g_physicalGameWindow || !IsWindow(g_physicalGameWindow)) {
            HWND gameWindow = nullptr;
            if (IsOurForegroundWindow(&gameWindow)) InstallGameThreadHook(gameWindow);
        }
        RequestPhysicalCaravanTick();
#endif
        Sleep(50);
    }
}

} // namespace

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(instance);
        HANDLE thread = CreateThread(nullptr, 0, Worker, nullptr, 0, nullptr);
        if (thread) CloseHandle(thread);
    }
    return TRUE;
}
