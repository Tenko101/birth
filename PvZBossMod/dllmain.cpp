// ============================================================================
// dllmain.cpp — DLL Entry Point & version.dll Proxy
// ============================================================================
//
// This is the entry point for our mod DLL. It does three things:
//
//   1. VERSION.DLL PROXY:
//      We disguise our mod as "version.dll" so Windows loads it automatically
//      when PvZ starts (DLL search order hijacking). We forward all real
//      version.dll calls to the system's actual version.dll so nothing breaks.
//
//   2. INITIALIZATION:
//      When our DLL loads (DLL_PROCESS_ATTACH), we start a background thread
//      that waits for the game to fully initialize, then installs our hooks
//      and creates the boss.
//
//   3. CLEANUP:
//      When the game exits (DLL_PROCESS_DETACH), we remove hooks, delete the
//      boss, and free the console.
//
// HOW DLL SEARCH ORDER HIJACKING WORKS:
//   Windows looks for DLLs in this order:
//     1. The application's directory (where PvZ.exe lives)
//     2. The system directory (C:\Windows\System32)
//     3. Other system paths
//   By placing our "version.dll" in the game's folder, Windows loads OURS
//   first. We then load the REAL version.dll from System32 and forward
//   all legitimate calls to it. Sneaky but effective!
//
// ============================================================================

#include <windows.h>
#include <cstdio>
#include "game.h"
#include "hooks.h"
#include "boss.h"

// ============================================================================
// SECTION 1: version.dll Proxy — Loading the Real DLL
// ============================================================================

// Handle to the REAL version.dll (loaded from System32)
static HMODULE g_realVersionDll = nullptr;

// Load the real version.dll from the Windows System32 directory.
// This is called when our DLL first loads.
static bool LoadRealVersionDll() {
    // Build the path to the real version.dll
    // e.g., "C:\Windows\System32\version.dll"
    char systemDir[MAX_PATH];
    GetSystemDirectoryA(systemDir, MAX_PATH);

    char realDllPath[MAX_PATH];
    sprintf_s(realDllPath, "%s\\version.dll", systemDir);

    // Load it!
    g_realVersionDll = LoadLibraryA(realDllPath);
    if (!g_realVersionDll) {
        printf("[PROXY] ERROR: Failed to load real version.dll from: %s\n", realDllPath);
        printf("[PROXY] Error code: %d\n", GetLastError());
        return false;
    }

    printf("[PROXY] Loaded real version.dll from: %s\n", realDllPath);
    return true;
}

// ============================================================================
// SECTION 2: version.dll Export Forwarding
// ============================================================================
// These functions are the exports that version.dll is supposed to provide.
// We load the REAL function address from the real DLL and call it.
//
// The version.def file tells the linker about these exports, but we also
// need the actual function bodies to forward calls properly.
//
// We use a helper macro to reduce repetitive code:

