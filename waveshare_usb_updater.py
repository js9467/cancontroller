#!/usr/bin/env python3
"""
Bronco Controls - Waveshare USB Firmware Updater

Small Windows GUI app that:
- Scans USB serial ports to find the ESP32-S3 Waveshare display
- Downloads the latest Bronco firmware (.bin) from GitHub (with local fallback)
- Installs it via esptool (same layout as PlatformIO upload)

Intended to be bundled as a standalone .exe for end users.
"""

import sys
import threading
import subprocess
import tempfile
import re
from pathlib import Path

import tkinter as tk
from tkinter import ttk, messagebox, scrolledtext

try:
    import requests
except ImportError as e:  # pragma: no cover
    raise SystemExit("The 'requests' package is required. Install with: pip install requests") from e

try:
    from serial.tools import list_ports
except ImportError as e:  # pragma: no cover
    raise SystemExit("The 'pyserial' package is required. Install with: pip install pyserial") from e

try:
    import esptool  # type: ignore
except ImportError as e:  # pragma: no cover
    raise SystemExit("The 'esptool' package is required. Install with: pip install esptool") from e


GITHUB_REPO = "js9467/cancontroller"
GITHUB_BRANCH = "master"
GITHUB_VERSIONS_PATH = "versions"

# OPTIONAL: GitHub personal access token for private repos.
#
# WARNING: If you embed a real token here and build an EXE, anyone with the
# EXE can potentially extract that token and gain the same GitHub access.
# Use a token with minimal permissions (read-only on this repo) and accept
# the risk before distributing.
#
# Example (do NOT commit real tokens to public repos):
#   GITHUB_TOKEN = "ghp_XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"
GITHUB_TOKEN = None


def parse_version_from_name(name: str):
    """Extract semantic version tuple from bronco_vX.Y.Z.bin name.

    Returns (major, minor, build) or None if not matching.
    """
    m = re.match(r"bronco_v(\d+)\.(\d+)\.(\d+)\.bin$", name)
    if not m:
        return None
    return tuple(int(part) for part in m.groups())


def find_waveshare_ports():
    """Scan serial ports and return a list of likely Waveshare/ESP32-S3 devices.

    Heuristics only; if multiple matches, user will choose.
    """
    candidates = []
    for port in list_ports.comports():
        desc = (port.description or "").lower()
        hwid = (port.hwid or "").lower()

        # Common USB-serial chips and ESP32 identifiers
        keywords = [
            "waveshare",
            "esp32",
            "usb-serial",
            "usb serial",
            "ch340",
            "cp210",
            "ftdi",
        ]
        vid_keywords = ["vid_1a86", "vid_10c4", "vid_0403"]  # CH340, CP210x, FTDI

        score = 0
        if any(k in desc for k in keywords):
            score += 2
        if any(k in hwid for k in vid_keywords):
            score += 1

        if score > 0:
            candidates.append((score, port))

    # Sort by score (best first)
    candidates.sort(key=lambda x: x[0], reverse=True)
    return [p for _, p in candidates]


