// ============================================================================
// boss.cpp — The Roaring Knight Boss (Implementation)
// ============================================================================
//
// This file implements the CustomBoss class: a Deltarune-inspired armored
// knight boss fight for Plants vs. Zombies 1.
//
// The boss runs as an "overlay entity" — it's not a real PvZ zombie.
// It interacts with the game by:
//   - Checking plant positions to target attacks
//   - Spawning real zombies as minions via the game's spawn system
//   - Drawing itself on screen using GDI (Windows graphics)
//   - Taking damage based on plant projectile positions
//
// ============================================================================

#include "boss.h"
#include "game.h"
#include <cstdio>
#include <cstdlib>  // For rand(), srand()
#include <ctime>    // For time() to seed random

// ============================================================================
// Global Boss Pointer
// ============================================================================
// This is THE boss instance. There's only ever one. It's created in dllmain.cpp
// when we enter a level, and destroyed on shutdown.
CustomBoss* g_Boss = nullptr;

// ============================================================================
// Constructor — Initialize the boss with full HP and Phase 1 settings
// ============================================================================
CustomBoss::CustomBoss() {
    printf("[BOSS] ========================================\n");
    printf("[BOSS]   THE ROARING KNIGHT HAS APPEARED!!\n");
    printf("[BOSS]   HP: %d / %d\n", BossStats::MAX_HP, BossStats::MAX_HP);
    printf("[BOSS]   \"Your petals will wither before my blade!\"\n");
    printf("[BOSS] ========================================\n");

    // --- Initialize HP ---
    m_currentHP = BossStats::MAX_HP;
    m_maxHP     = BossStats::MAX_HP;

    // --- Start in Phase 1 ---
    m_phase = BossPhase::PHASE_1;

    // --- Position: right side of the screen ---
    m_posX = BossStats::BOSS_X;
    m_posY = BossStats::BOSS_Y;

    // --- Attack timer: start with a brief delay before first attack ---
    m_attackCooldown = BossStats::P1_ATTACK_COOLDOWN;
    m_attackTimer    = m_attackCooldown;  // First attack after the full cooldown

    // --- Phase transition ---
    m_phaseTransitionTimer = 0;
    m_isInvulnerable       = false;
    m_invulnTimer          = 0;

    // --- Misc ---
    m_tickCounter      = 0;
    m_lastAttack       = AttackType::NONE;
    m_damageFlashTimer = 0;

    // --- Seed random number generator ---
    srand((unsigned int)time(nullptr));

    // --- Create GDI brushes and pens for drawing ---
    // These are Windows drawing objects. We create them ONCE and reuse them.
    // Creating them every frame would be slow!
    m_bodyBrush        = CreateSolidBrush(RGB(80, 0, 120));    // Dark purple (knight armor)
    m_hpBarBrushGreen  = CreateSolidBrush(RGB(0, 200, 0));     // Green (healthy)
    m_hpBarBrushYellow = CreateSolidBrush(RGB(220, 200, 0));   // Yellow (Phase 2)
    m_hpBarBrushRed    = CreateSolidBrush(RGB(220, 0, 0));     // Red (Phase 3)
    m_hpBarBrushBg     = CreateSolidBrush(RGB(40, 40, 40));    // Dark gray background
    m_outlinePen       = CreatePen(PS_SOLID, 2, RGB(255, 255, 255)); // White outline
}

// ============================================================================
// Destructor — Clean up GDI resources
// ============================================================================
CustomBoss::~CustomBoss() {
    printf("[BOSS] The Roaring Knight has been destroyed.\n");

    // Delete all GDI objects to avoid memory leaks.
    // Windows has a limited number of GDI handles — always clean up!
    DeleteObject(m_bodyBrush);
    DeleteObject(m_hpBarBrushGreen);
    DeleteObject(m_hpBarBrushYellow);
    DeleteObject(m_hpBarBrushRed);
    DeleteObject(m_hpBarBrushBg);
    DeleteObject(m_outlinePen);
}

