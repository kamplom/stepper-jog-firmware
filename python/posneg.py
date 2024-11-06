import serial
import time
import re
import csv
import matplotlib.pyplot as plt

# Configuration
SERIAL_PORT = '/dev/ttyACM0'  # Change to your serial port
BAUD_RATE = 115200             # Adjust according to your device
TIMEOUT = 1                    # Timeout for reading from the serial port
COMMAND_H = 'h\r'              # Command to send before J command
COMMAND_J5 = 'J=X7\r'          # Command to send for the first sequence
COMMAND_J0 = 'J=X0\r'          # Command to send after receiving Iterations
LOG_FILE = 'data_log.csv'

# Initialize lists for logging
aux_positions_5 = []
sys_positions_5 = []
aux_velocities_5 = []
sys_velocities_5 = []
target_positions_5 = []        # New list for target positions when jogging with J=X5
target_positions_0 = []        # New list for target positions when jogging with J=X0
change_positions_5 = []        # New list for change positions when jogging with J=X5
change_positions_0 = []        # New list for change positions when jogging with J=X0
original_change_positions_5 = []  # New list for original change positions when jogging with J=X5
original_change_positions_0 = []  # New list for original change positions when jogging with J=X0
pos_func_positions_5 = []      # New list for pos_func positions when jogging with J=X5
pos_func_positions_0 = []      # New list for pos_func positions when jogging with J=X0

aux_positions_0 = []
sys_positions_0 = []
aux_velocities_0 = []
sys_velocities_0 = []

def main():
    # Open serial port
    with serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=TIMEOUT) as ser:
        time.sleep(2)  # Wait for the serial connection to initialize
        
        # Send the command 'h'
        ser.write(COMMAND_H.encode())
        time.sleep(5)  # Wait for 5 seconds

        # Send the command 'J=X5'
        ser.write(COMMAND_J5.encode())
        
        # Gather data for J=X5
        gather_data(ser, aux_positions_5, sys_positions_5, aux_velocities_5, sys_velocities_5,
                    target_positions_5, change_positions_5, original_change_positions_5, pos_func_positions_5)

        # Send the command 'J=X0' after receiving the Iterations line
        ser.write(COMMAND_J0.encode())
        gather_data(ser, aux_positions_0, sys_positions_0, aux_velocities_0, sys_velocities_0,
                    target_positions_0, change_positions_0, original_change_positions_0, pos_func_positions_0)

        # Adjust target positions for J=X5 using the first value of J=X0 target array
        if target_positions_0:
            first_target_j0 = target_positions_0[0]
            for i in range(len(target_positions_5)):
                target_positions_5[i] = first_target_j0 - target_positions_5[i]

            # Adjust change positions for J=X5
            first_change_j0 = change_positions_0[0] if change_positions_0 else 0
            for i in range(len(change_positions_5)):
                change_positions_5[i] = abs(change_positions_5[i])

            # Adjust original change positions for J=X5
            first_original_change_j0 = original_change_positions_0[0] if original_change_positions_0 else 0
            for i in range(len(original_change_positions_5)):
                original_change_positions_5[i] = abs(original_change_positions_5[i])

            # Adjust pos_func positions for J=X5
            first_pos_func_j0 = pos_func_positions_0[0] if pos_func_positions_0 else 0
            #for i in range(len(pos_func_positions_5)):
            #    pos_func_positions_5[i] = first_pos_func_j0 - pos_func_positions_5[i]

        # Save data to CSV
        save_to_csv(LOG_FILE)

        # Plot the results
        plot_data()

