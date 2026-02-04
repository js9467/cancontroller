#!/usr/bin/env python3
"""Utility for driving the Behavioral Output REST API from a laptop.

Sends sample behaviors or scenes to the ESP32 so you can validate POWERCELL
responses without touching the on-device preview UI.
"""

from __future__ import annotations

import argparse
import json
import sys
import time
import urllib.error
import urllib.request
from typing import Any, Dict, Tuple

DEFAULT_HOST = "192.168.4.250"


def _http_request(host: str, path: str, method: str = "GET", payload: Dict[str, Any] | None = None) -> Tuple[int, str]:
    url = f"http://{host}{path}"
    data = None
    headers = {"Accept": "application/json"}
    if payload is not None:
        data = json.dumps(payload).encode("utf-8")
        headers["Content-Type"] = "application/json"

    request = urllib.request.Request(url, data=data, headers=headers, method=method)
    try:
        with urllib.request.urlopen(request, timeout=5) as response:
            body = response.read().decode("utf-8", errors="ignore")
            return response.status, body
    except urllib.error.HTTPError as exc:
        body = exc.read().decode("utf-8", errors="ignore")
        return exc.code, body
    except urllib.error.URLError as exc:  # pragma: no cover - interactive tool
        raise SystemExit(f"Connection to {url} failed: {exc}")


def list_outputs(host: str) -> None:
    status, body = _http_request(host, "/api/outputs")
    if status != 200:
        raise SystemExit(f"Server responded with HTTP {status}: {body}")
    outputs = json.loads(body)
    print(f"Found {len(outputs)} outputs:\n")
    for entry in outputs:
        name = entry.get("name", entry["id"])
        print(f"- {name} (id={entry['id']}, cell={entry.get('cellAddress')}, out={entry.get('outputNumber')})")


def set_behavior(host: str, output_id: str, behavior: Dict[str, Any]) -> None:
    status, body = _http_request(host, f"/api/outputs/{output_id}/behavior", method="POST", payload=behavior)
    if status != 200:
        raise SystemExit(f"Failed to set behavior (HTTP {status}): {body}")
    print(f"Behavior applied to {output_id}")


def deactivate_output(host: str, output_id: str) -> None:
    status, body = _http_request(host, f"/api/outputs/{output_id}/deactivate", method="POST")
    if status != 200:
        raise SystemExit(f"Failed to deactivate {output_id} (HTTP {status}): {body}")
    print(f"{output_id} deactivated")


def toggle_scene(host: str, scene_id: str, active: bool) -> None:
    path = f"/api/scenes/{'activate' if active else 'deactivate'}/{scene_id}"
    status, body = _http_request(host, path, method="POST")
    if status != 200:
        raise SystemExit(f"Scene command failed (HTTP {status}): {body}")
    state = "activated" if active else "deactivated"
    print(f"Scene {scene_id} {state}")


def run_demo(host: str, demo: str) -> None:
    if demo == "left-turn":
        print("Running left turn demo (front + rear) ...")
        toggle_scene(host, "left_turn", True)
        time.sleep(8)
        toggle_scene(host, "left_turn", False)
    elif demo == "hazard":
        print("Running hazard flash demo ...")
        toggle_scene(host, "hazard", True)
        time.sleep(10)
        toggle_scene(host, "hazard", False)
    else:
        raise SystemExit(f"Unknown demo '{demo}'")


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="Send behavioral output samples to the ESP32 controller")
    parser.add_argument("--host", default=DEFAULT_HOST, help="Controller IP/hostname (default: %(default)s)")

    subparsers = parser.add_subparsers(dest="command", required=True)

    subparsers.add_parser("list", help="List all outputs exposed by the controller")

    set_parser = subparsers.add_parser("set", help="Apply a behavior to a specific output")
    set_parser.add_argument("output_id", help="Behavioral output ID (e.g., left_turn_front)")
    set_parser.add_argument("behavior_type", choices=[
        "STEADY",
        "FLASH",
        "HOLD_TIMED",
        "RAMP",
    ], help="Behavior type to apply")
    set_parser.add_argument("--value", type=int, default=255, help="Target value (0-255)")
    set_parser.add_argument("--period", type=int, default=1000, help="Period for FLASH (ms)")
    set_parser.add_argument("--duty", type=int, default=50, help="Duty cycle for FLASH (%)")
    set_parser.add_argument("--duration", type=int, default=0, help="Auto-off duration (ms, 0 = infinite)")
    set_parser.add_argument("--fade", type=int, default=500, help="Fade time for RAMP (ms)")
    set_parser.add_argument("--soft-start", action="store_true", help="Enable soft-start bit")

    deactivate_parser = subparsers.add_parser("deactivate", help="Stop a specific output")
    deactivate_parser.add_argument("output_id", help="Behavioral output ID")

    scene_parser = subparsers.add_parser("scene", help="Activate or deactivate a scene")
    scene_parser.add_argument("scene_id", help="Scene identifier (e.g., hazard)")
    scene_parser.add_argument("state", choices=["on", "off"], help="Desired state")

    demo_parser = subparsers.add_parser("demo", help="Run a canned demonstration")
    demo_parser.add_argument("name", choices=["left-turn", "hazard"], help="Demo selection")

    return parser


def main(argv: list[str] | None = None) -> int:
    parser = build_parser()
    args = parser.parse_args(argv)

    if args.command == "list":
        list_outputs(args.host)
    elif args.command == "set":
        behavior = {
            "type": args.behavior_type,
            "targetValue": max(0, min(255, args.value)),
            "period_ms": args.period,
            "dutyCycle": args.duty,
            "duration_ms": args.duration,
            "fadeTime_ms": args.fade,
            "softStart": args.soft_start,
        }
        if args.behavior_type == "FLASH":
            on_time = int(args.period * (args.duty / 100.0))
            off_time = args.period - on_time
            behavior["onTime_ms"] = on_time
            behavior["offTime_ms"] = off_time
        elif args.behavior_type == "RAMP":
            behavior["fadeTime_ms"] = args.fade
        set_behavior(args.host, args.output_id, behavior)
    elif args.command == "deactivate":
        deactivate_output(args.host, args.output_id)
    elif args.command == "scene":
        toggle_scene(args.host, args.scene_id, args.state == "on")
    elif args.command == "demo":
        run_demo(args.host, args.name)
    else:  # pragma: no cover - argparse guards commands
        parser.print_help()
        return 1

    return 0


if __name__ == "__main__":  # pragma: no cover - CLI entry point
    sys.exit(main())
