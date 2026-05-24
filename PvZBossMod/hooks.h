#pragma once
// ============================================================================
// hooks.h — Function Hooking System (Header)
// ============================================================================
//
// This file declares a simple x86 function hooking (detouring) system.
// No external libraries needed — we do everything with the Windows API.
//
// WHAT IS HOOKING?
//   "Hooking" means intercepting a function call so YOUR code runs instead of
//   (or in addition to) the original function. We do this by overwriting the
//   first few bytes of the target function with a JMP instruction that
//   redirects execution to our function.
//
// HOW IT WORKS (the "trampoline" approach):
//   1. We save the first 5+ bytes of the original function (the "stolen bytes")
//   2. We overwrite those bytes with:  JMP <our_function>
//   3. We create a small "trampoline" that:
//      a) Executes the stolen bytes
//      b) JMPs back to original_function + 5 (right after our patch)
//   4. When we want to call the ORIGINAL function, we call the trampoline.
//
//   Visual diagram:
//
//   Original Function:          After Hooking:
//   ┌──────────────┐           ┌──────────────┐
//   │ push ebp      │ ──────>  │ JMP ourFunc   │ ← 5-byte JMP overwrites start
//   │ mov ebp, esp  │          │ ...           │
//   │ sub esp, 0x20 │          │ sub esp, 0x20 │ ← execution resumes here via trampoline
//   │ ...           │          │ ...           │
//   └──────────────┘          └──────────────┘
//
//   Trampoline (allocated separately):
//   ┌──────────────────────────┐
//   │ push ebp                 │ ← stolen bytes (original first instructions)
//   │ mov ebp, esp             │
//   │ JMP original_func + 5   │ ← jump back into the original
//   └──────────────────────────┘
//
// ============================================================================

#ifndef HOOKS_H
#define HOOKS_H

#include <windows.h>
#include <cstdint>

// ============================================================================
// Hook structure — stores all the info we need to manage one hook
// ============================================================================
struct Hook {
    uintptr_t targetAddress;     // Address of the function we're hooking
    uintptr_t detourAddress;     // Address of our replacement function
    uintptr_t trampolineAddress; // Address of the trampoline (calls original)
    BYTE      stolenBytes[16];   // Backup of the original bytes we overwrote
    int       stolenByteCount;   // How many bytes we stole (at least 5)
    bool      isInstalled;       // Is this hook currently active?
};

// ============================================================================
// Public API — Functions you actually call
// ============================================================================

// Install a hook: redirect `targetFunc` to `detourFunc`.
// After calling this, `outTrampoline` will point to a function you can call
// to invoke the ORIGINAL function (before hooking).
//
// Parameters:
//   targetFunc   — address of the function to hook (e.g., Board::Update)
//   detourFunc   — address of YOUR function that will be called instead
//   outTrampoline — [output] address you call to run the original function
//   hook         — [output] Hook struct that stores hook state for later removal
//
// Returns: true on success, false on failure.
bool InstallHook(uintptr_t targetFunc, uintptr_t detourFunc,
                 uintptr_t& outTrampoline, Hook& hook);

// Remove a previously installed hook, restoring the original function.
// Parameters:
//   hook — the Hook struct that was filled in by InstallHook
// Returns: true on success, false if the hook wasn't installed.
bool RemoveHook(Hook& hook);

// ============================================================================
// Game-specific hooks — Board::Update and Board::Draw
// ============================================================================

// These are the addresses of the functions we want to hook.
// Found via reverse engineering the PvZ GOTY executable.

// Board::Update — called every game frame to advance game logic.
// This is where zombies move, plants shoot, etc.
// We hook this to run our boss logic every frame.
constexpr uintptr_t BOARD_UPDATE_ADDR = 0x415D40;

// Board::Draw — called every frame to render the game.
// We hook this to draw our boss HP bar and effects on top of the game.
constexpr uintptr_t BOARD_DRAW_ADDR   = 0x417080;

// Global hook structs — we need these to install/remove hooks cleanly.
extern Hook g_updateHook;
extern Hook g_drawHook;

// Trampoline function pointers — call these to invoke the ORIGINAL functions.
// These are set up by InstallHook() and used inside our detour functions.
extern uintptr_t g_originalUpdateTrampoline;
extern uintptr_t g_originalDrawTrampoline;

// Install all game hooks. Call this after the game has fully loaded.
bool InstallGameHooks();

// Remove all game hooks. Call this on shutdown.
void RemoveGameHooks();

#endif // HOOKS_H