// ============================================================================
// Update — Called every game tick (~100 times per second)
// ============================================================================
// This is the boss's "brain" — it runs every frame and handles:
//   1. Phase transitions (checking if HP dropped below a threshold)
//   2. Attack timing (counting down to the next attack)
//   3. Damage detection (are plants hurting us?)
//   4. Invulnerability management (during phase transitions)
//
void CustomBoss::Update() {
    // Don't update if the boss is dead
    if (m_phase == BossPhase::DEFEATED) return;

    // Don't update if there's no active game board
    if (!GetBoard()) return;

    m_tickCounter++;

    // --- Handle invulnerability (boss can't be hurt during phase transitions) ---
    if (m_isInvulnerable) {
        m_invulnTimer--;
        if (m_invulnTimer <= 0) {
            m_isInvulnerable = false;
            printf("[BOSS] Invulnerability ended. The knight can be hurt again!\n");
        }
    }

    // --- Handle phase transition animation timer ---
    if (m_phaseTransitionTimer > 0) {
        m_phaseTransitionTimer--;
        // During transition, the boss doesn't attack — just dramatically poses
        return;
    }

    // --- Handle damage flash timer (visual feedback) ---
    if (m_damageFlashTimer > 0) {
        m_damageFlashTimer--;
    }

    // --- Check if plants are dealing damage to us ---
    CheckForPlantDamage();

    // --- Check for phase transitions ---
    CheckPhaseTransition();

    // --- Attack timer ---
    m_attackTimer--;
    if (m_attackTimer <= 0) {
        ChooseAndExecuteAttack();
        m_attackTimer = m_attackCooldown; // Reset the timer
    }
}

// ============================================================================
// Draw — Render the boss on screen
// ============================================================================
// This is called every frame from our Board::Draw hook.
// We use GDI (Windows' built-in 2D graphics) to draw directly on the game
// window. This isn't the prettiest approach, but it works without needing
// to reverse-engineer PvZ's internal rendering system.
//
// The `graphics` parameter is PvZ's internal Graphics* pointer. We don't use
// it directly here — instead we get the game window's HDC (device context)
// and draw with standard Windows GDI calls.
//
void CustomBoss::Draw(void* graphics) {
    // Don't draw if defeated (or add a death animation later!)
    if (m_phase == BossPhase::DEFEATED) return;

    // Get the game window's handle
    // PvZ creates a window with the class name "MainWindow" (SexyAppFramework)
    HWND gameWindow = FindWindowA("MainWindow", nullptr);
    if (!gameWindow) return;

    // Get a device context (DC) for drawing on the window.
    // GetDC lets us draw on top of whatever is already rendered.
    HDC hdc = GetDC(gameWindow);
    if (!hdc) return;

    // Draw the boss body, HP bar, and phase indicator
    DrawBossBody(hdc);
    DrawHPBar(hdc);
    DrawPhaseIndicator(hdc);

    // Release the DC when done — ALWAYS do this or you'll leak GDI handles!
    ReleaseDC(gameWindow, hdc);
}

// ============================================================================
// TakeDamage — Apply damage to the boss
// ============================================================================
void CustomBoss::TakeDamage(int amount) {
    // Can't damage an invulnerable boss
    if (m_isInvulnerable) return;

    // Can't damage a dead boss
    if (m_phase == BossPhase::DEFEATED) return;

    m_currentHP -= amount;
    m_damageFlashTimer = 8; // Flash white for 8 ticks (~80ms)

    // Clamp to zero
    if (m_currentHP <= 0) {
        m_currentHP = 0;
        m_phase = BossPhase::DEFEATED;
        printf("[BOSS] ========================================\n");
        printf("[BOSS]   THE ROARING KNIGHT HAS BEEN DEFEATED!\n");
        printf("[BOSS]   \"Impossible... my armor... shattered...\"\n");
        printf("[BOSS]   VICTORY! The lawn is saved!\n");
        printf("[BOSS] ========================================\n");
    }

    // Only print damage every 10th hit to avoid console spam
    if (m_tickCounter % 10 == 0 && m_currentHP > 0) {
        printf("[BOSS] Took %d damage! HP: %d / %d (%.0f%%)\n",
               amount, m_currentHP, m_maxHP, GetHPPercent() * 100.0f);
    }
}

// ============================================================================
// State Queries
// ============================================================================

bool CustomBoss::IsAlive() const {
    return m_phase != BossPhase::DEFEATED;
}

bool CustomBoss::IsDefeated() const {
    return m_phase == BossPhase::DEFEATED;
}

BossPhase CustomBoss::GetPhase() const {
    return m_phase;
}

float CustomBoss::GetHPPercent() const {
    return (float)m_currentHP / (float)m_maxHP;
}

