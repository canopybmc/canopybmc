// @ts-check
'use strict';

// Headings of the issue form fields in .github/ISSUE_TEMPLATE, as GitHub
// renders them into the issue body.
const FIELDS = {
  target: 'Target machine',
  version: 'Version',
  logid: 'canopy-diag Log ID',
  components: 'Affected components',
  regression: 'Regression',
  lastWorking: 'Last working version',
};

// Components and the topic label they belong to.
//
// `names` are identifiers specific enough to be matched anywhere: issue
// title and body, PR titles and the recipe part of changed file paths.
// `words` are generic and only matched against the "Affected components"
// field and titles, where the user named them on purpose.
//
// Patterns are regular expression sources, matched case-insensitively as
// whole words. Add new components here.
const COMPONENTS = [
  {
    label: 'topic/redfish',
    names: ['bmcweb', 'redfish'],
    words: [],
  },
  {
    label: 'topic/ipmi',
    names: ['ipmi', 'ipmid', 'phosphor-host-ipmid', 'phosphor-ipmi-\\w+', 'ipmitool', 'dcmi', 'sel-logger'],
    words: ['kcs', 'sel'],
  },
  {
    label: 'topic/sensors',
    names: [
      'dbus-sensors', 'entity-manager', 'phosphor-virtual-sensor',
      'phosphor-pid-control', 'swampd', 'phosphor-fan', 'phosphor-hwmon',
    ],
    words: ['sensors?', 'fans?', 'thermal', 'hwmon', 'temperature'],
  },
  {
    label: 'topic/power',
    names: ['x86-power-control', 'phosphor-state-manager', 'phosphor-post-code-manager', 'phosphor-power'],
    words: ['power', 'post-?codes?', 'boot ?flow', 'host state', 'chassis'],
  },
  {
    label: 'topic/networking',
    names: ['phosphor-networkd', 'ncsi', 'ncsid'],
    words: ['network(?:ing)?', 'dhcp', 'ipv[46]', 'vlan', 'ethernet'],
  },
  {
    label: 'topic/webui',
    names: ['webui-vue', 'webui'],
    words: ['web ?ui', 'web interface'],
  },
  {
    label: 'topic/u-boot',
    names: ['u-boot[\\w-]*', 'libubootenv'],
    words: ['uboot', 'bootloader'],
  },
  {
    label: 'topic/linux',
    names: ['linux-[\\w.-]+', 'recipes-kernel', 'device ?tree'],
    words: ['linux', 'kernel', 'dts', 'drivers?'],
  },
  {
    label: 'topic/build',
    names: ['bitbake'],
    words: ['build', 'yocto', 'openembedded', 'recipes?', 'layers?', 'bbappend'],
  },
  {
    label: 'topic/ci',
    names: ['firmwareci'],
    words: ['ci', 'github actions?'],
  },
  {
    label: 'topic/docs',
    names: [],
    words: ['docs?', 'documentation', 'readme'],
  },
  {
    label: 'topic/security',
    names: ['cve-\\d{4}-\\d+'],
    words: ['security', 'vulnerabilit(?:y|ies)', 'ssh', 'dropbear', 'tls', 'certificates?'],
  },
];

const UUID_RE = /^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$/i;

// Version schema of meta-canopy/recipes-core/os-release/os-release.bbappend:
//   <tag>[-<custom>]                     exact tag, e.g. 2026.06, 2026.06-rc1, 2026.03.1
//   <tag>-<commits>-<hash>[-<custom>]    untagged commit
const VERSION_RE =
  /^(\d{4}\.(?:0[1-9]|1[0-2]))((?:\.\d+)?(?:-(?:rc|beta)\d*)?)(?:-(\d+)-g?([0-9a-f]{7,40}))?(?:-([\w.+-]+))?$/i;

function wordRegex(sources) {
  return new RegExp(`(?<![\\w-])(?:${sources.join('|')})(?![\\w-])`, 'i');
}

const COMPILED = COMPONENTS.map((c) => ({
  label: c.label,
  names: c.names.length ? wordRegex(c.names) : null,
  any: wordRegex([...c.names, ...c.words]),
}));

// Split a rendered issue form body into { heading: value }.
function parseForm(body) {
  const fields = {};
  const parts = (body || '').replace(/\r\n/g, '\n').split(/^### (.+)$/m);
  for (let i = 1; i < parts.length; i += 2) {
    const value = parts[i + 1].trim();
    fields[parts[i].trim()] = value === '_No response_' ? '' : value;
  }
  return fields;
}

function isForm(fields) {
  return Object.values(FIELDS).some((heading) => heading in fields);
}

function isChecked(value, text) {
  return (value || '')
    .split('\n')
    .some((line) => /^- \[x\]/i.test(line.trim()) && (!text || line.includes(text)));
}

// Accepts a bare version, "VERSION_ID=..." or a pasted /etc/os-release.
function parseVersion(input) {
  let value = (input || '').trim();
  const line = value.match(/^VERSION_ID=(.*)$/m);
  if (line) value = line[1];
  value = value.trim().replace(/^["']|["']$/g, '');

  const m = value.match(VERSION_RE);
  if (!m) return null;
  return {
    raw: value,
    base: m[1],
    tag: m[1] + m[2],
    commits: m[3] ? Number(m[3]) : 0,
    hash: m[4] ? m[4].toLowerCase() : null,
  };
}

// A version as accepted by parseVersion(), or a bare commit hash.
function parseRevision(input) {
  const version = parseVersion(input);
  if (version) return version;
  const hash = (input || '').trim().match(/^g?([0-9a-f]{7,40})$/i);
  return hash ? { raw: hash[1], hash: hash[1].toLowerCase() } : null;
}

function isUuid(value) {
  return UUID_RE.test((value || '').trim());
}

// Topic labels whose specific component names appear in free text.
function topicsInText(text) {
  return COMPILED.filter((c) => c.names && c.names.test(text || '')).map((c) => c.label);
}

// Topic labels for text where generic words count too (titles, the
// components field).
function topicsInTitle(text) {
  return COMPILED.filter((c) => c.any.test(text || '')).map((c) => c.label);
}

// Maps the comma separated "Affected components" field. Returns the topics
// and the entries that matched nothing, so triagers see what's missing.
function topicsInComponents(value) {
  const topics = new Set();
  const unmapped = [];
  for (const entry of (value || '').split(/[,;\n]+/).map((s) => s.trim()).filter(Boolean)) {
    const hits = topicsInTitle(entry);
    if (hits.length === 0) unmapped.push(entry);
    hits.forEach((t) => topics.add(t));
  }
  return { topics: [...topics], unmapped };
}

// Only the part of a path starting at the recipe directory is meaningful,
// layer names like meta-kommando-ipmi-card would match otherwise.
function topicsInPath(path) {
  const idx = path.search(/(?:^|\/)recipes-/);
  return idx === -1 ? [] : topicsInText(path.slice(idx).replace(/\//g, ' '));
}

module.exports = {
  FIELDS,
  COMPONENTS,
  parseForm,
  isForm,
  isChecked,
  parseVersion,
  parseRevision,
  isUuid,
  topicsInText,
  topicsInTitle,
  topicsInComponents,
  topicsInPath,
};
