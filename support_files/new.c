// Params
const float Imax = 32.0;
const float MIN_ISET = 7.0;
const float SAFETY_MARGIN = 0.5; // A headroom
const float EMERG_MARGIN = 1.0;  // emergency threshold above Imax
const float RATE_NORMAL = 2.0;   // A per update
const float RATE_FAST = 6.0;     // A per update (fast reaction)
const int EMERG_DEBOUNCE_SAMPLES = 1; // short confirmation
const int EMERG_LOCK_SECONDS = 10;    // short lock after emergency

// State
float Iset = 7.0;
int emerg_counter = 0;
int emerg_lock_timer = 0; // seconds remaining for lock

void loop_tick() {
    float Ich = read_charging_current();
    float Iuti = read_house_current();
    float required = Iuti - Imax + SAFETY_MARGIN; // positive if over

    // Clear emergency lock timer if running
    if (emerg_lock_timer > 0) emerg_lock_timer -= SAMPLE_INTERVAL_S;

    if (required <= 0.0) {
        // under limit -> normal control (PI etc.)
        emerg_counter = 0;
        // normal controller call (PI) updates Iset with RATE_NORMAL
        Iset = normal_PI_step(Iuti, Ich, Iset);
        return;
    }

    // Overload detected
    if (Iuti >= Imax + EMERG_MARGIN) {
        // emergency threshold exceeded
        emerg_counter++;
    } else {
        emerg_counter = 0;
    }

    if (emerg_counter >= EMERG_DEBOUNCE_SAMPLES || emerg_lock_timer > 0) {
        // Emergency action (fast reduction). If in lock, continue stricter behavior.
        float delta = required; // how much to reduce by to get back under Imax
        float reduce_by = delta;
        // Apply max fast rate unless you want immediate drop
        if (reduce_by > RATE_FAST) reduce_by = RATE_FAST;
        float Iset_candidate = Iset - reduce_by;
        if (Iset_candidate < MIN_ISET) Iset_candidate = MIN_ISET;

        // If delta is very large (can't fix with RATE_FAST), consider immediate MIN_ISET
        if (required >= (Iset - MIN_ISET)) {
            Iset_candidate = MIN_ISET;
        }

        // Apply immediately (bypass slow rate), but you can choose to step quickly
        Iset = Iset_candidate;

        // set emergency lock so we keep tight monitoring
        emerg_lock_timer = EMERG_LOCK_SECONDS;

        // optionally notify/log
        log_event("EMERGENCY_REDUCE", Iset, Iuti);
        // write set current to charger
        request_set_current(Iset);
        return;
    }

    // Otherwise, mild overload but below emergency margin
    // Do a fast constrained reduction (faster than normal)
    float reduce_by = required;
    if (reduce_by > RATE_FAST) reduce_by = RATE_FAST;
    float Iset_candidate = Iset - reduce_by;
    if (Iset_candidate < MIN_ISET) Iset_candidate = MIN_ISET;

    // Use this change but still respect some rate limit
    if (Iset_candidate < Iset - RATE_NORMAL) {
        // allow faster change but limited to RATE_FAST
        Iset = max(Iset - RATE_FAST, Iset_candidate);
    } else {
        Iset = Iset_candidate;
    }

    request_set_current(Iset);
    // do not reset emerg_counter here; keep observing
}
