// @ts-check
'use strict';

const { execFileSync } = require('node:child_process');
const fs = require('node:fs');
const path = require('node:path');

// Machines whose target label doesn't follow `target/<machine>`.
const LABEL_ALIASES = {
  'canopy-qemu': 'target/qemu',
};

// FirmwareCI workflow and DUT names that don't start with the machine name.
const FIRMWARECI_ALIASES = {
  'hpe-dl320': 'hpe-proliant-g11',
  'asus-kommando-card': 'kommando-ipmi-card',
};

function findMachineConfs(dir, out = []) {
  for (const entry of fs.readdirSync(dir, { withFileTypes: true })) {
    const full = path.join(dir, entry.name);
    if (entry.isDirectory()) {
      findMachineConfs(full, out);
    } else if (entry.name.endsWith('.conf') && path.basename(path.dirname(full)) === 'machine' &&
               path.basename(path.dirname(path.dirname(full))) === 'conf') {
      out.push(full);
    }
  }
  return out;
}

// Returns [{ name, layer, vendor, vendorRoot }] with paths relative to root.
function listMachines(root) {
  // setup initialises the openbmc submodule when this is missing, listing
  // the machines doesn't need it.
  const stub = path.join(root, 'openbmc', 'oe-init-build-env');
  if (!fs.existsSync(stub)) {
    fs.mkdirSync(path.dirname(stub), { recursive: true });
    fs.writeFileSync(stub, '');
  }

  // `setup machines` drops everything with "qemu" in its name, including
  // canopy-qemu, so call machine() directly and only drop the upstream
  // openembedded-core qemu machines.
  const out = execFileSync('bash', ['-c', 'source ./setup machines >/dev/null && machine || true'], {
    cwd: root,
    encoding: 'utf8',
    stdio: ['ignore', 'pipe', 'ignore'],
  });
  const names = out
    .split('\n')
    .map((l) => l.trim())
    .filter((l) => l && !l.startsWith('openbmc/'))
    .map((l) => l.slice(l.lastIndexOf('/') + 1));

  const confs = fs
    .readdirSync(root)
    .filter((d) => d.startsWith('meta-') && fs.statSync(path.join(root, d)).isDirectory())
    .flatMap((d) => findMachineConfs(path.join(root, d)))
    .map((f) => path.relative(root, f));

  return [...new Set(names)].sort().map((name) => {
    const conf = confs.find((c) => path.basename(c) === `${name}.conf`) || null;
    const layer = conf ? path.dirname(path.dirname(path.dirname(conf))) : null;
    const vendor = layer && layer.match(/^meta-canopy\/meta-([^/]+)/);
    return {
      name,
      conf,
      layer,
      vendor: vendor ? vendor[1] : null,
      vendorRoot: vendor ? vendor[0] : null,
    };
  });
}

function machineLabel(name) {
  return LABEL_ALIASES[name] || `target/${name}`;
}

function vendorLabel(vendor) {
  return `target/${vendor}/all`;
}

// Maps a FirmwareCI workflow or DUT directory name to a machine.
function firmwareciMachine(machines, dirName) {
  let name = dirName.replace(/^dut-/, '');
  for (const [prefix, machine] of Object.entries(FIRMWARECI_ALIASES)) {
    if (name.startsWith(prefix)) name = machine + name.slice(prefix.length);
  }
  return machines.find((m) => name === m.name || name.startsWith(`${m.name}-`)) || null;
}

module.exports = { listMachines, machineLabel, vendorLabel, firmwareciMachine };
