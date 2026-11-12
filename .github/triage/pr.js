// @ts-check
'use strict';

const rules = require('./rules');
const { listMachines, machineLabel, vendorLabel, firmwareciMachine } = require('./machines');
const gh = require('./github');

// Checkboxes of .github/pull_request_template.md
const CHECKBOXES = {
  'Bug fix': ['type/bug'],
  'Regression fix': ['type/bug', 'type/regression'],
  'New feature': ['type/feature'],
  'Hardware enablement': ['type/hardware-enablement'],
  'CI or tooling': ['topic/ci'],
  'Documentation': ['topic/docs'],
  'Needs a backport': ['backport/needed'],
};

const BUILD_FILES = ['openbmc', 'setup', 'Taskfile.yml', '.gitmodules', '.templateconf', 'oe-init-build-env'];

function targetsForFile(machines, file) {
  const fwci = file.match(/^\.firmwareci\/(?:workflows|duts)\/([^/]+)\//);
  if (fwci) {
    const m = firmwareciMachine(machines, fwci[1]);
    return m ? [machineLabel(m.name)] : [];
  }
  if (file === 'openbmc') return ['target/all'];
  if (!file.startsWith('meta-canopy/')) return [];

  const conf = machines.find((m) => m.conf === file);
  if (conf) return [machineLabel(conf.name)];

  // A machine with a layer of its own below the vendor layer.
  const own = machines.find((m) => m.layer && m.layer !== m.vendorRoot && m.layer !== 'meta-canopy' &&
    file.startsWith(`${m.layer}/`));
  if (own) return [machineLabel(own.name)];

  const vendor = file.match(/^meta-canopy\/meta-([^/]+)\//);
  return [vendor ? vendorLabel(vendor[1]) : 'target/all'];
}

function topicsForFile(file) {
  const topics = rules.topicsInPath(file);
  if (file.startsWith('.github/')) topics.push('topic/ci');
  if (/^\.github\/(?:workflows|actions)\//.test(file)) topics.push('github_actions');
  if (file.startsWith('.firmwareci/')) topics.push('topic/ci');
  if (/^(?:doc\/|release-notes\/)|(?:^|\/)README\.md$/.test(file)) topics.push('topic/docs');
  if (BUILD_FILES.includes(file) || /^meta-[^/]+\/(?:.*\/)?conf\//.test(file)) topics.push('topic/build');
  return topics;
}

module.exports = async ({ github, context, core }) => {
  const repo = context.repo;
  const pr = context.payload.pull_request;
  const machines = listMachines(process.env.GITHUB_WORKSPACE);
  const wanted = new Set();

  const files = await github.paginate(github.rest.pulls.listFiles, {
    ...repo,
    pull_number: pr.number,
    per_page: 100,
  });
  for (const { filename } of files) {
    targetsForFile(machines, filename).forEach((l) => wanted.add(l));
    topicsForFile(filename).forEach((l) => wanted.add(l));
  }

  // Release
  const base = pr.base.ref;
  if (base === context.payload.repository.default_branch) {
    wanted.add('release/rolling');
  } else if (gh.RELEASE_BRANCH_RE.test(base)) {
    wanted.add(`release/${base}`);
  }

  // Title, e.g. "meta-hpe: u-boot: update to 2026.10"
  // Machine names like kommando-ipmi-card would match topics otherwise.
  let title = pr.title;
  machines.forEach((m) => { title = title.split(m.name).join(''); });
  rules.topicsInTitle(title).forEach((l) => wanted.add(l));

  // Pull request template
  for (const [text, labels] of Object.entries(CHECKBOXES)) {
    if (rules.isChecked(pr.body, text)) labels.forEach((l) => wanted.add(l));
  }

  const existing = await gh.existingLabels(github, repo);
  const missing = [...wanted].filter((l) => !existing.has(l));
  if (missing.length) core.warning(`Labels not in the repository, skipped: ${missing.join(', ')}`);

  const current = new Set(pr.labels.map((l) => l.name));
  const add = [...wanted].filter((l) => existing.has(l) && !current.has(l));
  await gh.addLabels(github, repo, pr.number, add);
  core.info(`Added labels: ${add.join(', ') || 'none'}`);
};
