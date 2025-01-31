import serial
import time
import json
import os
import re
import logging
from tkinter import filedialog
import tkinter as tk
from tkinter import ttk

def parse_settings_enum(settings_h_path):
    """Parse settings.h to extract enum values and names"""
    enum_dict = {}
    try:
        with open(settings_h_path, 'r') as f:
            content = f.read()
            enum_block = re.search(r'typedef\s+enum\s*{(.*?)}\s*setting_id_t', content, re.DOTALL)
            if enum_block:
                for line in enum_block.group(1).split('\n'):
                    match = re.search(r'^\s*(Setting_\w+)\s*(?:=\s*(\d+))?,?', line.strip())
                    if match:
                        name, value = match.groups()
                        if value is None:
                            value = str(len(enum_dict))
                        enum_dict[int(value)] = name
    except Exception as e:
        logging.error(f"Error parsing settings enum: {e}")
    return enum_dict

def parse_settings_details(settings_c_path):
    """Parse settings.c to extract setting details"""
    details_dict = {}
    try:
        with open(settings_c_path, 'r') as f:
            content = f.read()
            details_block = re.search(r'const\s+setting_detail_t\s+setting_detail\[\s*\]\s*=\s*{(.*?)};', content, re.DOTALL)
            if details_block:
                for line in details_block.group(1).split('\n'):
                    match = re.search(r'{(Setting_\w+)\s*,\s*"([^"]+)"\s*,\s*(?:Unit_)?(\w+)', line)
                    if match:
                        enum_name, key, unit = match.groups()
                        details_dict[enum_name] = {
                            'key': key,
                            'unit': unit
                        }
    except Exception as e:
        logging.error(f"Error parsing settings details: {e}")
    return details_dict

def generate_settings_dict(main_folder='../main'):
    """Generate SETTINGS_DETAILS dictionary from source files"""
    settings_h = os.path.join(main_folder, 'settings.h')
    settings_c = os.path.join(main_folder, 'settings.c')
    
    # Parse both files
    enums = parse_settings_enum(settings_h)
    details = parse_settings_details(settings_c)
    
    # Combine information
    settings_details = {}
    for id, enum_name in enums.items():
        if enum_name in details:
            settings_details[id] = {
                'name': details[enum_name]['key'],
                'unit': details[enum_name]['unit'],
                'description': f"{enum_name} setting"
            }
    
    return settings_details

# Generate settings dictionary on module import
try:
    SETTINGS_DETAILS = generate_settings_dict()
except Exception as e:
    logging.error(f"Failed to generate settings dictionary: {e}")
    # Fallback to existing dictionary if generation fails
    SETTINGS_DETAILS = {
        0: {"name": "Stepsmm", "unit": "steps/mm", "description": "Steps per millimeter"},
        # ... rest of your existing dictionary ...
    }

class SettingsEditor(tk.Toplevel):
    def __init__(self, parent, settings):
        super().__init__(parent)
        self.settings = settings
        self.title("Settings Editor")
        
        # Create treeview
        self.tree = ttk.Treeview(self, columns=('ID', 'Name', 'Value', 'Unit', 'Description'), 
                                show='headings')
        
        # Define columns
        self.tree.heading('ID', text='ID')
        self.tree.heading('Name', text='Name')
        self.tree.heading('Value', text='Value')
        self.tree.heading('Unit', text='Unit')
        self.tree.heading('Description', text='Description')
        
        # Column widths
        self.tree.column('ID', width=50)
        self.tree.column('Name', width=150)
        self.tree.column('Value', width=100)
        self.tree.column('Unit', width=100)
        self.tree.column('Description', width=300)
        
        self.tree.pack(padx=10, pady=10, fill='both', expand=True)
        
        # Buttons
        button_frame = ttk.Frame(self)
        button_frame.pack(pady=5)
        
        ttk.Button(button_frame, text="Save Changes", command=self.save_changes).pack(side='left', padx=5)
        ttk.Button(button_frame, text="Close", command=self.destroy).pack(side='left', padx=5)
        
        # Bind double-click event
        self.tree.bind('<Double-1>', self.edit_value)
        
        self.load_settings()

    def load_settings(self):
        """Load settings into treeview"""
        for item in self.tree.get_children():
            self.tree.delete(item)
            
        for id, details in SETTINGS_DETAILS.items():
            value = self.settings.values.get(id, '')
            self.tree.insert('', 'end', values=(id, 
                                              details['name'],
                                              value,
                                              details['unit'],
                                              details['description']))

    def edit_value(self, event):
        """Handle double-click on value"""
        item = self.tree.selection()[0]
        column = self.tree.identify_column(event.x)
        
        # Only allow editing the value column
        if column == '#3':  # Value column
            x, y, w, h = self.tree.bbox(item, column)
            
            # Create entry widget
            entry = ttk.Entry(self.tree, width=20)
            entry.place(x=x, y=y, width=w, height=h)
            entry.insert(0, self.tree.set(item, column))
            entry.select_range(0, 'end')
            entry.focus()
            
            def save_value(event):
                """Save the edited value"""
                value = entry.get()
                try:
                    id = int(self.tree.item(item)['values'][0])
                    self.settings.values[id] = int(value)
                    self.tree.set(item, column, value)
                except ValueError:
                    pass  # Invalid input - ignore
                entry.destroy()
                
            entry.bind('<Return>', save_value)
            entry.bind('<FocusOut>', lambda e: entry.destroy())

    def save_changes(self):
        """Save changes to file"""
        self.settings.save_to_file()

