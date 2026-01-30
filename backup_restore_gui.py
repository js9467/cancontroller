#!/usr/bin/env python3
"""
Bronco Controls Version & Backup Manager - GUI Application
User-friendly interface for version management, backups, and OTA updates
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
        self.root.title("Bronco Controls - Version & Backup Manager")
        self.root.geometry("1000x750")
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
        
        style.configure('Warning.TButton', font=('Segoe UI', 10), 
                       padding=8, background='#ffc107', foreground='black')
        style.map('Warning.TButton', background=[('active', '#e0a800')])
    
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
        
        # Tab 1: Version Update
        version_tab = ttk.Frame(notebook, padding="10")
        notebook.add(version_tab, text="📝 Version Update")
        self.create_version_tab(version_tab)
        
        # Tab 2: Full Backup
        backup_tab = ttk.Frame(notebook, padding="10")
        notebook.add(backup_tab, text="📦 Full Backup")
        self.create_backup_tab(backup_tab)
        
        # Tab 3: OTA Update
        ota_tab = ttk.Frame(notebook, padding="10")
        notebook.add(ota_tab, text="📡 OTA Update")
        self.create_ota_tab(ota_tab)
        
        # Tab 4: Restore
        restore_tab = ttk.Frame(notebook, padding="10")
        notebook.add(restore_tab, text="♻️ Restore")
        self.create_restore_tab(restore_tab)
        
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
        
        print(f"Bronco Controls Version & Backup Manager")
        print(f"Current Version: v{self.current_version}")
        print(f"GitHub: https://github.com/js9467/cancontroller/tree/master/versions")
        print(f"Ready.\n")
    
    def create_version_tab(self, parent):
        """Create version update tab (no device needed)"""
        parent.columnconfigure(0, weight=1)
        
        # Info
        info_text = (
            "📝 Create a Version Update\n\n"
            "This creates a lightweight version snapshot from your current project state.\n\n"
            "What happens:\n"
            "• Build number increments (e.g., 1.3.82 → 1.3.83)\n"
            "• Builds firmware from current source code\n"
            "• Creates firmware.bin for OTA updates\n"
            "• Uploads .bin to GitHub (OTA-capable)\n"
            "• For major versions (v2→v3), also creates .zip for USB install\n"
            "• Commits version files to git\n\n"
            "No device connection required!"
        )
        
        info_label = ttk.Label(parent, text=info_text, justify=tk.LEFT, 
                              font=('Segoe UI', 10))
        info_label.grid(row=0, column=0, sticky=(tk.W, tk.E), pady=(0, 20))
        
        # Version preview
        version_frame = ttk.Frame(parent)
        version_frame.grid(row=1, column=0, sticky=(tk.W, tk.E), pady=(0, 10))
        
        ttk.Label(version_frame, text="Next Version:", 
                 font=('Segoe UI', 10, 'bold')).grid(row=0, column=0, sticky=tk.W)
        
        next_ver = self.get_next_version(increment_type='build')
        ttk.Label(version_frame, text=f"v{next_ver}", 
                 font=('Segoe UI', 14, 'bold'), 
                 foreground='#667eea').grid(row=0, column=1, sticky=tk.W, padx=(10, 0))
        
        # Button
        version_btn = ttk.Button(parent, text="📝 Create Version Update", 
                               style='Success.TButton',
                               command=self.start_version_update)
        version_btn.grid(row=2, column=0, pady=10)
    
    def create_backup_tab(self, parent):
        """Create full backup tab content"""
        parent.columnconfigure(0, weight=1)
        parent.rowconfigure(0, weight=1)
        
        # Create canvas for scrolling
        canvas = tk.Canvas(parent, highlightthickness=0)
        scrollbar = ttk.Scrollbar(parent, orient="vertical", command=canvas.yview)
        scrollable_frame = ttk.Frame(canvas)
        
        scrollable_frame.bind(
            "<Configure>",
            lambda e: canvas.configure(scrollregion=canvas.bbox("all"))
        )
        
        canvas.create_window((0, 0), window=scrollable_frame, anchor="nw")
        canvas.configure(yscrollcommand=scrollbar.set)
        
        canvas.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        scrollbar.grid(row=0, column=1, sticky=(tk.N, tk.S))
        
        scrollable_frame.columnconfigure(0, weight=1)
        
        # Info
        info_text = (
            "📦 Create a FULL BACKUP\n\n"
            "This creates a complete backup of your ESP32-S3 device hardware + software.\n\n"
            "What happens:\n"
            "• MAJOR version increments (e.g., 1.3.82 → 2.0.0)\n"
            "• Reads ALL flash memory from connected device\n"
            "• Creates firmware.bin for OTA updates\n"
            "• Creates .zip archive for USB installation\n"
            "• Uploads both .bin and .zip to GitHub\n"
            "• Creates restoration point for known-good hardware state\n\n"
            "Includes:\n"
            "✓ Bootloader and partition table\n"
            "✓ Application firmware (both OTA slots)\n"
            "✓ NVS (settings, WiFi credentials, CAN config)\n"
            "✓ OTA data partition\n"
            "✓ File system (SPIFFS)\n\n"
            "Use this when you have a groundbreaking change or stable milestone!"
        )
        
        info_label = ttk.Label(scrollable_frame, text=info_text, justify=tk.LEFT, 
                              font=('Segoe UI', 9))
        info_label.grid(row=0, column=0, sticky=(tk.W, tk.E), pady=(0, 20), padx=10)
        
        # Version preview
        version_frame = ttk.Frame(scrollable_frame)
        version_frame.grid(row=1, column=0, sticky=(tk.W, tk.E), pady=(0, 10), padx=10)
        
        ttk.Label(version_frame, text="Next Version:", 
                 font=('Segoe UI', 10, 'bold')).grid(row=0, column=0, sticky=tk.W)
        
        next_ver = self.get_next_version(increment_type='major')
        ttk.Label(version_frame, text=f"v{next_ver} (MAJOR)", 
                 font=('Segoe UI', 14, 'bold'), 
                 foreground='#dc3545').grid(row=0, column=1, sticky=tk.W, padx=(10, 0))
        
        # Warning
        warning_frame = ttk.Frame(scrollable_frame, relief=tk.RIDGE, borderwidth=2)
        warning_frame.grid(row=2, column=0, sticky=(tk.W, tk.E), pady=(0, 20), padx=10)
        warning_frame.columnconfigure(0, weight=1)
        
        ttk.Label(warning_frame, text="⚠️ DEVICE REQUIRED", 
                 font=('Segoe UI', 11, 'bold'), 
                 foreground='#ffc107', 
                 padding=10).grid(row=0, column=0)
        
        ttk.Label(warning_frame, 
                 text="Device must be connected via USB.\nThis operation takes 3-5 minutes.",
                 font=('Segoe UI', 9),
                 justify=tk.CENTER,
                 padding=(10, 0, 10, 10)).grid(row=1, column=0)
        
        # Backup button
        backup_btn = ttk.Button(scrollable_frame, text="📦 Create FULL BACKUP", 
                               style='Warning.TButton',
                               command=self.start_full_backup)
        backup_btn.grid(row=3, column=0, pady=20)
        
        # Enable mousewheel scrolling
        def _on_mousewheel(event):
            canvas.yview_scroll(int(-1*(event.delta/120)), "units")
        canvas.bind_all("<MouseWheel>", _on_mousewheel)
    
    def create_ota_tab(self, parent):
        """Create OTA update tab"""
        parent.columnconfigure(0, weight=1)
        parent.rowconfigure(1, weight=1)
        
        # Info
        info_text = (
            "📡 Over-The-Air Updates\n\n"
            "Check for available firmware versions on GitHub and upgrade your device wirelessly.\n"
            "(Coming Soon - Manual OTA selection)"
        )
        
        info_label = ttk.Label(parent, text=info_text, justify=tk.LEFT, 
                              font=('Segoe UI', 10))
        info_label.grid(row=0, column=0, sticky=(tk.W, tk.E), pady=(0, 20))
        
        # Versions list
        list_frame = ttk.LabelFrame(parent, text="Available GitHub Versions", padding="10")
        list_frame.grid(row=1, column=0, sticky=(tk.W, tk.E, tk.N, tk.S), pady=(0, 10))
        list_frame.columnconfigure(0, weight=1)
        list_frame.rowconfigure(0, weight=1)
        
        # Treeview for versions
        columns = ('Type', 'Size')
        self.version_tree = ttk.Treeview(list_frame, columns=columns, show='tree headings', height=10)
        
        self.version_tree.heading('#0', text='Version')
        self.version_tree.heading('Type', text='Type')
        self.version_tree.heading('Size', text='Size (MB)')
        
        self.version_tree.column('#0', width=150)
        self.version_tree.column('Type', width=150, anchor=tk.CENTER)
        self.version_tree.column('Size', width=100, anchor=tk.E)
        
        self.version_tree.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        
        # Scrollbar
        scrollbar = ttk.Scrollbar(list_frame, orient=tk.VERTICAL, command=self.version_tree.yview)
        scrollbar.grid(row=0, column=1, sticky=(tk.N, tk.S))
        self.version_tree.configure(yscrollcommand=scrollbar.set)
        
        # Buttons
        btn_frame = ttk.Frame(parent)
        btn_frame.grid(row=2, column=0, pady=10)
        
        ttk.Button(btn_frame, text="🔄 Check for Updates", 
                  style='Action.TButton',
                  command=self.check_github_versions).grid(row=0, column=0, padx=5)
        
        # OTA button (placeholder for future)
        ttk.Button(btn_frame, text="📡 OTA Update (Coming Soon)", 
                  state='disabled').grid(row=0, column=1, padx=5)
    
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
        columns = ('Version', 'Type', 'Date', 'Size')
        self.backup_tree = ttk.Treeview(list_frame, columns=columns, show='tree headings', height=8)
        
        self.backup_tree.heading('#0', text='Backup Name')
        self.backup_tree.heading('Version', text='Version')
        self.backup_tree.heading('Type', text='Type')
        self.backup_tree.heading('Date', text='Date')
        self.backup_tree.heading('Size', text='Size (MB)')
        
        self.backup_tree.column('#0', width=200)
        self.backup_tree.column('Version', width=80, anchor=tk.CENTER)
        self.backup_tree.column('Type', width=120, anchor=tk.CENTER)
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
    
    def get_next_version(self, increment_type='build'):
        """Calculate next version number"""
        current = self.current_version
        parts = list(map(int, current.split('.')))
        
        if increment_type == 'major':
            parts[0] += 1
            parts[1] = 0
            parts[2] = 0
        else:  # build
            parts[2] += 1
        
        return '.'.join(map(str, parts))
    
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
            backup_type = 'Full Backup' if '_FULL' in name else 'Version Update'
            
            self.backup_tree.insert('', 'end', text=name, 
                                   values=(version, backup_type, date, f"{size_mb:.1f}"),
                                   tags=(str(backup['path']),))
    
    def open_backups_folder(self):
        """Open backups folder in explorer"""
        import subprocess
        subprocess.Popen(f'explorer "{self.manager.backups_dir}"')
    
    def check_github_versions(self):
        """Check for available versions on GitHub"""
        def check_thread():
            try:
                self.disable_buttons()
                
                # Clear existing items
                for item in self.version_tree.get_children():
                    self.version_tree.delete(item)
                
                # Fetch versions
                versions = self.manager.list_github_versions()
                
                # Populate tree
                for v in versions:
                    size_mb = v['size'] / (1024 * 1024)
                    self.version_tree.insert('', 'end', text=f"v{v['version']}", 
                                           values=(v['type'], f"{size_mb:.1f}"),
                                           tags=(v['download_url'],))
                
                if not versions:
                    messagebox.showinfo("No Versions", "No versions found on GitHub.")
                
            except Exception as e:
                messagebox.showerror("Error", f"Failed to check versions:\n{str(e)}")
            finally:
                self.enable_buttons()
        
        threading.Thread(target=check_thread, daemon=True).start()
    
    def start_version_update(self):
        """Start version update (no device needed)"""
        def version_thread():
            try:
                self.disable_buttons()
                version_folder, version = self.manager.version_update()
                
                if version_folder:
                    self.refresh_backups()
                    self.current_version = version
                    messagebox.showinfo("Success", 
                                       f"Version update completed!\nVersion: v{version}\n\n"
                                       f"Files created:\n"
                                       f"• firmware.bin (for OTA updates)\n"
                                       f"• Uploaded to GitHub automatically\n\n"
                                       f"Your device can now update OTA to this version!")
                else:
                    messagebox.showerror("Error", "Version update failed. Check console for details.")
            except Exception as e:
                messagebox.showerror("Error", f"Version update failed:\n{str(e)}")
            finally:
                self.enable_buttons()
        
        threading.Thread(target=version_thread, daemon=True).start()
    
    def start_full_backup(self):
        """Start full backup from device"""
        def backup_thread():
            try:
                self.disable_buttons()
                self.manager.port = self.port_var.get()
                backup_folder, version = self.manager.full_backup()
                
                if backup_folder:
                    self.refresh_backups()
                    self.current_version = version
                    messagebox.showinfo("Success", 
                                       f"FULL BACKUP completed!\nVersion: v{version}\n\n"
                                       f"Files created:\n"
                                       f"• firmware.bin (for OTA updates)\n"
                                       f"• Complete .zip archive (for USB install)\n"
                                       f"• All flash partitions backed up\n\n"
                                       f"Location:\n{backup_folder}\n\n"
                                       f"Uploaded to GitHub automatically.")
                else:
                    messagebox.showerror("Error", "Full backup failed. Check console for details.")
            except Exception as e:
                messagebox.showerror("Error", f"Full backup failed:\n{str(e)}")
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
