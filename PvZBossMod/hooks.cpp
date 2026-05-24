// ============================================================================
// hooks.cpp — Function Hooking System (Implementation)
// ============================================================================
//
// This implements the hooking system declared in hooks.h.
// All hooking is done manually using VirtualProtect and raw byte patching.
// No external libraries required!
//
// The key Windows API we use:
//   - VirtualProtect: Changes memory protection so we can write to code pages
//   - VirtualAlloc:   Allocates executable memory for our trampolines
//   - VirtualFree:    Frees trampoline memory when we remove hooks
//
// ============================================================================

#include "hooks.h"
#include "game.h"
#include "boss.h"
#include <cstdio>
#include <cstring>

// ============================================================================
// Global hook state
// ============================================================================

// These store the state of our two hooks so we can remove them later.
Hook g_updateHook = {};
Hook g_drawHook   = {};

// These will point to the trampolines — callable addresses that execute
// the ORIGINAL function code. Set by InstallHook().
uintptr_t g_originalUpdateTrampoline = 0;
uintptr_t g_originalDrawTrampoline   = 0;

// ============================================================================
// x86 Instruction Length Helper
// ============================================================================
// We need to know how long each x86 instruction is so we can steal COMPLETE
// instructions (not cut one in half). This is a SIMPLIFIED decoder — it only
// handles the most common instructions found at function prologues.
//
// A real disassembler would handle hundreds of opcodes, but function prologues
// are very predictable in MSVC-compiled code. They almost always start with:
//   push ebp          (1 byte: 0x55)
//   mov ebp, esp      (2 bytes: 0x8B 0xEC or 0x89 0xE5)
//   sub esp, <imm>    (3 or 6 bytes)
//   push <reg>        (1 byte: 0x50–0x57)
//
// We only need to decode enough to steal at least 5 bytes (size of a JMP).

static int GetInstructionLength(const BYTE* code) {
    // === 1-byte instructions ===
    // PUSH/POP register (0x50–0x5F)
    if (code[0] >= 0x50 && code[0] <= 0x5F) return 1;

    // NOP (0x90)
    if (code[0] == 0x90) return 1;

    // RET (0xC3)
    if (code[0] == 0xC3) return 1;

    // === 2-byte instructions ===
    // MOV reg, reg patterns (0x8B xx or 0x89 xx)
    if (code[0] == 0x8B || code[0] == 0x89) return 2;

    // XOR reg, reg (0x33 xx or 0x31 xx) — common for "xor eax, eax" (set to 0)
    if (code[0] == 0x33 || code[0] == 0x31) return 2;

    // TEST reg, reg (0x85 xx)
    if (code[0] == 0x85) return 2;

    // === 3-byte instructions ===
    // SUB ESP, imm8 (0x83 0xEC xx)
    if (code[0] == 0x83) return 3;

    // LEA with short offset (0x8D with ModR/M)
    if (code[0] == 0x8D) {
        // Simple ModR/M, varies but 2–3 bytes is common for short forms
        BYTE modrm = code[1];
        if ((modrm >> 6) == 1) return 3; // [reg+disp8]
        if ((modrm >> 6) == 0) return 2; // [reg]
        if ((modrm >> 6) == 2) return 6; // [reg+disp32]
        return 2;
    }

    // === 5-byte instructions ===
    // MOV reg, imm32 (0xB8–0xBF xx xx xx xx)
    if (code[0] >= 0xB8 && code[0] <= 0xBF) return 5;

    // CALL rel32 (0xE8 xx xx xx xx)
    if (code[0] == 0xE8) return 5;

    // JMP rel32 (0xE9 xx xx xx xx)
    if (code[0] == 0xE9) return 5;

    // === 6-byte instructions ===
    // SUB ESP, imm32 (0x81 0xEC xx xx xx xx)
    if (code[0] == 0x81) return 6;

    // === 2-byte opcode prefix (0x0F) ===
    if (code[0] == 0x0F) {
        // Jcc rel32 — conditional jumps (0x0F 0x80–0x8F + 4 bytes)
        if (code[1] >= 0x80 && code[1] <= 0x8F) return 6;
        return 3; // Default for other 0F-prefixed instructions
    }

    // === Fallback ===
    // If we don't recognize the instruction, assume 1 byte and hope for the best.
    // In a production mod, you'd use a proper disassembler library here.
    printf("[HOOKS] WARNING: Unknown opcode 0x%02X at %p, assuming 1 byte\n",
           code[0], code);
    return 1;
}

