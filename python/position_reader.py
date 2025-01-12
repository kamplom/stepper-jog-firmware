import serial
import struct
import time
import threading
import glob
from serial.serialutil import SerialException
from getpass import getpass

def find_acm_ports():
    return glob.glob('/dev/ttyACM*')

def reconnect_serial():
    last_working_port = None
    while not exit_flag.is_set():
        try:
            # Try last working port first
            if last_working_port and last_working_port in find_acm_ports():
                ports = [last_working_port]
            else:
                ports = find_acm_ports()
            
            for port in ports:
                try:
                    ser = serial.Serial(
                        port=port,
                        baudrate=115200,
                        timeout=1
                    )
                    print(f"\nDevice connected on {port}")
                    last_working_port = port
                    return ser
                except SerialException:
                    continue
            
            print("\rWaiting for device...", end='', flush=True)
            time.sleep(1)
            
        except SerialException:
            time.sleep(1)
    return None

def bytes_to_uint32(b1, b2, b3):
    # Reconstruct 24-bit number from 3 bytes
    return (b1 << 16) | (b2 << 8) | b3

def read_position():
    global ser
    while not exit_flag.is_set():
        try:
            if ser.in_waiting:
                byte = ser.read()
                if byte == b'@':
                    data = ser.read(3)
                    if len(data) == 3:
                        position_um = bytes_to_uint32(data[0], data[1], data[2])
                        position_mm = position_um / 1000.0
                        print(f"\rPosition: {position_mm:.3f} mm", end='', flush=True)
            time.sleep(0.001)
        except SerialException:
            print("\nDevice disconnected")
            ser = reconnect_serial()

def send_commands():
    global ser
    while not exit_flag.is_set():
        try:
            cmd = getpass(prompt='')
            if cmd.lower() == 'q':
                exit_flag.set()
                break
            ser.write(cmd.encode() + b'\r')
        except SerialException:
            time.sleep(0.01)  # Wait for reconnection

try:
    exit_flag = threading.Event()
    ser = reconnect_serial()
    if ser:
        print("Reading position data from /dev/ttyACM0...")
        print("Commands are hidden. Type 'q' to quit")
        
        # Start reader and command threads
        reader_thread = threading.Thread(target=read_position)
        command_thread = threading.Thread(target=send_commands)
        
        reader_thread.start()
        command_thread.start()
        
        # Wait for threads to complete
        command_thread.join()
        reader_thread.join()

except KeyboardInterrupt:
    exit_flag.set()

finally:
    if ser:
        ser.close()
    print("\nExiting...")