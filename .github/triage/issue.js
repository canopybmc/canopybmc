// @ts-check
// SPDX-License-Identifier: GPL-2.0-only
//
// Labels new and edited issues. Issues using an issue form are labelled
// from the form answers, other issues from their title and description.
// This adds target, release, topic and status labels.
//
// It also keeps one summary comment on the issue up to date. The comment
// lists each label with the reason it was added, answers the reporter needs
// to fix, and notes for maintainers.
//
// .github/workflows/issue-triage.yml runs this through actions/github-script,
// which calls the exported function with { github, context, core }.
'use strict';

const rules = require('./rules');
const { listMachines, machineLabel } = require('./machines');
const gh = require('./github');

const MARKER = '<!-- canopybmc-triage -->';

// Quotes user input in the comment as a code span on a single line.
const code = (s) => `\`${s.split('\n')[0].replace(/`/g, "'").slice(0, 80)}\``;

module.exports = async ({ github, context, core }) => {
  const repo = context.repo;
  const issue = context.payload.issue;
  const fields = rules.parseForm(issue.body);
  const isForm = rules.isForm(fields);
  const machines = listMachines(process.env.GITHUB_WORKSPACE);
  const machineNames = machines.map((m) => m.name);

  const wanted = new Map(); // Maps each label to the reason it was added.
  const errors = [];
  const notes = [];
  const add = (label, reason) => {
    if (!wanted.has(label)) wanted.set(label, reason);
  };

  add('needs triage', 'waiting for a maintainer to confirm the labels');

  // Target
  const target = fields[rules.FIELDS.target] || '';
  if (target) {
    for (const entry of target.split(/[,;\n]+/).map((s) => s.trim()).filter(Boolean)) {
      const name = entry.toLowerCase().match(/^[\w.-]+/)?.[0] || entry;
      if (name === 'all') {
        add('target/all', 'target machine `all`');
      } else if (machineNames.includes(name)) {
        add(machineLabel(name), `target machine \`${name}\``);
      } else {
        errors.push(`Unknown target machine ${code(entry)}. Valid machines are ` +
          `${machineNames.map((n) => `\`${n}\``).join(', ')}, or \`all\`.`);
      }
    }
  } else if (!isForm) {
    // Without a form, look for machine names in the title and body.
    const text = `${issue.title}\n${issue.body || ''}`;
    for (const name of machineNames) {
      if (new RegExp(`(?<![\\w-])${name}(?![\\w-])`, 'i').test(text)) {
        add(machineLabel(name), `\`${name}\` is mentioned`);
      }
    }
  }

  // Version and release
  const versionField = fields[rules.FIELDS.version] || '';
  const version = rules.parseVersion(versionField);
  if (versionField) {
    if (!version) {
      errors.push(`Version ${code(versionField)} doesn't look like a CanopyBMC version. ` +
        'Use the firmware version that the web UI or Redfish shows, e.g. `2026.06`, `2026.06-rc1` ' +
        'or `2026.06-94-1519e89`.');
    } else {
      const release = await gh.releaseForVersion(github, repo, context.payload.repository.default_branch, version);
      if (release.label) add(release.label, `version ${code(version.raw)}`);
      if (release.note) notes.push(release.note);
    }
  }

  // canopy-diag Log ID
  const logid = fields[rules.FIELDS.logid] || '';
  if (logid) {
    if (rules.isUuid(logid)) {
      add('logid-provided', 'a canopy-diag Log ID was provided');
    } else {
      errors.push(`canopy-diag Log ID ${code(logid)} is not a UUID of the form ` +
        '`xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx`.');
    }
  }

  // Regression
  const lastWorkingField = fields[rules.FIELDS.lastWorking] || '';
  const lastWorking = rules.parseRevision(lastWorkingField);
  if (rules.isChecked(fields[rules.FIELDS.regression])) {
    add('type/regression', 'marked as a regression');
  } else if (lastWorking) {
    add('type/regression', 'a last working version was given');
  } else if (!isForm && /\bregress(?:ion|ed)?\b/i.test(issue.title)) {
    add('type/regression', 'title mentions a regression');
  }
  if (lastWorkingField && !lastWorking) {
    errors.push(`Last working version ${code(lastWorkingField)} is neither a CanopyBMC version nor a commit hash.`);
  } else if (lastWorking?.hash && !(await gh.commitExists(github, repo, lastWorking.hash))) {
    notes.push(`Last working commit ${code(lastWorking.hash)} is not in this repository.`);
  } else if (lastWorking && version) {
    const good = lastWorking.hash || lastWorking.tag;
    const bad = version.hash || version.tag;
    notes.push(`Changes between the last working and the affected version: ` +
      `https://github.com/${repo.owner}/${repo.repo}/compare/${good}...${bad}`);
  }

  // Components and topics
  const { topics, unmapped } = rules.topicsInComponents(fields[rules.FIELDS.components]);
  topics.forEach((t) => add(t, 'affected components'));
  rules.topicsInTitle(issue.title).forEach((t) => add(t, 'title'));
  // Remove machine names first. Otherwise kommando-ipmi-card would match
  // topic/ipmi.
  let body = issue.body || '';
  machineNames.forEach((n) => { body = body.split(n).join(''); });
  rules.topicsInText(body).forEach((t) => add(t, 'description'));
  if (unmapped.length) {
    notes.push(`No topic matches ${unmapped.map(code).join(', ')}. ` +
      'If one fits, add a mapping to `.github/triage/rules.js`.');
  }

  if (errors.length) add('needs-info', 'some answers need to be fixed, see below');

  // Apply
  const existing = await gh.existingLabels(github, repo);
  const missing = [...wanted.keys()].filter((l) => !existing.has(l));
  for (const label of missing) {
    notes.push(label.startsWith('release/')
      ? `\`${label}\` is not a supported release, so the triage added no release label.`
      : `Label \`${label}\` doesn't exist. Add it in canopybmc/gh-provisioning.`);
    wanted.delete(label);
  }
  const current = new Set(issue.labels.map((l) => l.name));
  await gh.addLabels(github, repo, issue.number, [...wanted.keys()].filter((l) => !current.has(l)));
  core.info(`Labels: ${[...wanted.keys()].join(', ')}`);

  // Summary comment
  const lines = [
    'The triage bot added these labels based on this issue. ' +
      'A maintainer will confirm them during triage.',
    '',
    '| Label | Reason |',
    '| --- | --- |',
    ...[...wanted].map(([label, reason]) => `| \`${label}\` | ${reason} |`),
  ];
  if (wanted.has('topic/security')) {
    lines.push('', '> [!IMPORTANT]', '> If this is a vulnerability, please report it privately as described ' +
      `in the [security policy](https://github.com/${repo.owner}/${repo.repo}/security/policy) instead.`);
  }
  if (errors.length) {
    lines.push('', '### Please check', '', ...errors.map((e) => `- ${e}`), '',
      'Edit the issue to fix this. The bot updates this comment after each edit.');
  }
  if (notes.length) {
    lines.push('', '### Notes for maintainers', '', ...notes.map((n) => `- ${n}`));
  }
  await gh.upsertComment(github, repo, issue.number, MARKER, lines.join('\n'));
};
