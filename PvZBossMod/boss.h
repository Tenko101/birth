#pragma once
// ============================================================================
// boss.h — The Roaring Knight Boss (Header)
// ============================================================================
//
// This file defines the CustomBoss class: a multi-phase boss fight inspired
// by the Roaring Knight from Deltarune. The boss is a massive armored knight
// zombie that the player must defeat using their plants.
//
// BOSS OVERVIEW:
//   - 3000 HP total, divided into 3 phases
//   - Phase 1 (100%–60% HP): Basic attacks, summons regular zombies
//   - Phase 2 (60%–30% HP):  Stronger attacks, faster pace, tougher summons
//   - Phase 3 (30%–0% HP):   Devastating attacks, summons Gargantuars!
//   - Boss sits on the right side of the screen and attacks the player's lawn
//
// DESIGN PHILOSOPHY:
//   The boss is implemented as a standalone C++ class that gets Update()'d
//   and Draw()'n each frame via our hooks into Board::Update/Board::Draw.
//   It's intentionally NOT a "real" zombie in the game's zombie array —
//   that would require deep reverse engineering of the zombie state machine.
//   Instead, it's an overlay entity that interacts with the game by:
//     - Reading plant positions to target attacks
//     - Writing to the zombie spawn system to summon minions
//     - Drawing directly to the screen via GDI
//
// ============================================================================

#ifndef BOSS_H
#define BOSS_H

#include <windows.h>
#include <cstdint>

// ============================================================================
// Boss Phase Enum
// ============================================================================
enum class BossPhase {
    PHASE_1,        // Full armor — basic attacks (100% to 60% HP)
    PHASE_2,        // Cracked armor — enhanced attacks (60% to 30% HP)
    PHASE_3,        // Berserker mode — devastating attacks (30% to 0% HP)
    DEFEATED        // Boss is dead — victory!
};

// ============================================================================
// Attack Type Enum
// ============================================================================
enum class AttackType {
    NONE,               // No attack happening
    SWORD_SLASH,        // Phase 1: Damages plants in one row
    SUMMON_BASIC,       // Phase 1: Summons normal/flag zombies
    CRYSTAL_BARRAGE,    // Phase 2: Damages random plants across the field
    SUMMON_ARMORED,     // Phase 2: Summons cone/bucket head zombies
    ROARING_SLASH,      // Phase 3: Damages ALL plants in 2 columns
    SUMMON_GARGANTUAR,  // Phase 3: Summons Gargantuars!!
};

// ============================================================================
// Boss Stats — Easy to tweak!
// ============================================================================
// All boss stats are in one place so you can easily adjust difficulty.

namespace BossStats {
    // --- HP ---
    constexpr int   MAX_HP              = 3000;   // Total boss hitpoints
    constexpr float PHASE_2_THRESHOLD   = 0.60f;  // Enter Phase 2 at 60% HP
    constexpr float PHASE_3_THRESHOLD   = 0.30f;  // Enter Phase 3 at 30% HP

    // --- Position ---
    // The boss is drawn on the right side of the screen.
    // These are pixel coordinates.
    constexpr float BOSS_X              = 700.0f; // X position (near right edge)
    constexpr float BOSS_Y              = 40.0f;  // Y position (top area)
    constexpr int   BOSS_WIDTH          = 80;     // Width in pixels
    constexpr int   BOSS_HEIGHT         = 480;    // Height in pixels (tall knight!)
    // The boss occupies columns 7–8 conceptually

    // --- Attack Timing (in game ticks, ~100 ticks/sec) ---
    constexpr int   P1_ATTACK_COOLDOWN  = 300;    // Phase 1: attack every 3 seconds
    constexpr int   P2_ATTACK_COOLDOWN  = 200;    // Phase 2: every 2 seconds
    constexpr int   P3_ATTACK_COOLDOWN  = 120;    // Phase 3: every 1.2 seconds!

    // --- Attack Damage ---
    constexpr int   SLASH_DAMAGE        = 150;    // Sword Slash damage per plant hit
    constexpr int   CRYSTAL_DAMAGE      = 100;    // Crystal Barrage damage per hit
    constexpr int   ROARING_DAMAGE      = 300;    // Roaring Slash (Phase 3) — ouch!
    constexpr int   CRYSTAL_HIT_COUNT   = 3;      // Crystal Barrage hits this many random plants
    constexpr int   ROARING_COL_START   = 6;      // Roaring Slash hits columns 6 and 7

    // --- Summon Counts ---
    constexpr int   P1_SUMMON_COUNT     = 2;      // Phase 1: summon 2 basic zombies
    constexpr int   P2_SUMMON_COUNT     = 3;      // Phase 2: summon 3 armored zombies
    constexpr int   P3_GARG_COUNT       = 1;      // Phase 3: summon 1 Gargantuar at a time