def get_latest_firmware():
    """Determine latest bronco_vX.Y.Z.bin from GitHub, with local fallback.

    Returns (version_str, local_path_or_tempfile_path).
    Raises RuntimeError on failure with a human-friendly reason.
    """
    versions = []
    github_status = None
    github_error = None

    # Build headers for GitHub (optionally with PAT for private repo)
    headers = {"Accept": "application/vnd.github.v3+json"}
    if GITHUB_TOKEN:
        headers["Authorization"] = f"token {GITHUB_TOKEN}"

    # 1) Try GitHub contents API
    try:
        api_url = f"https://api.github.com/repos/{GITHUB_REPO}/contents/{GITHUB_VERSIONS_PATH}?ref={GITHUB_BRANCH}"
        resp = requests.get(api_url, headers=headers, timeout=15)
        github_status = resp.status_code
        if resp.status_code == 200:
            for item in resp.json():
                name = item.get("name", "")
                ver_tuple = parse_version_from_name(name)
                if ver_tuple:
                    versions.append({
                        "version_tuple": ver_tuple,
                        "version_str": ".".join(str(p) for p in ver_tuple),
                        "name": name,
                        "download_url": item.get("download_url"),
                        "source": "github",
                    })
        else:
            github_error = f"GitHub API returned HTTP {resp.status_code} for {api_url}"
    except Exception as e:
        github_error = f"GitHub request failed: {e}"

    # 2) Local fallback: ./versions folder next to this script (or EXE)
    script_dir = Path(__file__).parent.resolve()
    candidate_dirs = [
        script_dir / "versions",
        script_dir.parent / "versions",  # helpful when running from dist/ next to project root
    ]

    local_found = False
    for local_versions_dir in candidate_dirs:
        if local_versions_dir.exists():
            local_found = True
            for entry in local_versions_dir.glob("bronco_v*.bin"):
                ver_tuple = parse_version_from_name(entry.name)
                if ver_tuple:
                    versions.append({
                        "version_tuple": ver_tuple,
                        "version_str": ".".join(str(p) for p in ver_tuple),
                        "name": entry.name,
                        "local_path": entry,
                        "source": "local",
                    })

    if not versions:
        parts = []
        if github_status is not None:
            if github_status == 404:
                parts.append("GitHub: repo or versions/ path not accessible (HTTP 404)")
            elif github_status == 401:
                parts.append("GitHub: unauthorized (private repo or bad credentials)")
            else:
                parts.append(f"GitHub: HTTP {github_status}")
        if github_error:
            parts.append(github_error)
        if not local_found:
            parts.append("Local: no versions/ folder with bronco_vX.Y.Z.bin found next to the app")

        detail = "; ".join(parts) if parts else "No firmware sources available"
        raise RuntimeError(detail)

    # Pick highest semantic version
    versions.sort(key=lambda v: v["version_tuple"], reverse=True)
    latest = versions[0]

    # If already local, just return
    if latest.get("source") == "local" and latest.get("local_path") is not None:
        return latest["version_str"], latest["local_path"]

    # Otherwise download from GitHub to a temp file
    download_url = latest.get("download_url")
    if not download_url:
        raise RuntimeError("Latest firmware entry has no download_url from GitHub")

    tmp_dir = Path(tempfile.gettempdir())
    tmp_path = tmp_dir / latest["name"]

    # Use raw download URL with same auth headers if needed
    with requests.get(download_url, headers=headers, stream=True, timeout=60) as r:
        if r.status_code != 200:
            raise RuntimeError(f"Download failed with HTTP {r.status_code}")
        with open(tmp_path, "wb") as f:
            for chunk in r.iter_content(chunk_size=8192):
                if chunk:
                    f.write(chunk)

    return latest["version_str"], tmp_path


