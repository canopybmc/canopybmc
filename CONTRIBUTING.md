# Contributing

## Reporting issues

Security vulnerabilities are reported privately, see [SECURITY.md](SECURITY.md).

Issues are filed through the issue forms. A few fields are worth looking up
before opening one:

| Field              | Where to find it                                                      |
| ------------------ | --------------------------------------------------------------------- |
| Target machine     | The name passed to `source setup <machine>`, e.g. `hpe-proliant-g11`  |
| Version            | Web UI, Redfish (see below) or `VERSION_ID` in `/etc/os-release`      |
| canopy-diag Log ID | The UUID canopy-diag shows after uploading an FFDC                    |

In Redfish, the version is the `Version` of the software image that
`/redfish/v1/Managers/bmc` links as `Links.ActiveSoftwareImage`:

```sh
bmc='[2001:db8::2]'
password=0penBmc
image=$(curl -sku root:${password} https://${bmc}/redfish/v1/Managers/bmc \
  | jq -r '.Links.ActiveSoftwareImage."@odata.id"')
curl -sku root:${password} "https://${bmc}${image}" | jq -r .Version
```

Builds from a release tag show the tag, e.g. `2026.06`. Builds from any
other commit show the closest tag, the number of commits since and the
commit, e.g. `2026.06-94-1519e89`.

After an issue is opened, a bot adds labels based on the answers and posts a
comment listing them. If an answer can't be used, e.g. an unknown machine or
a malformed version, the comment says what to fix and the issue gets
`needs-info`. Editing the issue updates the comment.

## Labels

Labels are managed in
[gh-provisioning](https://github.com/canopybmc/gh-provisioning).

**Labels that don't exist there are never added by the automation!**

| What                 | Labels                                         | Source                                         |
| -------------------- | ---------------------------------------------- | ---------------------------------------------- |
| New issues           | `needs triage` and the issue type              | Issue form                                     |
| Target               | `target/<machine>`, `target/<vendor>/all`      | Machines known to `setup`, changed layers      |
| Release              | `release/rolling`, `release/<version>`         | Issue version, pull request base branch        |
| Topic                | `topic/*`                                      | Components, titles, changed recipes            |
| Type, backport       | `type/*`, `backport/needed`                    | Pull request template checkboxes               |
| canopy-diag          | `logid-provided`                               | A valid Log ID in the issue                    |

New machines are picked up from `setup` automatically, only their
`target/<machine>` label needs to be added in gh-provisioning. Components and
the topic they belong to are listed in `.github/triage/rules.js`.

The automation adds the labels based on pattern matching, but a maintainer has
to confirm them during triage. The maintainer will remove what doesn't fit, and
removes `needs triage` when done. From then on, edits to the issue no longer
change its labels.

## Developer Certificate of Origin

Every commit must be signed off by its author, certifying the
[Developer Certificate of Origin](https://developercertificate.org):

```
Signed-off-by: Jane Doe <jane.doe@example.com>
```

`git commit -s` adds it. Name and email have to match the commit author. The
`DCO` check fails on pull requests with commits that lack it. To sign off
all commits of a branch afterwards:

```sh
git rebase --signoff origin/main
git push --force-with-lease
```

## AI-generated code

Any AI-generated commit **MUST NOT** add a `Signed-off-by` trailer! This trailer
must always be written by a human.
The contributor is responsible for confirming the AI tool's terms don't conflict
with the open-source project's licence/IP policy.

Any AI-generated commit **MUST NOT** add a `Co-authored-by` trailer! Use
`Assisted-by` instead!