// ============================================================================
// CheckPhaseTransition — See if we should move to the next phase
// ============================================================================
void CustomBoss::CheckPhaseTransition() {
    float hpPercent = GetHPPercent();

    // Check Phase 1 → Phase 2 transition (at 60% HP)
    if (m_phase == BossPhase::PHASE_1 && hpPercent <= BossStats::PHASE_2_THRESHOLD) {
        OnPhaseTransition(BossPhase::PHASE_2);
    }
    // Check Phase 2 → Phase 3 transition (at 30% HP)
    else if (m_phase == BossPhase::PHASE_2 && hpPercent <= BossStats::PHASE_3_THRESHOLD) {
        OnPhaseTransition(BossPhase::PHASE_3);
    }
}

// ============================================================================
// OnPhaseTransition — Handle the dramatic phase change
// ============================================================================
void CustomBoss::OnPhaseTransition(BossPhase newPhase) {
    m_phase = newPhase;

    // Make the boss invulnerable during the transition animation
    m_isInvulnerable  = true;
    m_invulnTimer     = BossStats::INVULN_TICKS;
    m_phaseTransitionTimer = BossStats::PHASE_TRANSITION_TICKS;

    // Update attack cooldown (boss gets faster each phase)
    switch (newPhase) {
        case BossPhase::PHASE_2:
            m_attackCooldown = BossStats::P2_ATTACK_COOLDOWN;
            printf("[BOSS] ========================================\n");
            printf("[BOSS]   PHASE 2 — ARMOR CRACKED!\n");
            printf("[BOSS]   \"You dare?! I'll show you TRUE power!\"\n");
            printf("[BOSS]   Attack speed increased!\n");
            printf("[BOSS]   HP: %d / %d\n", m_currentHP, m_maxHP);
            printf("[BOSS] ========================================\n");
            break;

        case BossPhase::PHASE_3:
            m_attackCooldown = BossStats::P3_ATTACK_COOLDOWN;
            printf("[BOSS] ========================================\n");
            printf("[BOSS]   PHASE 3 — BERSERKER MODE!\n");
            printf("[BOSS]   \"RRROOOOAAARRR!! NO MORE MERCY!\"\n");
            printf("[BOSS]   Attack speed MAXIMUM! Gargantuars incoming!\n");
            printf("[BOSS]   HP: %d / %d\n", m_currentHP, m_maxHP);
            printf("[BOSS] ========================================\n");
            break;

        default:
            break;
    }

    // Reset attack timer so the boss attacks soon after transitioning
    m_attackTimer = m_attackCooldown / 2;
}

// ============================================================================
// ChooseAndExecuteAttack — Pick and perform an attack based on current phase
// ============================================================================
// The boss alternates between attacks, trying not to repeat the same one
// twice in a row (to keep things interesting).
//
void CustomBoss::ChooseAndExecuteAttack() {
    // Roll a random number to decide the attack
    // Each phase has different attack options
    int roll = rand() % 100; // 0–99

    switch (m_phase) {
        // ---- Phase 1: Basic attacks ----
        case BossPhase::PHASE_1:
            if (roll < 55) {
                // 55% chance: Sword Slash
                // (but if we just did it, summon instead)
                if (m_lastAttack == AttackType::SWORD_SLASH) {
                    AttackSummonBasic();
                } else {
                    AttackSwordSlash();
                }
            } else {
                // 45% chance: Summon basic zombies
                if (m_lastAttack == AttackType::SUMMON_BASIC) {
                    AttackSwordSlash();
                } else {
                    AttackSummonBasic();
                }
            }
            break;

        // ---- Phase 2: Enhanced attacks ----
        case BossPhase::PHASE_2:
            if (roll < 35) {
                AttackCrystalBarrage();
            } else if (roll < 65) {
                AttackSummonArmored();
            } else {
                // Phase 2 can still do sword slashes (but they're scarier now)
                AttackSwordSlash();
            }
            break;

        // ---- Phase 3: Devastating attacks ----
        case BossPhase::PHASE_3:
            if (roll < 35) {
                AttackRoaringSlash();
            } else if (roll < 60) {
                AttackSummonGargantuar();
            } else if (roll < 80) {
                AttackCrystalBarrage();
            } else {
                AttackSummonArmored();
            }
            break;

        default:
            break;
    }
}

// ============================================================================
// ATTACK IMPLEMENTATIONS
// ============================================================================

