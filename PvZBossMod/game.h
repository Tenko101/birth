#pragma once
// ============================================================================
// game.h — PvZ 1 (Steam GOTY) Game Structure Definitions
// ============================================================================
//
// This file defines the memory layout of Plants vs. Zombies' internal objects.
// PvZ was built with the PopCap "SexyAppFramework," and all game objects live
// in a hierarchy:  LawnApp  ->  Board  ->  Zombies / Plants / Projectiles
//
// HOW WE FIND THINGS IN MEMORY:
//   1. The game stores a global pointer to the LawnApp object at a fixed
//      address (0x6A9EC0 for Steam GOTY). This NEVER moves.
//   2. From LawnApp, we follow offsets to reach the Board (the actual
//      playing field), and from the Board we reach zombie/plant arrays.
//
// These offsets were found by the PvZ modding community using tools like
// Cheat Engine, IDA Pro, and Ghidra. They are specific to the Steam GOTY
// version of PvZ 1 (the one you buy on Steam today).
//
// IMPORTANT: This is a 32-bit (x86) game. All pointers are 4 bytes.
// ============================================================================

#ifndef GAME_H
#define GAME_H

#include <windows.h>
#include <cstdint>
#include <cstdio>

// ============================================================================
// SECTION 1: Static Addresses
// ============================================================================
// These are addresses in the PvZ executable that never change between runs.
// They point to global objects or contain pointers to global objects.

// The global LawnApp pointer lives at this address in memory.
// LawnApp is the "root" of the entire game — think of it as the game's
// main application object that owns everything else.
constexpr uintptr_t LAWNAPP_STATIC_PTR = 0x6A9EC0;

// ============================================================================
// SECTION 2: Offsets from LawnApp
// ============================================================================
// Once we have the LawnApp pointer, we add these offsets to reach sub-objects.

// Board* is stored at LawnApp + 0x868
// The Board is the actual gameplay screen — the lawn with the grid of plants,
// the zombies walking across, the sun counter, etc.
constexpr uintptr_t OFFSET_LAWNAPP_TO_BOARD = 0x868;

// ============================================================================
// SECTION 3: Offsets from Board
// ============================================================================
// The Board object contains arrays (really DataArrays) of game entities.

// Pointer to the zombie DataArray: Board + 0xA0
// This is a contiguous block of memory holding all active Zombie structs.
constexpr uintptr_t OFFSET_BOARD_ZOMBIE_ARRAY = 0xA0;

// Number of zombies currently in the array: Board + 0xA8
// (DataArray stores count at +8 from the array pointer field)
constexpr uintptr_t OFFSET_BOARD_ZOMBIE_COUNT = 0xA8;

// Pointer to the plant DataArray: Board + 0xC0
// Same idea — contiguous block of Plant structs.
constexpr uintptr_t OFFSET_BOARD_PLANT_ARRAY = 0xC0;

// Number of plants currently in the array: Board + 0xC8
constexpr uintptr_t OFFSET_BOARD_PLANT_COUNT = 0xC8;

// The current game tick / frame counter: Board + 0x5568
// This increments by 1 every game update (~100 updates/sec in PvZ).
constexpr uintptr_t OFFSET_BOARD_GAME_TICK = 0x5568;

// Sun counter (how much sun the player has): Board + 0x5578
constexpr uintptr_t OFFSET_BOARD_SUN_COUNT = 0x5578;

// Board's current row count (usually 5, but varies by level): Board + 0x15C
constexpr uintptr_t OFFSET_BOARD_ROW_COUNT = 0x15C;

// ============================================================================
// SECTION 4: Zombie Structure Layout
// ============================================================================
// Each zombie in the zombie array is a fixed-size struct (~0x168 bytes).
// Here are the important fields and their offsets within a single Zombie.

constexpr int ZOMBIE_STRUCT_SIZE = 0x168; // Total size of one Zombie struct