    // --- Damage the boss takes from plants ---
    // Plants deal damage to the boss when they shoot peas that reach
    // the boss's X position. We detect this via projectile position checks.
    // For simplicity, we also let the player damage the boss by placing
    // plants in columns 7–8.
    constexpr int   BOSS_HITBOX_LEFT    = 660;    // Left edge of boss hitbox
    constexpr int   BOSS_HITBOX_RIGHT   = 780;    // Right edge of boss hitbox

    // --- Phase Transition ---
    constexpr int   PHASE_TRANSITION_TICKS = 150; // How long the transition effect lasts
    constexpr int   INVULN_TICKS           = 100; // Invulnerability during phase transition

    // --- Damage absorption per tick ---
    // We check if peas are in the boss hitbox each frame.
    // Each "pea hit" does this much damage to the boss:
    constexpr int   PEA_DAMAGE          = 20;     // One pea hit = 20 damage to boss
}

// ============================================================================
// CustomBoss Class
// ============================================================================
class CustomBoss {
public:
    // ----- Constructor / Destructor -----
    CustomBoss();
    ~CustomBoss();

    // ----- Core Loop -----
    void Update();                  // Called every game tick from our Board::Update hook
    void Draw(void* graphics);      // Called every frame from our Board::Draw hook

    // ----- Damage -----
    void TakeDamage(int amount);    // Apply damage to the boss

    // ----- State Queries -----
    bool IsAlive() const;
    bool IsDefeated() const;
    BossPhase GetPhase() const;
    float GetHPPercent() const;     // Returns 0.0–1.0

private:
    // ----- Phase Management -----
    void CheckPhaseTransition();    // Check if we should advance to the next phase
    void OnPhaseTransition(BossPhase newPhase); // Handle effects when entering a new phase

    // ----- Attack Logic -----
    void ChooseAndExecuteAttack();  // Pick an attack based on current phase
    void AttackSwordSlash();        // Phase 1: damage one row
    void AttackSummonBasic();       // Phase 1: summon weak zombies
    void AttackCrystalBarrage();    // Phase 2: hit random plants
    void AttackSummonArmored();     // Phase 2: summon tough zombies
    void AttackRoaringSlash();      // Phase 3: devastate 2 columns
    void AttackSummonGargantuar();  // Phase 3: summon the big guys

    // ----- Plant Interaction -----
    void CheckForPlantDamage();     // Check if plants are dealing damage to us
    void DamagePlantsInRow(int row, int damage);                // Hurt plants in a specific row
    void DamagePlantsInColumn(int col, int damage);             // Hurt plants in a specific column
    void DamageRandomPlants(int count, int damage);             // Hurt random plants anywhere

    // ----- Drawing Helpers -----
    void DrawBossBody(HDC hdc);     // Draw the boss rectangle/sprite placeholder
    void DrawHPBar(HDC hdc);        // Draw the HP bar at the top of the screen
    void DrawPhaseIndicator(HDC hdc); // Draw which phase we're in

    // ----- Member Variables -----
    int         m_currentHP;            // Current hitpoints
    int         m_maxHP;                // Maximum hitpoints (3000)
    BossPhase   m_phase;                // Current phase
    float       m_posX;                 // Screen X position (pixels)
    float       m_posY;                 // Screen Y position (pixels)
    int         m_attackTimer;          // Counts down to next attack
    int         m_attackCooldown;       // Ticks between attacks (changes per phase)
    int         m_phaseTransitionTimer; // Countdown during phase transitions
    bool        m_isInvulnerable;       // True during phase transition animations
    int         m_invulnTimer;          // Ticks of invulnerability remaining
    int         m_tickCounter;          // Total ticks this boss has been alive
    AttackType  m_lastAttack;           // The last attack we performed (avoid repeats)
    int         m_damageFlashTimer;     // Visual feedback when boss takes damage

    // GDI resources for drawing (created once, reused every frame)
    HBRUSH      m_bodyBrush;            // Brush for drawing the boss body
    HBRUSH      m_hpBarBrushGreen;      // Green HP bar (healthy)
    HBRUSH      m_hpBarBrushYellow;     // Yellow HP bar (Phase 2)
    HBRUSH      m_hpBarBrushRed;        // Red HP bar (Phase 3)
    HBRUSH      m_hpBarBrushBg;         // Dark background for HP bar
    HPEN        m_outlinePen;           // White outline pen
};

// ============================================================================
// Global Boss Pointer
// ============================================================================
// This is the single global boss instance. Created when the mod loads,
// destroyed on shutdown. Checked in the hook callbacks.
extern CustomBoss* g_Boss;

#endif // BOSS_H
