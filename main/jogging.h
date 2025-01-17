/**
 * @brief Updates system velocity for the next step based on target and current velocity and position
 *
 */
void update_velocity(uint32_t target, int32_t *pos_ptr, int32_t *vel_ptr);
void update_velocity_exact(uint32_t target, int32_t *pos_ptr, int32_t *vel_ptr);
void cancel_jog();