// ============================================================================
// InstallHook — The core hooking function
// ============================================================================
bool InstallHook(uintptr_t targetFunc, uintptr_t detourFunc,
                 uintptr_t& outTrampoline, Hook& hook) {
    
    printf("[HOOKS] Installing hook: target=0x%08X, detour=0x%08X\n",
           targetFunc, detourFunc);

    // ---- Step 1: Figure out how many bytes to steal ----
    // We need at least 5 bytes for our JMP instruction.
    // We must steal COMPLETE instructions, so we might steal more than 5.
    int bytesToSteal = 0;
    const BYTE* codePtr = (const BYTE*)targetFunc;
    
    while (bytesToSteal < 5) {
        int len = GetInstructionLength(codePtr + bytesToSteal);
        bytesToSteal += len;
    }

    printf("[HOOKS]   Stealing %d bytes from target function\n", bytesToSteal);

    // Safety check: don't steal too many bytes (something is probably wrong)
    if (bytesToSteal > 15) {
        printf("[HOOKS]   ERROR: Too many bytes to steal (%d). Aborting.\n", bytesToSteal);
        return false;
    }

    // ---- Step 2: Back up the original bytes ----
    memcpy(hook.stolenBytes, (void*)targetFunc, bytesToSteal);
    hook.stolenByteCount = bytesToSteal;
    hook.targetAddress   = targetFunc;
    hook.detourAddress   = detourFunc;

    // ---- Step 3: Allocate executable memory for the trampoline ----
    // The trampoline needs: stolen bytes + 5 bytes for a JMP back
    BYTE* trampoline = (BYTE*)VirtualAlloc(
        nullptr,                        // Let Windows choose the address
        bytesToSteal + 5,               // Size: stolen bytes + JMP instruction
        MEM_COMMIT | MEM_RESERVE,       // Commit the memory immediately
        PAGE_EXECUTE_READWRITE          // Must be executable (we run code here!)
    );

    if (!trampoline) {
        printf("[HOOKS]   ERROR: VirtualAlloc failed for trampoline! Error=%d\n",
               GetLastError());
        return false;
    }

    // ---- Step 4: Build the trampoline ----
    // Copy the stolen bytes into the trampoline
    memcpy(trampoline, hook.stolenBytes, bytesToSteal);

    // Append a JMP instruction that jumps back into the original function,
    // right AFTER the bytes we stole.
    // JMP rel32 format: 0xE9 [4-byte relative offset]
    // The offset is calculated as: destination - (source + 5)
    //   where source is the address of the JMP instruction in the trampoline
    trampoline[bytesToSteal] = 0xE9; // JMP opcode
    uintptr_t jumpBackDest = targetFunc + bytesToSteal;           // Where to jump to
    uintptr_t jumpBackSrc  = (uintptr_t)(trampoline + bytesToSteal); // Where the JMP is
    int32_t jumpBackOffset = (int32_t)(jumpBackDest - jumpBackSrc - 5);
    memcpy(trampoline + bytesToSteal + 1, &jumpBackOffset, 4);

    hook.trampolineAddress = (uintptr_t)trampoline;
    outTrampoline = (uintptr_t)trampoline;

    printf("[HOOKS]   Trampoline created at 0x%08X\n", (uintptr_t)trampoline);

    // ---- Step 5: Patch the target function with a JMP to our detour ----
    // First, we need to make the target memory writable.
    // Code pages are normally read-only + execute. VirtualProtect lets us
    // temporarily make them writable.
    DWORD oldProtect;
    if (!VirtualProtect((void*)targetFunc, bytesToSteal, PAGE_EXECUTE_READWRITE, &oldProtect)) {
        printf("[HOOKS]   ERROR: VirtualProtect failed! Error=%d\n", GetLastError());
        VirtualFree(trampoline, 0, MEM_RELEASE);
        return false;
    }

    // Write the JMP instruction
    BYTE* target = (BYTE*)targetFunc;
    target[0] = 0xE9; // JMP opcode
    int32_t detourOffset = (int32_t)(detourFunc - targetFunc - 5);
    memcpy(target + 1, &detourOffset, 4);

    // If we stole more than 5 bytes, fill the rest with NOPs (0x90).
    // This is just for cleanliness — those bytes will never execute because
    // the JMP redirects before reaching them.
    for (int i = 5; i < bytesToSteal; i++) {
        target[i] = 0x90; // NOP
    }

    // Restore the original memory protection
    VirtualProtect((void*)targetFunc, bytesToSteal, oldProtect, &oldProtect);

    // Flush the CPU instruction cache — important after modifying code!
    // Without this, the CPU might execute stale cached instructions.
    FlushInstructionCache(GetCurrentProcess(), (void*)targetFunc, bytesToSteal);

    hook.isInstalled = true;
    printf("[HOOKS]   Hook installed successfully!\n");
    return true;
}

