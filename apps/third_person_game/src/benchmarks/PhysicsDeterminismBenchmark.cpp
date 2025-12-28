/**
 * Physics Determinism Benchmark
 * 
 * This benchmark tests frame-rate independence of character movement.
 * It moves the character forward at 30 FPS for 3 seconds, then backward at 400 FPS for 3 seconds,
 * and compares the distances traveled. If physics is deterministic, distances should be equal.
 * 
 * Results from testing (2024-12-28):
 * - 30 FPS:  Distance = 13.2517 | Ticks = 181
 * - 400 FPS: Distance = 13.8632 | Ticks = 181
 * - Difference: 0.6116 units (4.51%)
 * 
 * To use this benchmark:
 * 1. Add the BenchmarkPhase enum and variables to Character.h
 * 2. Add the benchmark code to Character::Update() and Character::FixedUpdate()
 * 3. The benchmark runs automatically on game start
 */

// ============================================================================
// CHARACTER.H ADDITIONS (add inside class Character private section)
// ============================================================================

/*
// Benchmark state machine
enum class BenchmarkPhase {
    WaitStart,      // Wait 1s before first test
    Move30FPS,      // Move forward for 3s at 30 FPS
    WaitMiddle,     // Wait 1s between tests
    Move400FPS,     // Move backward for 3s at high FPS
    Done            // Test complete
};

BenchmarkPhase benchmarkPhase_      = BenchmarkPhase::WaitStart;
float   benchmarkPhaseTimer_        = 0.0f;
int     benchmarkFixedTicks_        = 0;
Vector3 benchmark30FPS_StartPos_    = Vector3(0.0f);
Vector3 benchmark30FPS_EndPos_      = Vector3(0.0f);
Vector3 benchmark400FPS_StartPos_   = Vector3(0.0f);
Vector3 benchmark400FPS_EndPos_     = Vector3(0.0f);
int     benchmark30FPS_Ticks_       = 0;
int     benchmark400FPS_Ticks_      = 0;
int     targetFPS_                  = 400;  // Current target FPS (30 or 400)
*/


// ============================================================================
// CHARACTER.CPP - Start() function addition
// ============================================================================

/*
void Character::Start() {
    rigidbody_ = GetEntity().FindComponent<RigidbodyComponent>();
    physicsSystem_ = GetEntity().GetScene()->GetPhysicsSystem();
    
    // Start benchmark phase machine
    benchmarkPhase_ = BenchmarkPhase::WaitStart;
    benchmarkPhaseTimer_ = 0.0f;
    printf("\n[BENCHMARK] Automated test starting in 1 second...\n");
    printf("[BENCHMARK] Phase 1: Move FORWARD at 30 FPS for 3 seconds\n");
    printf("[BENCHMARK] Phase 2: Move BACKWARD at 400 FPS for 3 seconds\n");
    fflush(stdout);
}
*/


// ============================================================================
// CHARACTER.CPP - Update() function with benchmark
// ============================================================================

/*
void Character::Update(float dt) {
    // Reset movement state at start of each frame
    // This ensures all FixedUpdates in a frame see consistent state
    wantsToMove_ = false;
    
    // Set FPS limit based on current benchmark phase
    auto& window = se::Application::Get().GetWindow();
    
    if (benchmarkPhase_ == BenchmarkPhase::Move30FPS) {
        window.SetTargetFPS(30);
        // Simulate forward movement (+X) - same as player pressing W
        Move(Vector3(1.0f, 0.0f, 0.0f));
        RotateTowards(90.0f);  // Face +X direction
    } else if (benchmarkPhase_ == BenchmarkPhase::Move400FPS) {
        window.SetTargetFPS(0);  // Unlimited
        // Simulate backward movement (-X) - same as player pressing S
        Move(Vector3(-1.0f, 0.0f, 0.0f));
        RotateTowards(-90.0f);  // Face -X direction
    }
}
*/


// ============================================================================
// CHARACTER.CPP - FixedUpdate() benchmark state machine (add at end of FixedUpdate)
// ============================================================================