def gather_data(ser, aux_positions, sys_positions, aux_velocities, sys_velocities, 
                target_positions, change_positions, original_change_positions, pos_func_positions):
    while True:
        # Read a line from the serial port
        line = ser.readline().decode().strip()
        if not line:
            continue
        
        # Print each line received
        print(line)

        # Match auxiliary position (including negative values)
        aux_position_match = re.search(r'aux_pos:\s*(-?\d+)', line)
        if aux_position_match:
            aux_positions.append(float(aux_position_match.group(1)))
        
        # Match system position (including negative values)
        sys_position_match = re.search(r'sys\.status\.pos:\s*(-?\d+)', line)
        if sys_position_match:
            sys_positions.append(float(sys_position_match.group(1)))

        # Match auxiliary velocity (including negative values)
        aux_velocity_match = re.search(r'aux_vel:\s*(-?\d+)', line)
        if aux_velocity_match:
            aux_velocities.append(float(aux_velocity_match.group(1)))

        # Match system velocity (including negative values)
        sys_velocity_match = re.search(r'sys\.status\.vel:\s*(-?\d+)', line)
        if sys_velocity_match:
            sys_velocities.append(float(sys_velocity_match.group(1)))

        # Match target position when jogging
        target_position_match = re.search(r'Jogging:\s*target:\s*(-?\d+)', line)
        if target_position_match:
            target = float(target_position_match.group(1))
            target_positions.append(target)

        # Match change position when jogging
        change_position_match = re.search(r'Jogging:\s*change:\s*(-?\d+)', line)
        if change_position_match:
            change = float(change_position_match.group(1))
            change_positions.append(change)

        # Match original change position when jogging
        original_change_position_match = re.search(r'Jogging:\s*original_change:\s*(-?\d+)', line)
        if original_change_position_match:
            original_change = float(original_change_position_match.group(1))
            original_change_positions.append(original_change)

        # Match pos_func position when jogging
        pos_func_position_match = re.search(r'Jogging:\s*pos_func:\s*(-?\d+(\.\d+)?)', line)
        if pos_func_position_match:
            pos_func = float(pos_func_position_match.group(1))
            pos_func_positions.append(pos_func)

        # Check for termination line
        if 'Iterations' in line:
            break

def save_to_csv(filename):
    with open(filename, mode='w', newline='') as file:
        writer = csv.writer(file)
        writer.writerow(['Aux Position (J=X5)', 'Sys Position (J=X5)', 'Aux Velocity (J=X5)', 'Sys Velocity (J=X5)',
                         'Aux Position (J=X0)', 'Sys Position (J=X0)', 'Aux Velocity (J=X0)', 'Sys Velocity (J=X0)',
                         'Target Position (J=X5)', 'Target Position (J=X0)', 'Change Position (J=X5)', 'Change Position (J=X0)',
                         'Original Change Position (J=X5)', 'Original Change Position (J=X0)', 'Pos Func Position (J=X5)', 
                         'Pos Func Position (J=X0)'])
        max_length = max(len(aux_positions_5), len(sys_positions_5), len(aux_velocities_5), len(sys_velocities_5),
                         len(aux_positions_0), len(sys_positions_0), len(aux_velocities_0), len(sys_velocities_0),
                         len(target_positions_5), len(target_positions_0), len(change_positions_5), len(change_positions_0),
                         len(original_change_positions_5), len(original_change_positions_0), len(pos_func_positions_5), 
                         len(pos_func_positions_0))
        for i in range(max_length):
            row = [
                aux_positions_5[i] if i < len(aux_positions_5) else '',
                sys_positions_5[i] if i < len(sys_positions_5) else '',
                aux_velocities_5[i] if i < len(aux_velocities_5) else '',
                sys_velocities_5[i] if i < len(sys_velocities_5) else '',
                -aux_positions_0[i] if i < len(aux_positions_0) else '',
                -sys_positions_0[i] if i < len(sys_positions_0) else '',
                -aux_velocities_0[i] if i < len(aux_velocities_0) else '',
                -sys_velocities_0[i] if i < len(sys_velocities_0) else '',
                target_positions_5[i] if i < len(target_positions_5) else '',
                target_positions_0[i] if i < len(target_positions_0) else '',
                change_positions_5[i] if i < len(change_positions_5) else '',
                change_positions_0[i] if i < len(change_positions_0) else '',
                original_change_positions_5[i] if i < len(original_change_positions_5) else '',
                original_change_positions_0[i] if i < len(original_change_positions_0) else '',
                pos_func_positions_5[i] if i < len(pos_func_positions_5) else '',
                pos_func_positions_0[i] if i < len(pos_func_positions_0) else '',
            ]
            writer.writerow(row)