// Helper: Get a function pointer from the real version.dll and call it.
// If the real DLL isn't loaded, we return a failure value.
#define FORWARD_TO_REAL(funcName, retType, failRet, ...) \
    typedef retType (WINAPI* funcName##_t)(__VA_ARGS__); \
    static funcName##_t real_##funcName = nullptr; \
    if (!real_##funcName && g_realVersionDll) { \
        real_##funcName = (funcName##_t)GetProcAddress(g_realVersionDll, #funcName); \
    } \
    if (real_##funcName)

// --- Export: GetFileVersionInfoA ---
extern "C" __declspec(dllexport)
BOOL WINAPI versionProxy_GetFileVersionInfoA(LPCSTR lptstrFilename, DWORD dwHandle,
                                              DWORD dwLen, LPVOID lpData) {
    typedef BOOL(WINAPI* Func)(LPCSTR, DWORD, DWORD, LPVOID);
    static Func realFunc = nullptr;
    if (!realFunc && g_realVersionDll)
        realFunc = (Func)GetProcAddress(g_realVersionDll, "GetFileVersionInfoA");
    return realFunc ? realFunc(lptstrFilename, dwHandle, dwLen, lpData) : FALSE;
}

// --- Export: GetFileVersionInfoW ---
extern "C" __declspec(dllexport)
BOOL WINAPI versionProxy_GetFileVersionInfoW(LPCWSTR lptstrFilename, DWORD dwHandle,
                                              DWORD dwLen, LPVOID lpData) {
    typedef BOOL(WINAPI* Func)(LPCWSTR, DWORD, DWORD, LPVOID);
    static Func realFunc = nullptr;
    if (!realFunc && g_realVersionDll)
        realFunc = (Func)GetProcAddress(g_realVersionDll, "GetFileVersionInfoW");
    return realFunc ? realFunc(lptstrFilename, dwHandle, dwLen, lpData) : FALSE;
}

// --- Export: GetFileVersionInfoSizeA ---
extern "C" __declspec(dllexport)
DWORD WINAPI versionProxy_GetFileVersionInfoSizeA(LPCSTR lptstrFilename, LPDWORD lpdwHandle) {
    typedef DWORD(WINAPI* Func)(LPCSTR, LPDWORD);
    static Func realFunc = nullptr;
    if (!realFunc && g_realVersionDll)
        realFunc = (Func)GetProcAddress(g_realVersionDll, "GetFileVersionInfoSizeA");
    return realFunc ? realFunc(lptstrFilename, lpdwHandle) : 0;
}

// --- Export: GetFileVersionInfoSizeW ---
extern "C" __declspec(dllexport)
DWORD WINAPI versionProxy_GetFileVersionInfoSizeW(LPCWSTR lptstrFilename, LPDWORD lpdwHandle) {
    typedef DWORD(WINAPI* Func)(LPCWSTR, LPDWORD);
    static Func realFunc = nullptr;
    if (!realFunc && g_realVersionDll)
        realFunc = (Func)GetProcAddress(g_realVersionDll, "GetFileVersionInfoSizeW");
    return realFunc ? realFunc(lptstrFilename, lpdwHandle) : 0;
}

// --- Export: VerQueryValueA ---
extern "C" __declspec(dllexport)
BOOL WINAPI versionProxy_VerQueryValueA(LPCVOID pBlock, LPCSTR lpSubBlock,
                                         LPVOID* lplpBuffer, PUINT puLen) {
    typedef BOOL(WINAPI* Func)(LPCVOID, LPCSTR, LPVOID*, PUINT);
    static Func realFunc = nullptr;
    if (!realFunc && g_realVersionDll)
        realFunc = (Func)GetProcAddress(g_realVersionDll, "VerQueryValueA");
    return realFunc ? realFunc(pBlock, lpSubBlock, lplpBuffer, puLen) : FALSE;
}

// --- Export: VerQueryValueW ---
extern "C" __declspec(dllexport)
BOOL WINAPI versionProxy_VerQueryValueW(LPCVOID pBlock, LPCWSTR lpSubBlock,
                                         LPVOID* lplpBuffer, PUINT puLen) {
    typedef BOOL(WINAPI* Func)(LPCVOID, LPCWSTR, LPVOID*, PUINT);
    static Func realFunc = nullptr;
    if (!realFunc && g_realVersionDll)
        realFunc = (Func)GetProcAddress(g_realVersionDll, "VerQueryValueW");
    return realFunc ? realFunc(pBlock, lpSubBlock, lplpBuffer, puLen) : FALSE;
}

// --- Export: GetFileVersionInfoExA ---
extern "C" __declspec(dllexport)
BOOL WINAPI versionProxy_GetFileVersionInfoExA(DWORD dwFlags, LPCSTR lpwstrFilename,
                                                DWORD dwHandle, DWORD dwLen, LPVOID lpData) {
    typedef BOOL(WINAPI* Func)(DWORD, LPCSTR, DWORD, DWORD, LPVOID);
    static Func realFunc = nullptr;
    if (!realFunc && g_realVersionDll)
        realFunc = (Func)GetProcAddress(g_realVersionDll, "GetFileVersionInfoExA");
    return realFunc ? realFunc(dwFlags, lpwstrFilename, dwHandle, dwLen, lpData) : FALSE;
}

// --- Export: GetFileVersionInfoExW ---
extern "C" __declspec(dllexport)
BOOL WINAPI versionProxy_GetFileVersionInfoExW(DWORD dwFlags, LPCWSTR lpwstrFilename,
                                                DWORD dwHandle, DWORD dwLen, LPVOID lpData) {
    typedef BOOL(WINAPI* Func)(DWORD, LPCWSTR, DWORD, DWORD, LPVOID);
    static Func realFunc = nullptr;
    if (!realFunc && g_realVersionDll)
        realFunc = (Func)GetProcAddress(g_realVersionDll, "GetFileVersionInfoExW");
    return realFunc ? realFunc(dwFlags, lpwstrFilename, dwHandle, dwLen, lpData) : FALSE;
}

// --- Export: GetFileVersionInfoSizeExA ---
extern "C" __declspec(dllexport)
DWORD WINAPI versionProxy_GetFileVersionInfoSizeExA(DWORD dwFlags, LPCSTR lpwstrFilename,
                                                     LPDWORD lpdwHandle) {
    typedef DWORD(WINAPI* Func)(DWORD, LPCSTR, LPDWORD);
    static Func realFunc = nullptr;
    if (!realFunc && g_realVersionDll)
        realFunc = (Func)GetProcAddress(g_realVersionDll, "GetFileVersionInfoSizeExA");
    return realFunc ? realFunc(dwFlags, lpwstrFilename, lpdwHandle) : 0;
}

// --- Export: GetFileVersionInfoSizeExW ---
extern "C" __declspec(dllexport)
DWORD WINAPI versionProxy_GetFileVersionInfoSizeExW(DWORD dwFlags, LPCWSTR lpwstrFilename,
                                                     LPDWORD lpdwHandle) {
    typedef DWORD(WINAPI* Func)(DWORD, LPCWSTR, LPDWORD);
    static Func realFunc = nullptr;
    if (!realFunc && g_realVersionDll)
        realFunc = (Func)GetProcAddress(g_realVersionDll, "GetFileVersionInfoSizeExW");
    return realFunc ? realFunc(dwFlags, lpwstrFilename, lpdwHandle) : 0;
}

// --- Export: GetFileVersionInfoByHandle ---
extern "C" __declspec(dllexport)
BOOL WINAPI versionProxy_GetFileVersionInfoByHandle(DWORD dwFlags, HANDLE hFile,
                                                     DWORD dwLen, LPVOID lpData) {
    typedef BOOL(WINAPI* Func)(DWORD, HANDLE, DWORD, LPVOID);
    static Func realFunc = nullptr;
    if (!realFunc && g_realVersionDll)
        realFunc = (Func)GetProcAddress(g_realVersionDll, "GetFileVersionInfoByHandle");
    return realFunc ? realFunc(dwFlags, hFile, dwLen, lpData) : FALSE;
}

// --- Export: VerFindFileA ---
extern "C" __declspec(dllexport)
DWORD WINAPI versionProxy_VerFindFileA(DWORD uFlags, LPCSTR szFileName,
                                        LPCSTR szWinDir, LPCSTR szAppDir,
                                        LPSTR szCurDir, PUINT puCurDirLen,
                                        LPSTR szDestDir, PUINT puDestDirLen) {
    typedef DWORD(WINAPI* Func)(DWORD, LPCSTR, LPCSTR, LPCSTR, LPSTR, PUINT, LPSTR, PUINT);
    static Func realFunc = nullptr;
    if (!realFunc && g_realVersionDll)
        realFunc = (Func)GetProcAddress(g_realVersionDll, "VerFindFileA");
    return realFunc ? realFunc(uFlags, szFileName, szWinDir, szAppDir,
                               szCurDir, puCurDirLen, szDestDir, puDestDirLen) : 0;
}

// --- Export: VerFindFileW ---
extern "C" __declspec(dllexport)
DWORD WINAPI versionProxy_VerFindFileW(DWORD uFlags, LPCWSTR szFileName,
                                        LPCWSTR szWinDir, LPCWSTR szAppDir,
                                        LPWSTR szCurDir, PUINT puCurDirLen,
                                        LPWSTR szDestDir, PUINT puDestDirLen) {
    typedef DWORD(WINAPI* Func)(DWORD, LPCWSTR, LPCWSTR, LPCWSTR, LPWSTR, PUINT, LPWSTR, PUINT);
    static Func realFunc = nullptr;
    if (!realFunc && g_realVersionDll)
        realFunc = (Func)GetProcAddress(g_realVersionDll, "VerFindFileW");
    return realFunc ? realFunc(uFlags, szFileName, szWinDir, szAppDir,
                               szCurDir, puCurDirLen, szDestDir, puDestDirLen) : 0;
}

// --- Export: VerInstallFileA ---
extern "C" __declspec(dllexport)
DWORD WINAPI versionProxy_VerInstallFileA(DWORD uFlags, LPCSTR szSrcFileName,
                                           LPCSTR szDestFileName, LPCSTR szSrcDir,
                                           LPCSTR szDestDir, LPCSTR szCurDir,
                                           LPSTR szTmpFile, PUINT puTmpFileLen) {
    typedef DWORD(WINAPI* Func)(DWORD, LPCSTR, LPCSTR, LPCSTR, LPCSTR, LPCSTR, LPSTR, PUINT);
    static Func realFunc = nullptr;
    if (!realFunc && g_realVersionDll)
        realFunc = (Func)GetProcAddress(g_realVersionDll, "VerInstallFileA");
    return realFunc ? realFunc(uFlags, szSrcFileName, szDestFileName, szSrcDir,
                               szDestDir, szCurDir, szTmpFile, puTmpFileLen) : 0;
}

// --- Export: VerInstallFileW ---
extern "C" __declspec(dllexport)
DWORD WINAPI versionProxy_VerInstallFileW(DWORD uFlags, LPCWSTR szSrcFileName,
                                           LPCWSTR szDestFileName, LPCWSTR szSrcDir,
                                           LPCWSTR szDestDir, LPCWSTR szCurDir,
                                           LPWSTR szTmpFile, PUINT puTmpFileLen) {
    typedef DWORD(WINAPI* Func)(DWORD, LPCWSTR, LPCWSTR, LPCWSTR, LPCWSTR, LPCWSTR, LPWSTR, PUINT);
    static Func realFunc = nullptr;
    if (!realFunc && g_realVersionDll)
        realFunc = (Func)GetProcAddress(g_realVersionDll, "VerInstallFileW");
    return realFunc ? realFunc(uFlags, szSrcFileName, szDestFileName, szSrcDir,
                               szDestDir, szCurDir, szTmpFile, puTmpFileLen) : 0;
}

// --- Export: VerLanguageNameA ---
extern "C" __declspec(dllexport)
DWORD WINAPI versionProxy_VerLanguageNameA(DWORD wLang, LPSTR szLang, DWORD cchLang) {
    typedef DWORD(WINAPI* Func)(DWORD, LPSTR, DWORD);
    static Func realFunc = nullptr;
    if (!realFunc && g_realVersionDll)
        realFunc = (Func)GetProcAddress(g_realVersionDll, "VerLanguageNameA");
    return realFunc ? realFunc(wLang, szLang, cchLang) : 0;
}

// --- Export: VerLanguageNameW ---
extern "C" __declspec(dllexport)
DWORD WINAPI versionProxy_VerLanguageNameW(DWORD wLang, LPWSTR szLang, DWORD cchLang) {
    typedef DWORD(WINAPI* Func)(DWORD, LPWSTR, DWORD);
    static Func realFunc = nullptr;
    if (!realFunc && g_realVersionDll)
        realFunc = (Func)GetProcAddress(g_realVersionDll, "VerLanguageNameW");
    return realFunc ? realFunc(wLang, szLang, cchLang) : 0;
}

// ============================================================================
// SECTION 3: Console Setup — So we can see debug output
// ============================================================================

static bool g_consoleAllocated = false;

static void AllocateDebugConsole() {
    // Create a new console window attached to our process.
    // PvZ doesn't have a console, so we make one for debug output.
    if (AllocConsole()) {
        g_consoleAllocated = true;

        // Redirect stdout/stderr to our new console so printf() works.
        // freopen_s is the safe version of freopen (MSVC-specific).
        FILE* fp = nullptr;
        freopen_s(&fp, "CONOUT$", "w", stdout);  // printf goes to console
        freopen_s(&fp, "CONOUT$", "w", stderr);  // fprintf(stderr, ...) too

        // Set the console title so we know which console is ours
        SetConsoleTitleA("PvZ Boss Mod — The Roaring Knight [Debug Console]");

        // Make the console a nice size
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        SMALL_RECT windowSize = { 0, 0, 99, 30 }; // 100 columns, 31 rows
        SetConsoleWindowInfo(hConsole, TRUE, &windowSize);
    }
}

static void FreeDebugConsole() {
    if (g_consoleAllocated) {
        FreeConsole();
        g_consoleAllocated = false;
    }
}

// ============================================================================
// SECTION 4: Mod Initialization Thread
// ============================================================================
// We run initialization on a separate thread because:
//   1. DllMain has restrictions (no LoadLibrary, no blocking calls, etc.)
//   2. We need to wait for the game to fully load before installing hooks
//   3. If something goes wrong, we don't want to crash the DLL load process
//

static HANDLE g_modThread = nullptr;
static bool   g_shouldShutdown = false;

static DWORD WINAPI ModMainThread(LPVOID param) {
    // ---- Step 1: Open debug console ----
    AllocateDebugConsole();

    printf("============================================================\n");
    printf("   PvZ Boss Mod — THE ROARING KNIGHT\n");
    printf("   Inspired by the Roaring Knight from Deltarune\n");
    printf("   version.dll proxy loaded successfully!\n");
    printf("============================================================\n");
    printf("\n");
    printf("[INIT] Waiting for PvZ to fully load...\n");

    // ---- Step 2: Wait for the game to initialize ----
    // We poll for the LawnApp pointer to become valid.
    // This means the game's main application object has been created.
    int waitAttempts = 0;
    while (!g_shouldShutdown) {
        uintptr_t lawnApp = GetLawnApp();
        if (lawnApp != 0) {
            printf("[INIT] LawnApp found at 0x%08X! Game is loaded.\n", lawnApp);
            break;
        }

        waitAttempts++;
        if (waitAttempts % 20 == 0) {  // Print a dot every 2 seconds
            printf("[INIT] Still waiting... (attempt %d)\n", waitAttempts);
        }
        Sleep(100); // Check every 100ms
    }

    if (g_shouldShutdown) return 0;

    // ---- Step 3: Wait for the Board (gameplay screen) to load ----
    // The Board only exists when the player is actually in a level.
    // It's null on the main menu, level select, etc.
    printf("[INIT] Waiting for player to enter a level (Board to load)...\n");

    while (!g_shouldShutdown) {
        uintptr_t board = GetBoard();
        if (board != 0) {
            printf("[INIT] Board found at 0x%08X! Player is in a level.\n", board);
            break;
        }
        Sleep(200); // Check every 200ms — no rush, player is navigating menus
    }

    if (g_shouldShutdown) return 0;

    // Small extra delay to let the board fully initialize
    Sleep(500);

    // ---- Step 4: Install hooks ----
    printf("[INIT] Installing hooks...\n");
    if (!InstallGameHooks()) {
        printf("[INIT] ERROR: Failed to install hooks! Boss mod will not work.\n");
        printf("[INIT] The game will continue normally without the mod.\n");
        return 1;
    }

    // ---- Step 5: Create the boss ----
    printf("[INIT] Creating The Roaring Knight boss...\n");
    g_Boss = new CustomBoss();
    printf("[INIT] Boss created! The fight begins!\n");
    printf("\n");
    printf("============================================================\n");
    printf("  BOSS FIGHT ACTIVE — Good luck, gardener!\n");
    printf("  Plant your defenses wisely!\n");
    printf("============================================================\n");

    // ---- Step 6: Keep the thread alive ----
    // The thread stays alive so we can monitor the game state.
    // The actual boss logic runs in the hooked Board::Update function.
    while (!g_shouldShutdown) {
        // Periodically check if the Board is still valid
        // (player might exit the level to the main menu)
        uintptr_t board = GetBoard();
        if (board == 0 && g_Boss) {
            printf("[INIT] Board lost! Player exited the level.\n");
            printf("[INIT] Boss will pause until a new level is started.\n");
        }

        Sleep(1000); // Check once per second
    }

    return 0;
}

// ============================================================================
// SECTION 5: DllMain — The DLL Entry Point
// ============================================================================
// Windows calls this function when our DLL is loaded or unloaded.
// DLL_PROCESS_ATTACH = DLL was just loaded into the process
// DLL_PROCESS_DETACH = DLL is about to be unloaded (game exiting)
// DLL_THREAD_ATTACH/DETACH = new thread created/destroyed (we ignore these)

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID reserved) {
    switch (reason) {

        case DLL_PROCESS_ATTACH: {
            // ---- DLL just loaded ----
            
            // Optimization: tell Windows not to call DllMain for thread
            // attach/detach events (we don't need them)
            DisableThreadLibraryCalls(hModule);

            // Load the real version.dll from System32
            // We do this FIRST because the game might call version.dll
            // functions during startup.
            LoadRealVersionDll();

            // Start our mod thread
            // We can't do heavy initialization here in DllMain because
            // Windows holds the "loader lock" during DllMain, which prevents
            // us from calling certain APIs (like LoadLibrary).
            g_shouldShutdown = false;
            g_modThread = CreateThread(
                nullptr,       // Default security attributes
                0,             // Default stack size
                ModMainThread, // Thread function
                nullptr,       // Parameter (none)
                0,             // Start immediately
                nullptr        // Don't need the thread ID
            );

            break;
        }

        case DLL_PROCESS_DETACH: {
            // ---- DLL about to unload (game exiting) ----
            printf("[SHUTDOWN] Game is exiting. Cleaning up...\n");

            // Signal the mod thread to stop
            g_shouldShutdown = true;

            // Wait for the mod thread to finish (up to 3 seconds)
            if (g_modThread) {
                WaitForSingleObject(g_modThread, 3000);
                CloseHandle(g_modThread);
                g_modThread = nullptr;
            }

            // Remove hooks (restore original functions)
            RemoveGameHooks();

            // Destroy the boss
            if (g_Boss) {
                delete g_Boss;
                g_Boss = nullptr;
            }

            // Free the real version.dll
            if (g_realVersionDll) {
                FreeLibrary(g_realVersionDll);
                g_realVersionDll = nullptr;
            }

            printf("[SHUTDOWN] Cleanup complete. Goodbye!\n");

            // Free the debug console
            FreeDebugConsole();

            break;
        }

        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
            // We don't care about these (and we disabled them above)
            break;
    }

    return TRUE; // Return TRUE to let the DLL load/unload successfully
}