// Offsets within a single Zombie struct:
constexpr uintptr_t ZOMBIE_TYPE       = 0x24;  // int — which kind of zombie (0=Normal, 1=Flag, etc.)
constexpr uintptr_t ZOMBIE_STATUS     = 0x28;  // int — current state (walking, eating, dying...)
constexpr uintptr_t ZOMBIE_POS_X      = 0x2C;  // float — X position on screen (pixels from left)
constexpr uintptr_t ZOMBIE_POS_Y      = 0x30;  // float — Y position on screen (pixels from top)
constexpr uintptr_t ZOMBIE_HEALTH     = 0xC8;  // int — body hitpoints remaining
constexpr uintptr_t ZOMBIE_MAX_HEALTH = 0xCC;  // int — body hitpoints at full health
constexpr uintptr_t ZOMBIE_HELM_HP    = 0xD0;  // int — helmet/accessory hitpoints (cone, bucket, etc.)
constexpr uintptr_t ZOMBIE_SHIELD_HP  = 0xD4;  // int — shield hitpoints (screen-door, ladder, etc.)
constexpr uintptr_t ZOMBIE_ROW        = 0x1C;  // int — which row the zombie is in (0-4)
constexpr uintptr_t ZOMBIE_DEAD       = 0xEC;  // bool — true if this zombie is dead/inactive
constexpr uintptr_t ZOMBIE_ANIM_SPEED = 0x34;  // float — animation playback speed
constexpr uintptr_t ZOMBIE_VELOCITY_X = 0x38;  // float — how fast the zombie moves left (negative = left)

// ============================================================================
// SECTION 5: Plant Structure Layout
// ============================================================================
// Each plant in the plant array is a fixed-size struct (~0x14C bytes).

constexpr int PLANT_STRUCT_SIZE = 0x14C; // Total size of one Plant struct

// Offsets within a single Plant struct:
constexpr uintptr_t PLANT_TYPE       = 0x24;  // int — which kind of plant (0=Peashooter, etc.)
constexpr uintptr_t PLANT_ROW        = 0x1C;  // int — which row the plant is in (0-4)
constexpr uintptr_t PLANT_COL        = 0x28;  // int — which column the plant is in (0-8)
constexpr uintptr_t PLANT_POS_X      = 0x2C;  // float — X position on screen
constexpr uintptr_t PLANT_POS_Y      = 0x30;  // float — Y position on screen
constexpr uintptr_t PLANT_HEALTH     = 0x40;  // int — hitpoints remaining
constexpr uintptr_t PLANT_MAX_HEALTH = 0x44;  // int — max hitpoints
constexpr uintptr_t PLANT_DEAD       = 0x141; // bool — true if this plant is dead/inactive
constexpr uintptr_t PLANT_SQUISHED   = 0x142; // bool — true if the plant was squished
constexpr uintptr_t PLANT_VISIBLE    = 0x18;  // bool — is the plant visible?

// ============================================================================
// SECTION 6: Zombie Type IDs
// ============================================================================
// These are the integer values stored at Zombie + ZOMBIE_TYPE.
// The game uses these to determine zombie behavior, appearance, etc.

enum ZombieType : int {
    ZOMBIE_NORMAL      = 0,   // Basic zombie — your everyday undead neighbor
    ZOMBIE_FLAG        = 1,   // Flag zombie — signals a big wave
    ZOMBIE_CONEHEAD    = 2,   // Conehead — tougher with a traffic cone
    ZOMBIE_POLEVAULT   = 3,   // Pole Vaulting zombie — jumps over first plant
    ZOMBIE_BUCKETHEAD  = 4,   // Buckethead — very tough with a metal bucket
    ZOMBIE_NEWSPAPER   = 5,   // Newspaper zombie — gets angry when paper destroyed
    ZOMBIE_SCREENDOOR  = 6,   // Screen Door zombie — has a shield
    ZOMBIE_FOOTBALL    = 7,   // Football zombie — fast and tough
    ZOMBIE_DANCING     = 8,   // Dancing zombie — summons backup dancers
    ZOMBIE_BACKUP      = 9,   // Backup Dancer — summoned by Dancing zombie
    ZOMBIE_DUCKY_TUBE  = 10,  // Ducky Tube zombie — swims in pool levels
    ZOMBIE_SNORKEL     = 11,  // Snorkel zombie — dives underwater
    ZOMBIE_ZOMBONI     = 12,  // Zomboni — drives an ice machine
    ZOMBIE_DOLPHIN     = 14,  // Dolphin Rider zombie
    ZOMBIE_JACK        = 15,  // Jack-in-the-Box zombie — explodes!
    ZOMBIE_BALLOON     = 16,  // Balloon zombie — floats over plants
    ZOMBIE_DIGGER      = 17,  // Digger zombie — tunnels underground
    ZOMBIE_POGO        = 18,  // Pogo zombie — bounces over plants
    ZOMBIE_YETI        = 19,  // Yeti zombie — rare special zombie
    ZOMBIE_BUNGEE      = 20,  // Bungee zombie — drops from the sky
    ZOMBIE_LADDER      = 21,  // Ladder zombie — places ladders on walls
    ZOMBIE_CATAPULT    = 22,  // Catapult zombie — throws basketballs
    ZOMBIE_GARGANTUAR  = 23,  // Gargantuar — massive zombie, very tough
    ZOMBIE_IMP         = 24,  // Imp — thrown by Gargantuar
    ZOMBIE_GIGA_GARG   = 32,  // Giga-Gargantuar — even tougher Gargantuar
};