// --- Sword Slash: Damage all plants in one random row ---
void CustomBoss::AttackSwordSlash() {
    m_lastAttack = AttackType::SWORD_SLASH;

    // Pick a random row (0–4)
    int targetRow = rand() % NUM_ROWS;

    printf("[BOSS] SWORD SLASH! Targeting row %d!\n", targetRow);

    // Damage all plants in that row
    DamagePlantsInRow(targetRow, BossStats::SLASH_DAMAGE);
}

// --- Summon Basic: Spawn a few normal/flag zombies ---
void CustomBoss::AttackSummonBasic() {
    m_lastAttack = AttackType::SUMMON_BASIC;

    printf("[BOSS] SUMMON! Calling %d basic zombies!\n", BossStats::P1_SUMMON_COUNT);

    for (int i = 0; i < BossStats::P1_SUMMON_COUNT; i++) {
        int row = rand() % NUM_ROWS;
        // Alternate between normal and flag zombies
        int type = (i % 2 == 0) ? ZOMBIE_NORMAL : ZOMBIE_FLAG;
        void* zombie = SpawnZombie(type, row);
        if (zombie) {
            printf("[BOSS]   Spawned zombie type %d in row %d\n", type, row);
        }
    }
}

// --- Crystal Barrage: Hit random plants across the whole field ---
void CustomBoss::AttackCrystalBarrage() {
    m_lastAttack = AttackType::CRYSTAL_BARRAGE;

    printf("[BOSS] CRYSTAL BARRAGE! Raining crystals on %d plants!\n",
           BossStats::CRYSTAL_HIT_COUNT);

    DamageRandomPlants(BossStats::CRYSTAL_HIT_COUNT, BossStats::CRYSTAL_DAMAGE);
}

// --- Summon Armored: Spawn conehead and buckethead zombies ---
void CustomBoss::AttackSummonArmored() {
    m_lastAttack = AttackType::SUMMON_ARMORED;

    printf("[BOSS] SUMMON ARMORED! Calling %d armored zombies!\n",
           BossStats::P2_SUMMON_COUNT);

    for (int i = 0; i < BossStats::P2_SUMMON_COUNT; i++) {
        int row = rand() % NUM_ROWS;
        // Alternate between coneheads and bucketheads
        int type = (i % 2 == 0) ? ZOMBIE_CONEHEAD : ZOMBIE_BUCKETHEAD;
        void* zombie = SpawnZombie(type, row);
        if (zombie) {
            printf("[BOSS]   Spawned armored zombie type %d in row %d\n", type, row);
        }
    }
}

// --- Roaring Slash: Massive attack hitting ALL plants in 2 columns ---
void CustomBoss::AttackRoaringSlash() {
    m_lastAttack = AttackType::ROARING_SLASH;

    int col1 = BossStats::ROARING_COL_START;
    int col2 = col1 + 1;

    printf("[BOSS] *** ROARING SLASH!! *** Devastating columns %d and %d!\n",
           col1, col2);

    DamagePlantsInColumn(col1, BossStats::ROARING_DAMAGE);
    DamagePlantsInColumn(col2, BossStats::ROARING_DAMAGE);
}

// --- Summon Gargantuar: The big boys come out to play ---
void CustomBoss::AttackSummonGargantuar() {
    m_lastAttack = AttackType::SUMMON_GARGANTUAR;

    printf("[BOSS] !!! SUMMON GARGANTUAR !!! The ground shakes!\n");

    for (int i = 0; i < BossStats::P3_GARG_COUNT; i++) {
        int row = rand() % NUM_ROWS;
        void* zombie = SpawnZombie(ZOMBIE_GARGANTUAR, row);
        if (zombie) {
            printf("[BOSS]   A GARGANTUAR has appeared in row %d!!\n", row);
        }
    }
}

// ============================================================================
// Plant Interaction — Dealing damage to plants and taking damage from them
// ============================================================================

