import serial
import glob
from serial.serialutil import SerialException

class SerialHandler:
    STATE_BITS = {
        (1 << 0): 'ALERT',
        (1 << 1): 'IDLE',
        (1 << 2): 'JOGGING',
        (1 << 3): 'HOMING',
        (1 << 4): 'WHEEL',
        (1 << 5): 'MLOCKED'  # Changed from 'LOCKED' to 'MLOCKED' to match firmware
    }
    
    def __init__(self):
        self.ser = None
        self.last_working_port = None

    def bytes_to_uint32(self, b1, b2, b3):
        return (b1 << 16) | (b2 << 8) | b3

    def bytes_to_uint16(self, b1, b2):
        return (b1 << 8) | b2
        
    def parse_status(self, state_byte):
        states = []
        for bit, label in self.STATE_BITS.items():
            if state_byte & bit:
                states.append(label)
        return ' | '.join(states) if states else 'UNKNOWN'
        
    def connect(self):
        ports = [self.last_working_port] if self.last_working_port else glob.glob('/dev/ttyACM*')
        for port in ports:
            try:
                self.ser = serial.Serial(port=port, baudrate=115200, timeout=1)
                self.last_working_port = port
                return True
            except SerialException:
                continue
        return False
        
    def read_data(self):
        try:
            if self.ser and self.ser.in_waiting:
                byte = self.ser.read()
                if (byte == b'@'):
                    data = self.ser.read(3)
                    if len(data) == 3:
                        position_um = self.bytes_to_uint32(data[0], data[1], data[2])
                        return {"type": "position", "value": position_um / 1000.0}
                elif byte == b'>':
                    data = self.ser.read(3)
                    if len(data) == 3:
                        state = data[0]
                        max_pos = self.bytes_to_uint16(data[1], data[2])
                        return {
                            "type": "status",
                            "state": self.parse_status(state),
                            "max_position": max_pos
                        }
        except SerialException:
            self.connect()
        return None
    
    def send_command(self, cmd):
        try:
            if self.ser:
                self.ser.write(cmd.encode() + b'\r')
                return True
        except SerialException:
            self.connect()
        return False