// ============================================================================
// SECTION 7: Plant Type IDs
// ============================================================================

enum PlantType : int {
    PLANT_PEASHOOTER    = 0,
    PLANT_SUNFLOWER     = 1,
    PLANT_CHERRY_BOMB   = 2,
    PLANT_WALLNUT       = 3,
    PLANT_POTATO_MINE   = 4,
    PLANT_SNOW_PEA      = 5,
    PLANT_CHOMPER       = 6,
    PLANT_REPEATER      = 7,
    PLANT_PUFFSHROOM    = 8,
    PLANT_SUNSHROOM     = 9,
    PLANT_FUMESHROOM    = 10,
    PLANT_GRAVE_BUSTER  = 11,
    PLANT_HYPNOSHROOM   = 12,
    PLANT_SCAREDY       = 13,
    PLANT_ICESHROOM     = 14,
    PLANT_DOOMSHROOM    = 15,
    PLANT_LILY_PAD      = 16,
    PLANT_SQUASH        = 17,
    PLANT_THREEPEATER   = 18,
    PLANT_TANGLEKELP    = 19,
    PLANT_JALAPENO      = 20,
    PLANT_SPIKEWEED     = 21,
    PLANT_TORCHWOOD     = 22,
    PLANT_TALLNUT       = 23,
    PLANT_SEASHROOM     = 24,
    PLANT_PLANTERN      = 25,
    PLANT_CACTUS        = 26,
    PLANT_BLOVER        = 27,
    PLANT_SPLIT_PEA     = 28,
    PLANT_STARFRUIT     = 29,
    PLANT_PUMPKIN       = 30,
    PLANT_MAGNETSHROOM  = 31,
    PLANT_CABBAGE_PULT  = 32,
    PLANT_FLOWER_POT    = 33,
    PLANT_KERNEL_PULT   = 34,
    PLANT_COFFEE_BEAN   = 35,
    PLANT_GARLIC        = 36,
    PLANT_UMBRELLA_LEAF = 37,
    PLANT_MARIGOLD      = 38,
    PLANT_MELON_PULT    = 39,
    PLANT_GATLING_PEA   = 40,
    PLANT_TWIN_SUNFLOWER = 41,
    PLANT_GLOOMSHROOM   = 42,
    PLANT_CATTAIL       = 43,
    PLANT_WINTER_MELON  = 44,
    PLANT_GOLD_MAGNET   = 45,
    PLANT_SPIKEROCK     = 46,
    PLANT_COB_CANNON    = 47,
};

// ============================================================================
// SECTION 8: Screen Layout Constants
// ============================================================================
// PvZ 1 runs at a fixed 800x600 resolution.

constexpr int SCREEN_WIDTH  = 800;
constexpr int SCREEN_HEIGHT = 600;

// The lawn grid starts at approximately these pixel coordinates:
constexpr int LAWN_LEFT_X   = 80;   // Left edge of column 0
constexpr int LAWN_TOP_Y    = 80;   // Top edge of row 0
constexpr int CELL_WIDTH    = 80;   // Width of each grid cell in pixels
constexpr int CELL_HEIGHT   = 100;  // Height of each grid cell in pixels
constexpr int NUM_COLUMNS   = 9;    // 9 columns (0–8) on a standard lawn
constexpr int NUM_ROWS      = 5;    // 5 rows (0–4) on a standard day level

// ============================================================================
// SECTION 9: Helper Functions — Reading Game Pointers
// ============================================================================
// These functions follow the pointer chain to reach game objects.
// They return nullptr if the chain is broken (e.g., game not loaded yet).

// Read a pointer stored at `address` in the game's memory.
// Since we're injected into the same process, we can just dereference directly!
inline uintptr_t ReadPointer(uintptr_t address) {
    // Safety check: make sure the address isn't null or obviously invalid
    if (address < 0x10000) return 0;    // Addresses below 64KB are never valid on Windows
    return *(uintptr_t*)address;        // Dereference: read the 4-byte value at `address`
}

