#!/usr/bin/env python3
"""
Bronco Controls Backup & Restore - GUI Application
User-friendly Windows interface for device management
"""

import sys
import os
import threading
from pathlib import Path
import tkinter as tk
from tkinter import ttk, messagebox, scrolledtext, filedialog
from datetime import datetime

# Add parent directory to path to import backup manager
sys.path.insert(0, str(Path(__file__).parent))
from backup_restore_manager import BackupRestoreManager


class BackupRestoreGUI:
    def __init__(self, root):
        self.root = root
        self.root.title("Bronco Controls - Backup & Restore Manager")
        self.root.geometry("900x700")
        self.root.resizable(True, True)
        
        # Configure style
        self.setup_styles()
        
        # Manager instance
        self.manager = BackupRestoreManager()
        self.current_version = self.manager.get_current_version()
        
        # Build UI
        self.create_widgets()
        
        # Load initial data
        self.refresh_backups()
    
    def setup_styles(self):
        """Configure ttk styles"""
        style = ttk.Style()
        style.theme_use('clam')
        
        # Button styles
        style.configure('Action.TButton', font=('Segoe UI', 10, 'bold'), 
                       padding=10, background='#667eea', foreground='white')
        style.map('Action.TButton', background=[('active', '#764ba2')])
        
        style.configure('Success.TButton', font=('Segoe UI', 10), 
                       padding=8, background='#28a745', foreground='white')
        style.map('Success.TButton', background=[('active', '#218838')])
        
        style.configure('Danger.TButton', font=('Segoe UI', 10), 
                       padding=8, background='#dc3545', foreground='white')
        style.map('Danger.TButton', background=[('active', '#c82333')])
    
    def create_widgets(self):
        """Create all GUI widgets"""
        # Main container
        main_frame = ttk.Frame(self.root, padding="10")
        main_frame.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        self.root.columnconfigure(0, weight=1)
        self.root.rowconfigure(0, weight=1)
        main_frame.columnconfigure(0, weight=1)
        main_frame.rowconfigure(2, weight=1)
        
        # Header
        header_frame = ttk.Frame(main_frame)
        header_frame.grid(row=0, column=0, sticky=(tk.W, tk.E), pady=(0, 10))
        header_frame.columnconfigure(1, weight=1)
        
        ttk.Label(header_frame, text="🚗 Bronco Controls", 
                 font=('Segoe UI', 18, 'bold')).grid(row=0, column=0, sticky=tk.W)
        
        version_label = ttk.Label(header_frame, text=f"Current Version: v{self.current_version}", 
                                 font=('Segoe UI', 12))
        version_label.grid(row=0, column=1, sticky=tk.E)
        
        # Device info
        info_frame = ttk.LabelFrame(main_frame, text="Device Information", padding="10")
        info_frame.grid(row=1, column=0, sticky=(tk.W, tk.E), pady=(0, 10))
        info_frame.columnconfigure(1, weight=1)
        
        ttk.Label(info_frame, text="Port:").grid(row=0, column=0, sticky=tk.W, pady=2)
        self.port_var = tk.StringVar(value="COM5")
        port_combo = ttk.Combobox(info_frame, textvariable=self.port_var, 
                                  values=['COM3', 'COM4', 'COM5', 'COM6', 'COM7', 'COM8'],
                                  width=15, state='readonly')
        port_combo.grid(row=0, column=1, sticky=tk.W, pady=2)
        
        ttk.Label(info_frame, text="Device:").grid(row=0, column=2, sticky=tk.W, padx=(20, 5), pady=2)
        ttk.Label(info_frame, text="ESP32-S3-Touch-LCD-7", 
                 font=('Segoe UI', 9, 'bold')).grid(row=0, column=3, sticky=tk.W, pady=2)
        
        # Notebook for tabs
        notebook = ttk.Notebook(main_frame)
        notebook.grid(row=2, column=0, sticky=(tk.W, tk.E, tk.N, tk.S), pady=(0, 10))
        
        # Tab 1: Backup
        backup_tab = ttk.Frame(notebook, padding="10")
        notebook.add(backup_tab, text="📦 Backup")
        self.create_backup_tab(backup_tab)
        
        # Tab 2: Restore
        restore_tab = ttk.Frame(notebook, padding="10")
        notebook.add(restore_tab, text="♻️ Restore")
        self.create_restore_tab(restore_tab)
        
        # Tab 3: Test
        test_tab = ttk.Frame(notebook, padding="10")
        notebook.add(test_tab, text="🧪 Test")
        self.create_test_tab(test_tab)
        
        # Console output
        console_frame = ttk.LabelFrame(main_frame, text="Console Output", padding="5")
        console_frame.grid(row=3, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        console_frame.columnconfigure(0, weight=1)
        console_frame.rowconfigure(0, weight=1)
        
        self.console = scrolledtext.ScrolledText(console_frame, height=12, 
                                                 font=('Consolas', 9), 
                                                 bg='#1e1e1e', fg='#d4d4d4',
                                                 insertbackground='white')
        self.console.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        
        # Redirect stdout to console
        sys.stdout = ConsoleRedirector(self.console)
        
        print(f"Bronco Controls Backup & Restore Manager")
        print(f"Current Version: v{self.current_version}")
        print(f"Ready.\n")
    
    def create_backup_tab(self, parent):
        """Create backup tab content"""
        parent.columnconfigure(0, weight=1)
        
        # Info
        info_text = (
            "Create a complete backup of your ESP32-S3 device.\n\n"
            "This includes:\n"
            "• Bootloader and partition table\n"
            "• Application firmware (both slots)\n"
            "• NVS (settings, WiFi credentials, etc.)\n"
            "• OTA data\n"
            "• File system (SPIFFS)\n\n"
            "The version number will be auto-incremented."
        )
        
        info_label = ttk.Label(parent, text=info_text, justify=tk.LEFT, 
                              font=('Segoe UI', 9))
        info_label.grid(row=0, column=0, sticky=(tk.W, tk.E), pady=(0, 20))
        
        # Version preview and type selection
        version_frame = ttk.Frame(parent)
        version_frame.grid(row=1, column=0, sticky=(tk.W, tk.E), pady=(0, 10))
        
        ttk.Label(version_frame, text='Version Type:', 
                 font=('Segoe UI', 10, 'bold')).grid(row=0, column=0, sticky=tk.W)
        
        # Version type radio buttons
        self.version_type_var = tk.StringVar(value='build')
        
        radio_frame = ttk.Frame(version_frame)
        radio_frame.grid(row=0, column=1, sticky=tk.W, padx=(10, 0))
        
        ttk.Radiobutton(radio_frame, text='Build (x.x.N)', 
                       variable=self.version_type_var, value='build',
                       command=self.update_version_preview).grid(row=0, column=0, padx=5)
        ttk.Radiobutton(radio_frame, text='Minor (x.N.0)', 
                       variable=self.version_type_var, value='minor',
                       command=self.update_version_preview).grid(row=0, column=1, padx=5)
        ttk.Radiobutton(radio_frame, text='MAJOR (N.0.0) - FULL BACKUP', 
                       variable=self.version_type_var, value='major',
                       command=self.update_version_preview).grid(row=0, column=2, padx=5)
        
        # Next version display
        preview_frame = ttk.Frame(version_frame)
        preview_frame.grid(row=1, column=0, columnspan=2, sticky=tk.W, pady=(10, 0))
        
        ttk.Label(preview_frame, text='Next Version:', 
                 font=('Segoe UI', 10, 'bold')).grid(row=0, column=0, sticky=tk.W)
        
        self.next_version_label = ttk.Label(preview_frame, text=f"v{self.get_next_version('build')}", 
                 font=('Segoe UI', 14, 'bold'), 
                 foreground='#667eea')
        self.next_version_label.grid(row=0, column=1, sticky=tk.W, padx=(10, 0))
        
        # Info about releases
        info_text = (
            "⚠️ MAJOR: Full device backup (all sectors) - requires USB connection\n"
            "ℹ️ ALL versions automatically push to Git + copy firmware for OTA"
        )
        release_info = ttk.Label(parent, text=info_text,
                              font=('Segoe UI', 8), foreground='#ffc107', justify=tk.LEFT)
        release_info.grid(row=2, column=0, sticky=tk.W, pady=(5, 20))
        
        # Backup button
        backup_btn = ttk.Button(parent, text='🔽 Create Backup', 
                               style='Action.TButton',
                               command=self.start_backup)
        backup_btn.grid(row=3, column=0, pady=10)
    
    def create_restore_tab(self, parent):
        """Create restore tab content"""
        parent.columnconfigure(0, weight=1)
        parent.rowconfigure(1, weight=1)
        
        # Info
        info_text = (
            "⚠️ WARNING: Restore will COMPLETELY ERASE the device!\n\n"
            "Select a backup below to restore your device to that exact state.\n"
            "All current data will be lost."
        )
        
        info_label = ttk.Label(parent, text=info_text, justify=tk.LEFT, 
                              font=('Segoe UI', 9), foreground='#dc3545')
        info_label.grid(row=0, column=0, sticky=(tk.W, tk.E), pady=(0, 10))
        
        # Backups list
        list_frame = ttk.LabelFrame(parent, text="Available Backups", padding="10")
        list_frame.grid(row=1, column=0, sticky=(tk.W, tk.E, tk.N, tk.S), pady=(0, 10))
        list_frame.columnconfigure(0, weight=1)
        list_frame.rowconfigure(0, weight=1)
        
        # Treeview for backups
        columns = ('Version', 'Date', 'Size')
        self.backup_tree = ttk.Treeview(list_frame, columns=columns, show='tree headings', height=8)
        
        self.backup_tree.heading('#0', text='Backup Name')
        self.backup_tree.heading('Version', text='Version')
        self.backup_tree.heading('Date', text='Date')
        self.backup_tree.heading('Size', text='Size (MB)')
        
        self.backup_tree.column('#0', width=250)
        self.backup_tree.column('Version', width=80, anchor=tk.CENTER)
        self.backup_tree.column('Date', width=150, anchor=tk.CENTER)
        self.backup_tree.column('Size', width=80, anchor=tk.E)
        
        self.backup_tree.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        
        # Scrollbar
        scrollbar = ttk.Scrollbar(list_frame, orient=tk.VERTICAL, command=self.backup_tree.yview)
        scrollbar.grid(row=0, column=1, sticky=(tk.N, tk.S))
        self.backup_tree.configure(yscrollcommand=scrollbar.set)
        
        # Buttons
        btn_frame = ttk.Frame(parent)
        btn_frame.grid(row=2, column=0, pady=10)
        
        ttk.Button(btn_frame, text="🔄 Refresh List", 
                  command=self.refresh_backups).grid(row=0, column=0, padx=5)
        
        ttk.Button(btn_frame, text="📂 Open Backup Folder", 
                  command=self.open_backups_folder).grid(row=0, column=1, padx=5)
        
        ttk.Button(btn_frame, text="♻️ Restore Selected", 
                  style='Danger.TButton',
                  command=self.start_restore).grid(row=0, column=2, padx=5)
    
    def create_test_tab(self, parent):
        """Create test tab content"""
        parent.columnconfigure(0, weight=1)
        
        # Info
        info_text = (
            "🧪 Full Test Cycle\n\n"
            "This will perform a complete backup-erase-restore cycle:\n\n"
            "1. Create a new backup (version incremented)\n"
            "2. Completely erase the device flash\n"
            "3. Restore from the newly created backup\n\n"
            "This verifies that the backup/restore process works correctly.\n"
            "The device will be in the same state after the test."
        )
        
        info_label = ttk.Label(parent, text=info_text, justify=tk.LEFT, 
                              font=('Segoe UI', 10))
        info_label.grid(row=0, column=0, sticky=(tk.W, tk.E), pady=(0, 30))
        
        # Warning
        warning_frame = ttk.Frame(parent, relief=tk.RIDGE, borderwidth=2)
        warning_frame.grid(row=1, column=0, sticky=(tk.W, tk.E), pady=(0, 20))
        warning_frame.columnconfigure(0, weight=1)
        
        ttk.Label(warning_frame, text="⚠️ IMPORTANT", 
                 font=('Segoe UI', 11, 'bold'), 
                 foreground='#ffc107', 
                 padding=10).grid(row=0, column=0)
        
        ttk.Label(warning_frame, 
                 text="This process will temporarily erase your device.\nMake sure you have time for the full cycle (5-10 minutes).",
                 font=('Segoe UI', 9),
                 justify=tk.CENTER,
                 padding=(10, 0, 10, 10)).grid(row=1, column=0)
        
        # Test button
        test_btn = ttk.Button(parent, text="▶️ Run Full Test Cycle", 
                             style='Action.TButton',
                             command=self.start_test)
        test_btn.grid(row=2, column=0, pady=20)
    
    def get_next_version(self, increment_type='build'):
        """Calculate next version number based on increment type"""
        current = self.current_version
        parts = [int(p) for p in current.split('.')]
        
        if increment_type == 'major':
            parts[0] += 1
            parts[1] = 0
            parts[2] = 0
        elif increment_type == 'minor':
            parts[1] += 1
            parts[2] = 0
        else:  # build
            parts[2] += 1
        
        return '.'.join(str(p) for p in parts)
    
    def update_version_preview(self):
        """Update the version preview when type changes"""
        version_type = self.version_type_var.get()
        next_ver = self.get_next_version(version_type)
        self.next_version_label.config(text=f"v{next_ver}")
        
        # Change color for major releases
        if version_type == 'major':
            self.next_version_label.config(foreground='#dc3545')  # Red for major
        else:
            self.next_version_label.config(foreground='#667eea')  # Blue for normal
    
    def refresh_backups(self):
        """Refresh the backups list"""
        # Clear existing items
        for item in self.backup_tree.get_children():
            self.backup_tree.delete(item)
        
        # Get backups
        backups = self.manager.list_backups()
        
        for backup in backups:
            version = backup['version']
            date = backup['date'][:19].replace('T', ' ')
            size_mb = backup['size'] / (1024 * 1024)
            name = backup['path'].name
            
            self.backup_tree.insert('', 'end', text=name, 
                                   values=(version, date, f"{size_mb:.1f}"),
                                   tags=(str(backup['path']),))
    
    def open_backups_folder(self):
        """Open backups folder in explorer"""
        import subprocess
        subprocess.Popen(f'explorer "{self.manager.backups_dir}"')
    
    def start_backup(self):
        """Start backup in background thread"""
        def backup_thread():
            try:
                self.disable_buttons()
                self.manager.port = self.port_var.get()
                increment_type = self.version_type_var.get()
                
                # Show info for major release
                if increment_type == 'major':
                    messagebox.showinfo("Major Release", 
                                       "MAJOR VERSION RELEASE\n\n"
                                       "• Full device backup (all sectors)\n"
                                       "• Requires USB connection\n"
                                       "• Auto-pushes to Git\n\n"
                                       "This will take several minutes...")
                
                backup_folder, version = self.manager.backup_device(increment_type=increment_type)
                
                self.refresh_backups()
                self.current_version = version
                
                success_msg = f"Backup completed!\nVersion: v{version}\n\nLocation:\n{backup_folder}"
                if increment_type == 'major':
                    success_msg += "\n\n✓ FULL device backup (all sectors)\n✓ Source code + firmware pushed to Git"
                else:
                    success_msg += "\n\n✓ Source code + firmware pushed to Git"
                
                messagebox.showinfo("Success", success_msg)
            except Exception as e:
                messagebox.showerror("Error", f"Backup failed:\n{str(e)}")
            finally:
                self.enable_buttons()
        
        threading.Thread(target=backup_thread, daemon=True).start()
    
    def start_restore(self):
        """Start restore in background thread"""
        selection = self.backup_tree.selection()
        if not selection:
            messagebox.showwarning("No Selection", "Please select a backup to restore")
            return
        
        backup_path = self.backup_tree.item(selection[0])['tags'][0]
        
        # Confirm
        result = messagebox.askyesno("Confirm Restore", 
                                     f"⚠️ WARNING ⚠️\n\n"
                                     f"This will COMPLETELY ERASE your device and restore:\n"
                                     f"{Path(backup_path).name}\n\n"
                                     f"ALL CURRENT DATA WILL BE LOST!\n\n"
                                     f"Continue?",
                                     icon='warning')
        if not result:
            return
        
        def restore_thread():
            try:
                self.disable_buttons()
                self.manager.port = self.port_var.get()
                success = self.manager.restore_device(backup_path)
                
                if success:
                    messagebox.showinfo("Success", "Restore completed!\n\nDevice has been rebooted.")
                else:
                    messagebox.showerror("Error", "Restore failed. Check console for details.")
            except Exception as e:
                messagebox.showerror("Error", f"Restore failed:\n{str(e)}")
            finally:
                self.enable_buttons()
        
        threading.Thread(target=restore_thread, daemon=True).start()
    
    def start_test(self):
        """Start full test cycle"""
        result = messagebox.askyesno("Confirm Test", 
                                     "⚠️ FULL TEST CYCLE ⚠️\n\n"
                                     "This will:\n"
                                     "1. Backup current device\n"
                                     "2. Erase all data\n"
                                     "3. Restore from backup\n\n"
                                     "This takes 5-10 minutes.\n\n"
                                     "Continue?",
                                     icon='warning')
        if not result:
            return
        
        def test_thread():
            try:
                self.disable_buttons()
                self.manager.port = self.port_var.get()
                
                # Backup
                print("\n[TEST 1/3] Creating backup...")
                backup_folder, version = self.manager.backup_device()
                if not backup_folder:
                    raise Exception("Backup failed")
                
                # Erase
                print("\n[TEST 2/3] Erasing device...")
                import subprocess
                cmd = ['python', '-m', 'esptool', '--chip', 'esp32s3', 
                       '--port', self.manager.port, 'erase_flash']
                result = subprocess.run(cmd)
                if result.returncode != 0:
                    raise Exception("Erase failed")
                
                # Restore
                print("\n[TEST 3/3] Restoring device...")
                success = self.manager.restore_device(backup_folder)
                
                if success:
                    self.refresh_backups()
                    messagebox.showinfo("Test Complete", 
                                       "✓ Full test cycle completed successfully!\n\n"
                                       "Device has been:\n"
                                       "• Backed up\n"
                                       "• Erased\n"
                                       "• Restored\n\n"
                                       "Everything is working correctly!")
                else:
                    raise Exception("Restore failed")
                    
            except Exception as e:
                messagebox.showerror("Test Failed", f"Test cycle failed:\n{str(e)}")
            finally:
                self.enable_buttons()
        
        threading.Thread(target=test_thread, daemon=True).start()
    
    def disable_buttons(self):
        """Disable all action buttons during operations"""
        self.root.config(cursor="wait")
    
    def enable_buttons(self):
        """Re-enable all action buttons"""
        self.root.config(cursor="")


class ConsoleRedirector:
    """Redirect stdout to tkinter Text widget"""
    def __init__(self, text_widget):
        self.text_widget = text_widget
        self.original_stdout = sys.__stdout__
    
    def write(self, text):
        self.text_widget.insert(tk.END, text)
        self.text_widget.see(tk.END)
        self.text_widget.update_idletasks()
        # Also write to original stdout for debugging
        self.original_stdout.write(text)
    
    def flush(self):
        pass


def main():
    root = tk.Tk()
    app = BackupRestoreGUI(root)
    root.mainloop()


if __name__ == '__main__':
    main()