// --- Check if any plants are shooting at the boss ---
// We detect this by looking at the game's projectile system.
// For simplicity in v1, we check every ~30 ticks if there are plants in rows
// that could be shooting and deal passive damage to the boss.
// This simulates "plants are shooting at the boss."
void CustomBoss::CheckForPlantDamage() {
    // Only check every 30 ticks (about 3 times per second) for performance
    if (m_tickCounter % 30 != 0) return;

    uintptr_t plantBase = 0;
    int plantCount = 0;
    if (!GetPlantArray(plantBase, plantCount)) return;

    // Count how many ALIVE shooting plants there are.
    // Each one deals damage to the boss (they're shooting peas at it).
    int shooterCount = 0;

    for (int i = 0; i < plantCount; i++) {
        uintptr_t plant = plantBase + (i * PLANT_STRUCT_SIZE);

        // Skip dead plants
        bool isDead = *(bool*)(plant + PLANT_DEAD);
        if (isDead) continue;

        // Get the plant type
        int plantType = *(int*)(plant + PLANT_TYPE);

        // Check if this is a "shooting" plant (one that fires projectiles)
        bool isShooting = false;
        switch (plantType) {
            case PLANT_PEASHOOTER:
            case PLANT_REPEATER:
            case PLANT_SNOW_PEA:
            case PLANT_THREEPEATER:
            case PLANT_SPLIT_PEA:
            case PLANT_GATLING_PEA:
            case PLANT_CACTUS:
            case PLANT_STARFRUIT:
            case PLANT_CABBAGE_PULT:
            case PLANT_KERNEL_PULT:
            case PLANT_MELON_PULT:
            case PLANT_WINTER_MELON:
            case PLANT_COB_CANNON:
            case PLANT_CATTAIL:
            case PLANT_FUMESHROOM:
            case PLANT_GLOOMSHROOM:
                isShooting = true;
                break;
        }

        if (isShooting) {
            shooterCount++;
        }
    }

    // Each shooter deals PEA_DAMAGE to the boss every check interval
    if (shooterCount > 0) {
        int totalDamage = shooterCount * BossStats::PEA_DAMAGE;
        TakeDamage(totalDamage);
    }
}

// --- Damage all plants in a specific row ---
void CustomBoss::DamagePlantsInRow(int row, int damage) {
    uintptr_t plantBase = 0;
    int plantCount = 0;
    if (!GetPlantArray(plantBase, plantCount)) return;

    int plantsHit = 0;

    for (int i = 0; i < plantCount; i++) {
        uintptr_t plant = plantBase + (i * PLANT_STRUCT_SIZE);

        // Skip dead plants
        bool isDead = *(bool*)(plant + PLANT_DEAD);
        if (isDead) continue;

        // Check if this plant is in the target row
        int plantRow = *(int*)(plant + PLANT_ROW);
        if (plantRow != row) continue;

        // Deal damage to this plant!
        int* hp = (int*)(plant + PLANT_HEALTH);
        *hp -= damage;
        plantsHit++;

        // If the plant's HP went to 0 or below, the game will handle
        // destroying it on the next update tick.
        if (*hp <= 0) {
            printf("[BOSS]   Plant in row %d was destroyed!\n", row);
        }
    }

    if (plantsHit > 0) {
        printf("[BOSS]   Sword Slash hit %d plants in row %d for %d damage each!\n",
               plantsHit, row, damage);
    }
}

// --- Damage all plants in a specific column ---
void CustomBoss::DamagePlantsInColumn(int col, int damage) {
    uintptr_t plantBase = 0;
    int plantCount = 0;
    if (!GetPlantArray(plantBase, plantCount)) return;

    int plantsHit = 0;

    for (int i = 0; i < plantCount; i++) {
        uintptr_t plant = plantBase + (i * PLANT_STRUCT_SIZE);

        bool isDead = *(bool*)(plant + PLANT_DEAD);
        if (isDead) continue;

        int plantCol = *(int*)(plant + PLANT_COL);
        if (plantCol != col) continue;

        int* hp = (int*)(plant + PLANT_HEALTH);
        *hp -= damage;
        plantsHit++;

        if (*hp <= 0) {
            printf("[BOSS]   Plant in column %d was destroyed!\n", col);
        }
    }

    if (plantsHit > 0) {
        printf("[BOSS]   Attack hit %d plants in column %d for %d damage each!\n",
               plantsHit, col, damage);
    }
}