// Get a pointer to the LawnApp object (the game's root application object).
// Returns nullptr (0) if the game isn't loaded yet.
inline uintptr_t GetLawnApp() {
    // The static pointer at LAWNAPP_STATIC_PTR holds the address of LawnApp.
    uintptr_t lawnApp = ReadPointer(LAWNAPP_STATIC_PTR);
    return lawnApp;
}

// Get a pointer to the Board object (the gameplay screen).
// Returns 0 if we're not in a game (e.g., on the main menu).
inline uintptr_t GetBoard() {
    uintptr_t lawnApp = GetLawnApp();
    if (!lawnApp) return 0;

    // Board is a pointer stored at LawnApp + 0x868
    uintptr_t board = ReadPointer(lawnApp + OFFSET_LAWNAPP_TO_BOARD);
    return board;
}

// Get the base address of the zombie array and the current zombie count.
// Returns false if the data isn't available.
inline bool GetZombieArray(uintptr_t& outArrayBase, int& outCount) {
    uintptr_t board = GetBoard();
    if (!board) return false;

    outArrayBase = ReadPointer(board + OFFSET_BOARD_ZOMBIE_ARRAY);
    outCount     = *(int*)(board + OFFSET_BOARD_ZOMBIE_COUNT);
    return (outArrayBase != 0);
}

// Get the base address of the plant array and the current plant count.
// Returns false if the data isn't available.
inline bool GetPlantArray(uintptr_t& outArrayBase, int& outCount) {
    uintptr_t board = GetBoard();
    if (!board) return false;

    outArrayBase = ReadPointer(board + OFFSET_BOARD_PLANT_ARRAY);
    outCount     = *(int*)(board + OFFSET_BOARD_PLANT_COUNT);
    return (outArrayBase != 0);
}

// Get the current game tick (frame counter) from the Board.
// Returns 0 if the board isn't available.
inline int GetGameTick() {
    uintptr_t board = GetBoard();
    if (!board) return 0;
    return *(int*)(board + OFFSET_BOARD_GAME_TICK);
}

// Get the player's current sun count.
inline int GetSunCount() {
    uintptr_t board = GetBoard();
    if (!board) return 0;
    return *(int*)(board + OFFSET_BOARD_SUN_COUNT);
}

// Set the player's sun count to a specific value.
inline void SetSunCount(int amount) {
    uintptr_t board = GetBoard();
    if (!board) return;
    *(int*)(board + OFFSET_BOARD_SUN_COUNT) = amount;
}

// ============================================================================
// SECTION 10: Zombie Spawning
// ============================================================================
// To spawn a zombie, we call the game's own internal function.
// The function address was found via reverse engineering.
//
// Board::SpawnZombie is approximately at this address in the GOTY exe.
// Signature:  Zombie* __thiscall Board::SpawnZombie(ZombieType type, int row, ...)
//
// ALTERNATIVE APPROACH (used here): We can also write zombie spawn data into
// the game's zombie spawn queue. This is safer and doesn't require calling
// unknown functions. We'll use a function pointer approach with the known
// address of the spawn function.

// Address of Board::ZombieSpawnFromList or similar internal spawn function.
// NOTE: This address may vary slightly between PvZ builds. 
// The address below is a commonly known one for Steam GOTY.
// If zombies don't spawn, this address may need adjusting via disassembly.
constexpr uintptr_t BOARD_SPAWN_ZOMBIE_ADDR = 0x42A0F0;

// Function pointer type for the zombie spawn function.
// __thiscall means the first hidden parameter is `this` (the Board pointer),
// passed via the ECX register on x86.
// We model this as __fastcall with an unused EDX parameter.
//
// Parameters:
//   ecx = Board* this
//   edx = unused (placeholder for __fastcall convention)
//   type = ZombieType enum
//   row  = row to spawn in (0–4), or -1 for random
typedef void* (__fastcall* SpawnZombieFunc)(void* thisBoard, void* edx, int type, int row);

// Spawn a zombie on the board.
// Returns a pointer to the newly created Zombie struct, or nullptr on failure.
inline void* SpawnZombie(int zombieType, int row) {
    uintptr_t board = GetBoard();
    if (!board) return nullptr;

    // Cast the known address to our function pointer type and call it
    SpawnZombieFunc spawnFunc = (SpawnZombieFunc)BOARD_SPAWN_ZOMBIE_ADDR;
    return spawnFunc((void*)board, nullptr, zombieType, row);
}

#endif // GAME_H