class USBUpdaterGUI:
    def __init__(self, root: tk.Tk):
        self.root = root
        self.root.title("Bronco Controls - Waveshare USB Updater")
        self.root.geometry("700x500")
        self.root.resizable(True, True)

        self.port_var = tk.StringVar()
        self.version_var = tk.StringVar(value="Unknown")

        self._build_ui()
        self._auto_detect_port()
        self._load_latest_version_async()

    # UI construction
    def _build_ui(self):
        main = ttk.Frame(self.root, padding=10)
        main.grid(row=0, column=0, sticky="nsew")
        self.root.rowconfigure(0, weight=1)
        self.root.columnconfigure(0, weight=1)

        main.columnconfigure(0, weight=1)
        main.rowconfigure(3, weight=1)

        header = ttk.Label(main, text="Bronco Controls - Waveshare USB Firmware Updater",
                           font=("Segoe UI", 14, "bold"))
        header.grid(row=0, column=0, sticky="w", pady=(0, 10))

        # Device section
        device_frame = ttk.LabelFrame(main, text="Device", padding=10)
        device_frame.grid(row=1, column=0, sticky="ew", pady=(0, 10))
        device_frame.columnconfigure(1, weight=1)

        ttk.Label(device_frame, text="Port:").grid(row=0, column=0, sticky="w")
        self.port_combo = ttk.Combobox(device_frame, textvariable=self.port_var, width=15, state="readonly")
        self.port_combo.grid(row=0, column=1, sticky="w", padx=(5, 10))

        scan_btn = ttk.Button(device_frame, text="Scan Ports", command=self._auto_detect_port)
        scan_btn.grid(row=0, column=2, sticky="w")

        ttk.Label(device_frame, text="Latest Firmware:").grid(row=1, column=0, sticky="w", pady=(8, 0))
        self.version_label = ttk.Label(device_frame, textvariable=self.version_var, font=("Segoe UI", 10, "bold"))
        self.version_label.grid(row=1, column=1, sticky="w", pady=(8, 0))

        # Action button
        action_frame = ttk.Frame(main)
        action_frame.grid(row=2, column=0, pady=(0, 10))

        self.update_btn = ttk.Button(action_frame, text="Download && Install Latest Firmware",
                                     command=self._start_update_thread)
        self.update_btn.grid(row=0, column=0, padx=5)

        # Console
        console_frame = ttk.LabelFrame(main, text="Log", padding=5)
        console_frame.grid(row=3, column=0, sticky="nsew")
        console_frame.rowconfigure(0, weight=1)
        console_frame.columnconfigure(0, weight=1)

        self.console = scrolledtext.ScrolledText(console_frame, height=15, font=("Consolas", 9),
                                                 bg="#1e1e1e", fg="#d4d4d4", insertbackground="white")
        self.console.grid(row=0, column=0, sticky="nsew")

    # Logging helper
    def log(self, text: str):
        self.console.insert(tk.END, text + "\n")
        self.console.see(tk.END)
        self.console.update_idletasks()

    # Port handling
    def _auto_detect_port(self):
        ports = list_ports.comports()
        waveshare_ports = find_waveshare_ports()

        all_devices = [p.device for p in ports]
        self.port_combo["values"] = all_devices

        if waveshare_ports:
            # Use best candidate
            self.port_var.set(waveshare_ports[0].device)
            self.log(f"Auto-detected Waveshare / ESP32-S3 device on {waveshare_ports[0].device}")
        elif all_devices:
            # Fallback: first available port
            self.port_var.set(all_devices[0])
            self.log(f"No specific Waveshare match found; defaulting to {all_devices[0]}")
        else:
            self.port_var.set("")
            self.log("No serial ports detected. Connect the Waveshare device and click 'Scan Ports'.")

    def _set_busy(self, busy: bool):
        self.root.config(cursor="wait" if busy else "")
        state = tk.DISABLED if busy else tk.NORMAL
        self.update_btn.config(state=state)

    # Version loading
    def _load_latest_version_async(self):
        def worker():
            try:
                self.log("Checking for latest firmware version from GitHub...")
                version, _ = get_latest_firmware()
                self.root.after(0, lambda: self.version_var.set(f"v{version}"))
                self.root.after(0, lambda: self.log(f"Latest firmware: v{version}"))
            except Exception as e:
                self.root.after(0, lambda: self.log(f"Failed to determine latest firmware: {e}"))

        threading.Thread(target=worker, daemon=True).start()

    # Update flow
    def _start_update_thread(self):
        port = self.port_var.get().strip()
        if not port:
            messagebox.showwarning("No Port", "No serial port selected. Connect the device and click 'Scan Ports'.")
            return

        if not messagebox.askyesno(
            "Confirm Update",
            "This will download the latest Bronco firmware and flash it to the device via USB.\n\n"
            "Do not disconnect power or USB during the update.\n\nContinue?",
        ):
            return

        thread = threading.Thread(target=self._run_update, args=(port,), daemon=True)
        thread.start()

    def _run_update(self, port: str):
        self._set_busy(True)
        try:
            self.log("")
            self.log("================ USB Update Started ================")
            self.log(f"Port: {port}")

            # 1) Get latest firmware
            try:
                version, firmware_path = get_latest_firmware()
            except Exception as e:
                self.log(f"ERROR: Failed to get latest firmware: {e}")
                messagebox.showerror("Firmware Error", f"Failed to get latest firmware:\n{e}")
                return

            self.root.after(0, lambda: self.version_var.set(f"v{version}"))
            self.log(f"Using firmware v{version} ({firmware_path})")

            # 2) Flash via esptool (library call, no new process)
            self.log("")
            self.log("Flashing firmware with esptool...")
            self.log("If upload fails, you may need to:\n"
                     "  1) Hold BOOT button on the Waveshare board\n"
                     "  2) Press RESET button\n"
                     "  3) Release BOOT, then retry")

            args = [
                "esptool",
                "--chip",
                "esp32s3",
                "--port",
                port,
                "--baud",
                "115200",
                "write_flash",
                "0x0",
                str(firmware_path),
            ]

            self.log("Running (embedded esptool): " + " ".join(args))

            exit_code = 0
            try:
                # esptool.main() calls sys.exit(), so catch SystemExit
                esptool.main(args)
            except SystemExit as ex:  # pragma: no cover - behavior from esptool
                try:
                    exit_code = int(ex.code)
                except (TypeError, ValueError):
                    exit_code = 1

            if exit_code != 0:
                self.log("")
                self.log(f"Upload FAILED with exit code {exit_code}")
                messagebox.showerror("Upload Failed", "Firmware upload failed. Check USB connection and try again.")
                return

            self.log("")
            self.log("================  Update Complete  =================")
            self.log("Device should reboot automatically with the new firmware.")
            messagebox.showinfo("Success", f"Firmware v{version} installed successfully!\n\n"
                                           "The device should reboot into the new version.")
        finally:
            self._set_busy(False)


def main():  # pragma: no cover
    root = tk.Tk()
    app = USBUpdaterGUI(root)
    root.mainloop()


if __name__ == "__main__":  # pragma: no cover
    main()