// ============================================================================
// RemoveHook — Restore the original function
// ============================================================================
bool RemoveHook(Hook& hook) {
    if (!hook.isInstalled) return false;

    printf("[HOOKS] Removing hook at 0x%08X\n", hook.targetAddress);

    // Make the target writable again
    DWORD oldProtect;
    VirtualProtect((void*)hook.targetAddress, hook.stolenByteCount,
                   PAGE_EXECUTE_READWRITE, &oldProtect);

    // Restore the original stolen bytes
    memcpy((void*)hook.targetAddress, hook.stolenBytes, hook.stolenByteCount);

    // Restore protection
    VirtualProtect((void*)hook.targetAddress, hook.stolenByteCount,
                   oldProtect, &oldProtect);

    // Flush instruction cache
    FlushInstructionCache(GetCurrentProcess(), (void*)hook.targetAddress,
                          hook.stolenByteCount);

    // Free the trampoline memory
    if (hook.trampolineAddress) {
        VirtualFree((void*)hook.trampolineAddress, 0, MEM_RELEASE);
        hook.trampolineAddress = 0;
    }

    hook.isInstalled = false;
    printf("[HOOKS]   Hook removed successfully!\n");
    return true;
}

// ============================================================================
// SECTION: Game-Specific Detour Functions
// ============================================================================
// These are the functions that get called INSTEAD of the originals.
// They run our boss logic and then call the original function via trampoline.

// ---- Our Board::Update detour ----
// This uses __fastcall because Board::Update is a __thiscall function.
// On MSVC x86, __thiscall passes `this` in ECX.
// We emulate this with __fastcall, which passes the first arg in ECX
// and second in EDX (we ignore EDX).
void __fastcall DetourBoardUpdate(void* thisBoard, void* edx) {
    // Call the ORIGINAL Board::Update first, so the game updates normally.
    // We cast the trampoline address to the right function pointer type.
    typedef void(__fastcall* OriginalUpdate)(void* thisBoard, void* edx);
    OriginalUpdate originalFunc = (OriginalUpdate)g_originalUpdateTrampoline;
    originalFunc(thisBoard, edx);

    // Now run our boss logic AFTER the game has updated.
    // This means the boss sees the latest game state each frame.
    if (g_Boss) {
        g_Boss->Update();
    }
}

// ---- Our Board::Draw detour ----
// Board::Draw takes a Graphics* parameter in the game.
// Signature: void __thiscall Board::Draw(Graphics* g)
// We model this as __fastcall with (thisBoard, edx_unused, graphics).
void __fastcall DetourBoardDraw(void* thisBoard, void* edx, void* graphics) {
    // Call the ORIGINAL Board::Draw first, so the game renders normally.
    typedef void(__fastcall* OriginalDraw)(void* thisBoard, void* edx, void* graphics);
    OriginalDraw originalFunc = (OriginalDraw)g_originalDrawTrampoline;
    originalFunc(thisBoard, edx, graphics);

    // Now draw our boss ON TOP of the normal game graphics.
    // We pass the Graphics* pointer in case we want to use it later.
    if (g_Boss) {
        g_Boss->Draw(graphics);
    }
}

// ============================================================================
// InstallGameHooks / RemoveGameHooks — High-level hook management
// ============================================================================

bool InstallGameHooks() {
    printf("[HOOKS] === Installing game hooks ===\n");

    // Hook Board::Update
    bool ok1 = InstallHook(
        BOARD_UPDATE_ADDR,                          // Target: original Board::Update
        (uintptr_t)&DetourBoardUpdate,              // Detour: our replacement
        g_originalUpdateTrampoline,                 // Output: trampoline to call original
        g_updateHook                                // Output: hook state for removal
    );

    // Hook Board::Draw
    bool ok2 = InstallHook(
        BOARD_DRAW_ADDR,                            // Target: original Board::Draw
        (uintptr_t)&DetourBoardDraw,                // Detour: our replacement
        g_originalDrawTrampoline,                   // Output: trampoline to call original
        g_drawHook                                  // Output: hook state for removal
    );

    if (ok1 && ok2) {
        printf("[HOOKS] === All hooks installed successfully! ===\n");
        return true;
    } else {
        printf("[HOOKS] === WARNING: Some hooks failed to install! ===\n");
        return false;
    }
}

void RemoveGameHooks() {
    printf("[HOOKS] === Removing all game hooks ===\n");
    RemoveHook(g_updateHook);
    RemoveHook(g_drawHook);
    printf("[HOOKS] === All hooks removed ===\n");
}
