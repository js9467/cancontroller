#!/usr/bin/env node
import crypto from "node:crypto";
import fs from "node:fs";
import path from "node:path";

function usage() {
  console.error("Usage: node build_manifest.mjs --version <x.y.z> --firmware <path> [--channel stable] [--base-url https://example.com] [--outfile manifest.json] [--notes 'text']");
  process.exit(1);
}

const args = process.argv.slice(2);
const options = new Map();
for (let i = 0; i < args.length; i += 2) {
  const key = args[i];
  const value = args[i + 1];
  if (!key || key.startsWith("-")) {
    if (value === undefined || value.startsWith("-")) {
      usage();
    }
  }
  options.set(key, value);
}

const firmwarePath = options.get("--firmware");
const version = options.get("--version");
const channel = options.get("--channel") ?? "stable";
const baseUrl = options.get("--base-url") ?? "";
const notes = options.get("--notes") ?? "";
let outfile = options.get("--outfile");

if (!firmwarePath || !version) {
  usage();
}

const resolvedFirmware = path.resolve(firmwarePath);
if (!fs.existsSync(resolvedFirmware)) {
  console.error(`Firmware not found: ${resolvedFirmware}`);
  process.exit(2);
}

const firmwareBuffer = fs.readFileSync(resolvedFirmware);
const md5 = crypto.createHash("md5").update(firmwareBuffer).digest("hex");
const size = firmwareBuffer.length;

const firmwareName = path.basename(resolvedFirmware);
let firmwareUrl = firmwareName;
if (baseUrl) {
  firmwareUrl = `${baseUrl.replace(/\/$/, "")}/${firmwareName}`;
}

const manifest = {
  version,
  channel,
  released_at: new Date().toISOString(),
  notes,
  firmware: {
    url: firmwareUrl,
    size,
    md5
  }
};

if (!outfile) {
  outfile = path.join(path.dirname(resolvedFirmware), "manifest.json");
}

fs.writeFileSync(outfile, JSON.stringify(manifest, null, 2));
console.log(`Manifest written to ${outfile}`);