def plot_data():
    plt.figure(figsize=(12, 12))

    # Plot auxiliary and system positions for J=X5 and J=X0
    plt.subplot(6, 1, 1)
    plt.plot(aux_positions_5, label='Aux Position (J=X5)', color='red', linestyle='--')  # Dashed line for J=X5
    plt.plot(sys_positions_5, label='Sys Position (J=X5)', color='orange', linestyle='--')  # Dashed line for J=X5
    plt.plot([aux_positions_5[-1]-pos for pos in aux_positions_0], label='Aux Position (J=X0)', color='brown', linestyle='-')  # Solid line for J=X0
    plt.plot([sys_positions_5[-1]-pos for pos in sys_positions_0], label='Sys Position (J=X0)', color='purple', linestyle='-')  # Solid line for J=X0
    plt.xlabel('Sample Number')
    plt.ylabel('Position')
    plt.title('Position Over Time Comparison')
    plt.grid(True)
    plt.legend()

    # Plot target positions for J=X5 and J=X0
    plt.subplot(6, 1, 2)
    plt.plot(target_positions_5, label='Target Position (J=X5)', color='red', linestyle='--')  # Dashed line for J=X5
    plt.plot(target_positions_0, label='Target Position (J=X0)', color='orange', linestyle='-')  # Solid line for J=X0
    plt.xlabel('Sample Number')
    plt.ylabel('Target Position')
    plt.title('Target Position Over Time Comparison')
    plt.grid(True)
    plt.legend()

    # Plot auxiliary velocity and system velocity for J=X5 and J=X0
    plt.subplot(6, 1, 3)
    plt.plot(aux_velocities_5, label='Aux Velocity (J=X5)', color='r', linestyle='--')  # Dashed line for J=X5
    plt.plot(sys_velocities_5, label='Sys Velocity (J=X5)', color='orange', linestyle='--')  # Dashed line for J=X5
    plt.plot([-vel for vel in aux_velocities_0], label='Aux Velocity (J=X0)', color='brown', linestyle='-')  # Solid line for J=X0
    plt.plot([-vel for vel in sys_velocities_0], label='Sys Velocity (J=X0)', color='purple', linestyle='-')  # Solid line for J=X0
    plt.xlabel('Sample Number')
    plt.ylabel('Velocity')
    plt.title('Velocity Over Time Comparison')
    plt.grid(True)
    plt.legend()

    # Plot change data for J=X5 and J=X0
    plt.subplot(6, 1, 4)
    plt.plot(change_positions_5, label='Change Position (J=X5)', color='red', linestyle='--')  # Dashed line for J=X5
    plt.plot(change_positions_0, label='Change Position (J=X0)', color='brown', linestyle='-')  # Solid line for J=X0
    plt.xlabel('Sample Number')
    plt.ylabel('Change Position')
    plt.title('Change Position Over Time Comparison')
    plt.grid(True)
    plt.legend()

    # Plot original change data for J=X5 and J=X0
    plt.subplot(6, 1, 5)
    plt.plot(original_change_positions_5, label='Original Change Position (J=X5)', color='magenta', linestyle='--')  # Dashed line for J=X5
    plt.plot(original_change_positions_0, label='Original Change Position (J=X0)', color='black', linestyle='-')  # Solid line for J=X0
    plt.xlabel('Sample Number')
    plt.ylabel('Original Change Position')
    plt.title('Original Change Position Over Time Comparison')
    plt.grid(True)
    plt.legend()

    # Plot pos_func data for J=X5 and J=X0
    plt.subplot(6, 1, 6)
    plt.plot(pos_func_positions_5, label='Pos Func Position (J=X5)', color='cyan', linestyle='--')  # Dashed line for J=X5
    plt.plot(pos_func_positions_0, label='Pos Func Position (J=X0)', color='green', linestyle='-')  # Solid line for J=X0
    plt.xlabel('Sample Number')
    plt.ylabel('Pos Func Position')
    plt.title('Pos Func Position Over Time Comparison')
    plt.grid(True)
    plt.legend()

    plt.tight_layout()
    plt.show()

if __name__ == '__main__':
    main()