class Settings:
    def __init__(self, port='/dev/ttyACM0', baud=115200):
        self.port = port
        self.baud = baud
        self.values = {}
        self.settings_file = 'machine_settings.json'
        
    def connect(self):
        try:
            self.ser = serial.Serial(self.port, self.baud, timeout=1)
            return True
        except serial.SerialException as e:
            print(f"Error connecting to {self.port}: {e}")
            return False
            
    def read_settings(self):
        if not hasattr(self, 'ser'):
            return False
            
        # Clear input buffer
        self.ser.reset_input_buffer()
        
        # Send request character
        self.ser.write(b'$\r')
        time.sleep(2)  # Wait for response
        
        # Read and parse response
        start_time = time.time()
        while time.time() - start_time < 10:  # Loop for up to 10 seconds
            try:
                line = self.ser.readline().decode('utf-8').strip()
                # Skip lines starting with '>'
                if not line or line.startswith('>'):
                    continue
                if '=' in line:
                    key, value = line.split('=')
                    self.values[int(key)] = int(value)
            except UnicodeDecodeError:
                # Skip lines that can't be decoded
                continue
           
                
    def save_to_file(self):
        """Save settings to JSON file with file dialog"""
        root = tk.Tk()
        root.withdraw()
        
        filename = filedialog.asksaveasfilename(
            title="Save settings file",
            defaultextension=".json",
            filetypes=[("JSON files", "*.json"), ("All files", "*.*")],
            initialdir="."
        )
        
        if not filename:
            print("Save canceled")
            return False
            
        try:
            with open(filename, 'w') as f:
                json.dump(self.values, f, indent=4)
            print(f"Settings saved to: {filename}")
            return True
        except Exception as e:
            print(f"Error saving settings: {e}")
            return False
            
    def load_from_file(self):
        """Load settings from JSON file with file dialog"""
        # Create and hide root window
        root = tk.Tk()
        root.withdraw()
        
        # Show file dialog
        filename = filedialog.askopenfilename(
            title="Select settings file",
            filetypes=[("JSON files", "*.json"), ("All files", "*.*")],
            initialdir="."
        )
        
        # Check if user canceled
        if not filename:
            print("File selection canceled")
            return False
            
        try:
            with open(filename, 'r') as f:
                data = json.load(f)
                self.values = {int(k): v for k, v in data.items()}
            print(f"Settings loaded from: {filename}")
            return True
        except Exception as e:
            print(f"Error loading settings: {e}")
            return False

    def close(self):
        if hasattr(self, 'ser'):
            self.ser.close()

    def print_settings(self):
        """Print all settings in a formatted table"""
        if not self.values:
            print("\nNo settings loaded.")
            return
            
        print("\nCurrent Settings:")
        print("-" * 80)
        print("ID  | Name            | Value    | Unit     | Description")
        print("-" * 80)
        
        # Sort integer keys directly
        for key in sorted(self.values.keys()):
            if key not in SETTINGS_DETAILS:
                continue
            detail = SETTINGS_DETAILS[key]
            print(f"{key:3d} | {detail['name']:<15} | {self.values[key]:8d} | {detail['unit']:<8} | {detail['description']}")
        print("-" * 80)

    def show_editor(self):
        """Show the settings editor GUI"""
        root = tk.Tk()
        root.withdraw()  # Hide the root window
        editor = SettingsEditor(root, self)
        editor.mainloop()

    def write_setting_to_device(self, id, value):
        """Write single setting to device"""
        if not hasattr(self, 'ser'):
            if not self.connect():
                return False
        try:
            command = f"${id}={value}\r"
            self.ser.write(command.encode())
            time.sleep(0.1)  # Wait for processing
            return True
        except Exception as e:
            print(f"Error writing setting {id}: {e}")
            return False
            
    def write_all_to_device(self):
        """Write all settings to device"""
        if not self.values:
            print("No settings to write")
            return False
            
        print("\nWriting settings to device...")
        success = True
        time.sleep(4)
        for id, value in self.values.items():
            print(f"Writing ${id}={value}")
            if not self.write_setting_to_device(id, value):
                success = False
                break
                
        if hasattr(self, 'ser'):
            self.close()
            
        return success

def show_menu():
    """Display menu options"""
    print("\nSettings Manager")
    print("1. Read settings from device")
    print("2. Load settings from file")
    print("3. Save settings to file")
    print("4. Print current settings")
    print("5. Edit settings (GUI)")
    print("6. Write settings to device")
    print("7. Exit")
    return input("Select option (1-7): ")

if __name__ == "__main__":
    settings = Settings()
    
    while True:
        choice = show_menu()
        
        if choice == '1':
            if settings.connect():
                settings.read_settings()
                settings.print_settings()
                settings.close()
        elif choice == '2':
            settings.load_from_file()
        elif choice == '3':
            settings.save_to_file()
        elif choice == '4':
            settings.print_settings()
        elif choice == '5':
            settings.show_editor()
        elif choice == '6':
            confirm = input("Write all settings to device? (y/n): ")
            if confirm.lower() == 'y':
                settings.write_all_to_device()
        elif choice == '7':
            print("Exiting...")
            break
        else:
            print("Invalid option, please try again")