// --- Damage random plants anywhere on the field ---
void CustomBoss::DamageRandomPlants(int count, int damage) {
    uintptr_t plantBase = 0;
    int plantCount = 0;
    if (!GetPlantArray(plantBase, plantCount)) return;

    // First, collect indices of all alive plants
    // We use a simple array since we're avoiding STL
    int alivePlants[200]; // More than enough for any PvZ level
    int aliveCount = 0;

    for (int i = 0; i < plantCount && aliveCount < 200; i++) {
        uintptr_t plant = plantBase + (i * PLANT_STRUCT_SIZE);
        bool isDead = *(bool*)(plant + PLANT_DEAD);
        if (!isDead) {
            alivePlants[aliveCount++] = i;
        }
    }

    if (aliveCount == 0) {
        printf("[BOSS]   Crystal Barrage: No plants to hit!\n");
        return;
    }

    // Hit 'count' random plants (or fewer if there aren't enough)
    int toHit = (count < aliveCount) ? count : aliveCount;
    for (int h = 0; h < toHit; h++) {
        // Pick a random alive plant
        int randomIdx = rand() % aliveCount;
        int plantIdx  = alivePlants[randomIdx];
        uintptr_t plant = plantBase + (plantIdx * PLANT_STRUCT_SIZE);

        int* hp  = (int*)(plant + PLANT_HEALTH);
        int  row = *(int*)(plant + PLANT_ROW);
        int  col = *(int*)(plant + PLANT_COL);

        *hp -= damage;
        printf("[BOSS]   Crystal hit plant at row %d, col %d for %d damage! (HP: %d)\n",
               row, col, damage, *hp);

        // Remove this plant from the pool so we don't hit it twice
        // (swap with last element and shrink)
        alivePlants[randomIdx] = alivePlants[aliveCount - 1];
        aliveCount--;
    }
}

// ============================================================================
// DRAWING — Render the boss on screen
// ============================================================================
// All drawing uses Windows GDI — the simplest way to draw shapes and text
// on a window. Later, you could replace this with DirectDraw hooks for
// proper sprite rendering.

// --- Draw the boss body (a colored rectangle placeholder) ---
void CustomBoss::DrawBossBody(HDC hdc) {
    // Select our drawing tools
    HBRUSH oldBrush;
    HPEN   oldPen;

    // Flash white when taking damage (visual feedback!)
    if (m_damageFlashTimer > 0) {
        HBRUSH flashBrush = CreateSolidBrush(RGB(255, 255, 255));
        oldBrush = (HBRUSH)SelectObject(hdc, flashBrush);
        oldPen   = (HPEN)SelectObject(hdc, m_outlinePen);

        // Draw the boss rectangle
        Rectangle(hdc,
            (int)m_posX,                            // Left
            (int)m_posY,                            // Top
            (int)m_posX + BossStats::BOSS_WIDTH,    // Right
            (int)m_posY + BossStats::BOSS_HEIGHT    // Bottom
        );

        // Clean up flash brush
        SelectObject(hdc, oldBrush);
        SelectObject(hdc, oldPen);
        DeleteObject(flashBrush);
        return;
    }

    // Choose body color based on phase
    HBRUSH bodyBrush;
    switch (m_phase) {
        case BossPhase::PHASE_1:
            bodyBrush = CreateSolidBrush(RGB(80, 0, 120));   // Dark purple
            break;
        case BossPhase::PHASE_2:
            bodyBrush = CreateSolidBrush(RGB(140, 0, 80));   // Magenta
            break;
        case BossPhase::PHASE_3:
            bodyBrush = CreateSolidBrush(RGB(200, 0, 0));    // Angry red!
            break;
        default:
            bodyBrush = m_bodyBrush;
            break;
    }

    // Pulsing effect during phase transitions (invulnerability)
    if (m_isInvulnerable && (m_tickCounter / 5) % 2 == 0) {
        // Flash between body color and gold during transition
        DeleteObject(bodyBrush);
        bodyBrush = CreateSolidBrush(RGB(255, 215, 0)); // Gold flash
    }

    oldBrush = (HBRUSH)SelectObject(hdc, bodyBrush);
    oldPen   = (HPEN)SelectObject(hdc, m_outlinePen);

    // Draw the boss body rectangle
    Rectangle(hdc,
        (int)m_posX,
        (int)m_posY,
        (int)m_posX + BossStats::BOSS_WIDTH,
        (int)m_posY + BossStats::BOSS_HEIGHT
    );

    // Draw a "helmet" rectangle on top (smaller, darker)
    HBRUSH helmetBrush = CreateSolidBrush(RGB(40, 40, 60));
    HBRUSH prevBrush = (HBRUSH)SelectObject(hdc, helmetBrush);
    Rectangle(hdc,
        (int)m_posX + 10,
        (int)m_posY + 10,
        (int)m_posX + BossStats::BOSS_WIDTH - 10,
        (int)m_posY + 70
    );
    SelectObject(hdc, prevBrush);
    DeleteObject(helmetBrush);

    // Draw a sword line (simple vertical line on the left side of boss)
    HPEN swordPen = CreatePen(PS_SOLID, 3, RGB(200, 200, 220));
    HPEN prevPen = (HPEN)SelectObject(hdc, swordPen);
    MoveToEx(hdc, (int)m_posX - 10, (int)m_posY + 80, nullptr);
    LineTo(hdc,   (int)m_posX - 10, (int)m_posY + 350);
    SelectObject(hdc, prevPen);
    DeleteObject(swordPen);

    // Restore original GDI objects
    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);

    // Clean up the phase-specific brush (but not the member brushes!)
    if (bodyBrush != m_bodyBrush) {
        DeleteObject(bodyBrush);
    }
}

