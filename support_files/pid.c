// Parameters
const float Imax = 32.0;
const float MIN_ISET = 7.0;
const float Ts = 5.0; // s
// controller gains (start conservative)
const float Kp = 0.2;
const float Ki = 0.02;   // 1/s

const float RATE_LIMIT = 2.0; // A per update
const float HYSTERESIS = 0.5; // A
const float EMA_ALPHA = 0.85; // filter measurements

// State
float Iset = MIN_ISET;
float u_prev = Iset;
float e_prev = 0.0;
float Iuti_f = 0.0;
float Ich_f = 0.0;

loop every Ts seconds {
    // read raw
    float Ich_raw = read_charging_current();
    float Iuti_raw = read_house_current();

    // filter measurements
    Ich_f  = EMA_ALPHA*Ich_f  + (1-EMA_ALPHA)*Ich_raw;
    Iuti_f = EMA_ALPHA*Iuti_f + (1-EMA_ALPHA)*Iuti_raw;

    // error (we want Iuti to be at or below Imax)
    float e = Imax - Iuti_f;

    // incremental PI
    float delta_u = Kp*(e - e_prev) + Ki*Ts*e ;   // changed this to  ->  float delta_u = Kp*e + Ki*Ts*e;
    float u = u_prev + delta_u;

    // clamp to bounds
    if (u > Imax) u = Imax;
    if (u < MIN_ISET) u = MIN_ISET;

    // rate limit
    if (u > u_prev + RATE_LIMIT) u = u_prev + RATE_LIMIT;
    if (u < u_prev - RATE_LIMIT) u = u_prev - RATE_LIMIT;

    // hysteresis: only send command if big enough change
    if (fabs(u - Iset) >= HYSTERESIS) {
        request_set_current(u); // command EV charger
        Iset = u;
    } else {
        // no action
    }

    // store states
    e_prev = e;
    u_prev = u;
}
