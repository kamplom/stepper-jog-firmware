def calc_steps_mm(microsteps=8):
    # Motor constants
    STEP_ANGLE = 1.8  # degrees
    STEPS_PER_REV = 360 / STEP_ANGLE  # 200 steps/rev
    
    # Belt drive constants
    BELT_PITCH = 3  # mm (3GT)
    PULLEY_TEETH = 20
    
    # Calculations
    total_steps = STEPS_PER_REV * microsteps
    movement_per_rev = BELT_PITCH * PULLEY_TEETH
    steps_per_mm = total_steps / movement_per_rev
    
    return steps_per_mm

def calc_pulses_mm(pulses_per_rev):
    # Motor constants
    STEPS_PER_REV = pulses_per_rev  # 200 steps/rev
    
    # Belt drive constants
    BELT_PITCH = 3  # mm (3GT)
    PULLEY_TEETH = 20
    
    # Calculations
    total_steps = STEPS_PER_REV 
    movement_per_rev = BELT_PITCH * PULLEY_TEETH
    steps_per_mm = total_steps / movement_per_rev
    
    return steps_per_mm

if __name__ == "__main__":
    steps_mm = calc_pulses_mm(1000)
    print(f"Steps per mm: {steps_mm:.2f}")
    pulses_mm = calc_pulses_mm(4000)
    print(f"Steps per mm: {pulses_mm:.2f}")
    steps_per_pulse = steps_mm / pulses_mm
    print(f"Steps per pulse: {steps_per_pulse:.2f}")