// --- Draw the HP bar at the top of the screen ---
void CustomBoss::DrawHPBar(HDC hdc) {
    // HP bar dimensions (drawn at the very top of the screen)
    const int barX      = 150;   // Left edge
    const int barY      = 8;     // Top edge
    const int barWidth  = 500;   // Full width
    const int barHeight = 22;    // Height

    // --- Draw background (dark gray rectangle) ---
    HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, m_hpBarBrushBg);
    HPEN   oldPen   = (HPEN)SelectObject(hdc, m_outlinePen);
    Rectangle(hdc, barX, barY, barX + barWidth, barY + barHeight);

    // --- Draw the filled HP portion ---
    float hpPercent = GetHPPercent();
    int filledWidth = (int)(barWidth * hpPercent);

    // Choose color based on phase
    HBRUSH hpBrush;
    switch (m_phase) {
        case BossPhase::PHASE_1: hpBrush = m_hpBarBrushGreen;  break;
        case BossPhase::PHASE_2: hpBrush = m_hpBarBrushYellow; break;
        case BossPhase::PHASE_3: hpBrush = m_hpBarBrushRed;    break;
        default:                 hpBrush = m_hpBarBrushGreen;  break;
    }

    SelectObject(hdc, hpBrush);
    // No outline for the filled part — use a null pen
    HPEN nullPen = CreatePen(PS_NULL, 0, 0);
    SelectObject(hdc, nullPen);
    Rectangle(hdc, barX + 2, barY + 2, barX + 2 + filledWidth - 4, barY + barHeight - 2);

    // --- Draw boss name text ---
    SetBkMode(hdc, TRANSPARENT);            // Don't draw text background
    SetTextColor(hdc, RGB(255, 255, 255));  // White text

    // Draw the boss name on the left
    const char* bossName = "THE ROARING KNIGHT";
    TextOutA(hdc, barX + 5, barY + 3, bossName, (int)strlen(bossName));

    // Draw HP numbers on the right
    char hpText[64];
    sprintf_s(hpText, "%d / %d", m_currentHP, m_maxHP);
    int textLen = (int)strlen(hpText);
    SIZE textSize;
    GetTextExtentPoint32A(hdc, hpText, textLen, &textSize);
    TextOutA(hdc, barX + barWidth - textSize.cx - 8, barY + 3, hpText, textLen);

    // Restore GDI objects
    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(nullPen);
}

// --- Draw the current phase indicator ---
void CustomBoss::DrawPhaseIndicator(HDC hdc) {
    const char* phaseText = "";
    switch (m_phase) {
        case BossPhase::PHASE_1: phaseText = "[Phase 1 - ARMORED]";   break;
        case BossPhase::PHASE_2: phaseText = "[Phase 2 - CRACKED]";   break;
        case BossPhase::PHASE_3: phaseText = "[Phase 3 - BERSERKER]"; break;
        default: return;
    }

    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(255, 200, 100));  // Gold-ish color
    TextOutA(hdc, 330, 32, phaseText, (int)strlen(phaseText));

    // During phase transition, draw a dramatic message
    if (m_phaseTransitionTimer > 0) {
        SetTextColor(hdc, RGB(255, 0, 0));  // Red
        const char* transMsg = "!! PHASE TRANSITION !!";
        TextOutA(hdc, 310, 50, transMsg, (int)strlen(transMsg));
    }

    // During invulnerability, show a shield icon (text)
    if (m_isInvulnerable) {
        SetTextColor(hdc, RGB(100, 200, 255));  // Light blue
        const char* shieldMsg = "[INVULNERABLE]";
        TextOutA(hdc, 345, 50, shieldMsg, (int)strlen(shieldMsg));
    }
}
