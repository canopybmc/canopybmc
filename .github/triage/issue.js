// @ts-check
'use strict';

const rules = require('./rules');
const { listMachines, machineLabel } = require('./machines');
const gh = require('./github');

const MARKER = '<!-- canopybmc-triage -->';

// User input quoted in the comment, kept on one line inside a code span.
const code = (s) => `\`${s.split('\n')[0].replace(/`/g, "'").slice(0, 80)}\``;

module.exports = async ({ github, context, core }) => {
  const repo = context.repo;
  const issue = context.payload.issue;
  const fields = rules.parseForm(issue.body);
  const isForm = rules.isForm(fields);
  const machines = listMachines(process.env.GITHUB_WORKSPACE);
  const machineNames = machines.map((m) => m.name);

  const wanted = new Map(); // label -> reason
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
    // Free text issue, look for machine names.
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
        'Expected the firmware version shown in the web UI or Redfish, e.g. `2026.06`, `2026.06-rc1` ' +
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
      errors.push(`canopy-diag Log ID ${code(logid)} is not a UUID ` +
        '(`xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx`).');
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
  // Machine names like kommando-ipmi-card would match topics otherwise.
  let body = issue.body || '';
  machineNames.forEach((n) => { body = body.split(n).join(''); });
  rules.topicsInText(body).forEach((t) => add(t, 'description'));
  if (unmapped.length) {
    notes.push(`No topic is known for ${unmapped.map(code).join(', ')}. ` +
      'Add a mapping to `.github/triage/rules.js` if one fits.');
  }

  if (errors.length) add('needs-info', 'some answers need to be fixed, see below');

  // Apply
  const existing = await gh.existingLabels(github, repo);
  const missing = [...wanted.keys()].filter((l) => !existing.has(l));
  for (const label of missing) {
    notes.push(label.startsWith('release/')
      ? `\`${label}\` is not a supported release, no release label was added.`
      : `Label \`${label}\` doesn't exist. Add it in canopybmc/gh-provisioning.`);
    wanted.delete(label);
  }
  const current = new Set(issue.labels.map((l) => l.name));
  await gh.addLabels(github, repo, issue.number, [...wanted.keys()].filter((l) => !current.has(l)));
  core.info(`Labels: ${[...wanted.keys()].join(', ')}`);

  // Summary comment
  const lines = [
    'Thanks! Based on this issue, the following labels were added. ' +
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
      'Edit the issue to fix this, this comment gets updated.');
  }
  if (notes.length) {
    lines.push('', '### Notes for maintainers', '', ...notes.map((n) => `- ${n}`));
  }
  await gh.upsertComment(github, repo, issue.number, MARKER, lines.join('\n'));
};