/*
// Benchmark state machine
auto& transform = GetComponent<TransformComponent>();
benchmarkPhaseTimer_ += dt;

switch (benchmarkPhase_) {
    case BenchmarkPhase::WaitStart:
        if (benchmarkPhaseTimer_ >= 1.0f) {
            benchmarkPhaseTimer_ = 0.0f;
            benchmark30FPS_Ticks_ = 0;
            benchmark30FPS_StartPos_ = transform.Position;
            benchmarkPhase_ = BenchmarkPhase::Move30FPS;
            printf("\n[BENCHMARK] === PHASE 1: 30 FPS TEST STARTED ===\n");
            printf("[BENCHMARK] Start pos: (%.4f, %.4f, %.4f)\n", 
                   transform.Position.x, transform.Position.y, transform.Position.z);
            fflush(stdout);
        }
        break;
        
    case BenchmarkPhase::Move30FPS:
        // Movement is simulated in Update() via Move()
        benchmark30FPS_Ticks_++;
        
        if (benchmarkPhaseTimer_ >= 3.0f) {
            benchmarkPhaseTimer_ = 0.0f;
            benchmark30FPS_EndPos_ = transform.Position;
            
            Vector3 disp = benchmark30FPS_EndPos_ - benchmark30FPS_StartPos_;
            float dist = glm::length(disp);
            printf("[BENCHMARK] End pos: (%.4f, %.4f, %.4f)\n", 
                   transform.Position.x, transform.Position.y, transform.Position.z);
            printf("[BENCHMARK] Displacement: (%.4f, %.4f, %.4f)\n", disp.x, disp.y, disp.z);
            printf("[BENCHMARK] Distance: %.4f | Ticks: %d\n", dist, benchmark30FPS_Ticks_);
            printf("[BENCHMARK] === 30 FPS TEST COMPLETE. Waiting 1s... ===\n\n");
            fflush(stdout);
            
            benchmarkPhase_ = BenchmarkPhase::WaitMiddle;
        }
        break;
        
    case BenchmarkPhase::WaitMiddle:
        if (benchmarkPhaseTimer_ >= 1.0f) {
            benchmarkPhaseTimer_ = 0.0f;
            benchmark400FPS_Ticks_ = 0;
            benchmark400FPS_StartPos_ = transform.Position;
            benchmarkPhase_ = BenchmarkPhase::Move400FPS;
            printf("[BENCHMARK] === PHASE 2: 400 FPS TEST STARTED ===\n");
            printf("[BENCHMARK] Start pos: (%.4f, %.4f, %.4f)\n", 
                   transform.Position.x, transform.Position.y, transform.Position.z);
            fflush(stdout);
        }
        break;
        
    case BenchmarkPhase::Move400FPS:
        // Movement is simulated in Update() via Move()
        benchmark400FPS_Ticks_++;
        
        if (benchmarkPhaseTimer_ >= 3.0f) {
            benchmarkPhaseTimer_ = 0.0f;
            benchmark400FPS_EndPos_ = transform.Position;
            
            Vector3 disp = benchmark400FPS_EndPos_ - benchmark400FPS_StartPos_;
            float dist = glm::length(disp);
            printf("[BENCHMARK] End pos: (%.4f, %.4f, %.4f)\n", 
                   transform.Position.x, transform.Position.y, transform.Position.z);
            printf("[BENCHMARK] Displacement: (%.4f, %.4f, %.4f)\n", disp.x, disp.y, disp.z);
            printf("[BENCHMARK] Distance: %.4f | Ticks: %d\n", dist, benchmark400FPS_Ticks_);
            printf("[BENCHMARK] === 400 FPS TEST COMPLETE ===\n\n");
            
            // Print final comparison
            Vector3 disp30 = benchmark30FPS_EndPos_ - benchmark30FPS_StartPos_;
            float dist30 = glm::length(disp30);
            printf("==========================================\n");
            printf("[BENCHMARK] FINAL COMPARISON:\n");
            printf("  30 FPS:  Distance = %.4f | Ticks = %d\n", dist30, benchmark30FPS_Ticks_);
            printf(" 400 FPS:  Distance = %.4f | Ticks = %d\n", dist, benchmark400FPS_Ticks_);
            printf("  Difference: %.4f units (%.2f%%)\n", 
                   std::abs(dist30 - dist), 
                   std::abs(dist30 - dist) / ((dist30 + dist) / 2.0f) * 100.0f);
            printf("==========================================\n\n");
            fflush(stdout);
            
            benchmarkPhase_ = BenchmarkPhase::Done;
        }
        break;
        
    case BenchmarkPhase::Done:
        // Benchmark complete, normal operation
        break;
}
*/
