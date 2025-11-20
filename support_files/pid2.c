error = Imax - Ilb
P_term = Kp × error
I_term = I_term + Ki × error × dt
D_term = Kd × (error - error_prev) / dt

u = P_term + I_term + D_term
Iset_adjusted = Iset + u

// Anti-windup for I_term
