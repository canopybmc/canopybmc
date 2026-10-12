# Security Policy

If you have found a potential security vulnerability, disclosing it publicly
without review could put systems and people at risk through compromised
hardware or software. This process allows fixes to be discussed privately for
a time before public disclosure.

Issues that are [discovered with AI assistance][linux-security-bugs], do not
present a security issue on a real system, or cannot be reproduced, but
otherwise might be an improvement to the code, **MUST NOT** be reported
through the processes below. Please open an issue or pull request instead, as
described in [CONTRIBUTING.md](CONTRIBUTING.md).

[linux-security-bugs]: https://docs.kernel.org/process/security-bugs.html#what-qualifies-as-a-security-bug

## Supported versions

Security fixes are made for:

| Version                 | Supported                                              |
| ----------------------- | ------------------------------------------------------ |
| `main` (rolling)        | Yes                                                    |
| LTS releases            | Yes, as long as their release branch is maintained     |
| Other tagged releases   | No, update to the rolling release or an LTS release    |

LTS releases are marked as such in their
[GitHub release](https://github.com/canopybmc/canopybmc/releases).

The version running on a BMC is shown in the web UI and in Redfish, see
[CONTRIBUTING.md](CONTRIBUTING.md#reporting-issues).

## CanopyBMC or upstream?

CanopyBMC is a distribution of OpenBMC. Report vulnerabilities to CanopyBMC
when they are in:

- the CanopyBMC layers of this repository, including the vendor layers and
  the patches they apply to upstream components
- the CanopyBMC configuration of upstream components, e.g. enabled features,
  default settings or credentials

Vulnerabilities in upstream components that also affect upstream OpenBMC
belong to the upstream project, e.g. through the
[OpenBMC security process](https://github.com/openbmc/docs/blob/master/security/how-to-report-a-security-vulnerability.md).
If you are not sure, report to CanopyBMC, and we'll either coordinate it with
upstream or direct you to the correct contact.

## Reporting software vulnerabilities

**Please don't report vulnerabilities through public issues, discussions or
pull requests.**

### What to include

A report must include:

1. The affected component, and the layer or upstream repository it comes from.
2. The security issue, including a basic justification of why it is a
   security issue.
3. The machine and the version the vulnerability is demonstrated on.
4. To your best effort, the earliest commit or version that is affected.
5. A script or example of how to reproduce the issue, running against a BMC.
6. Quotes from any relevant specifications governing the vulnerability.
7. If you have one, a proposed patch to fix the issue.
8. Your name, so we can credit you when the issue is disclosed. If you don't
   want to be credited, please state that you want to stay anonymous.

Before reporting, make sure the issue reproduces on a supported version, or
built from a commit less than a week old. If you have found multiple
vulnerabilities, please create one report per vulnerability.

### Creating a report

Open a draft security advisory through
[Report a vulnerability](https://github.com/canopybmc/canopybmc/security/advisories/new)
on the Security tab of this repository. It should contain:

- A "Description" that contains the complete discussion of the issue, covering
  points 2 to 7 above.
- "Weaknesses" that list the CWEs which are considered relevant.
- If you want to be credited or stay anonymous.

The details must be in the "Description" field and only be related to the issue
found. Binary attachments, such as PDFs or compressed archives, are disregarded.

If you don't have a GitHub account, send the report to
[cybersecurity at 9elements dot com](mailto:cybersecurity@9elements.com) instead.
The email **MUST** be plain text and **MUST NOT** contain binary attachments.

### Resolution process

- We acknowledge the report within 5 working days.
- A member of the Canopy team takes care of triaging the report. The team
  makes a preliminary analysis, proposes a fix and keeps you updated on
  the progress.
- Fixes are made in private and released for all supported versions at the
  same time, together with a GitHub security advisory.
- Vulnerabilities that also affect upstream projects are reported to them, and
  disclosure is coordinated with them.
- You are credited in the advisory, unless you prefer not to be.

See the [CERT Guide to Coordinated Vulnerability Disclosure][cert-cvd] for
further considerations.

[cert-cvd]: https://resources.sei.cmu.edu/asset_files/SpecialReport/2017_003_001_503340.pdf

If you feel your report is not being handled in a fair or reasonable manner,
write to [cybersecurity at 9elements dot com](mailto:cybersecurity@9elements.com)
to escalate it. Include the link to the draft security advisory and the
circumstances that made you escalate.

## Reporting hardware vulnerabilities

Report hardware vulnerabilities affecting CanopyBMC to
[cybersecurity at 9elements dot com](mailto:cybersecurity@9elements.com),
including:

- The affected hardware component and machine.
- Its relationship to CanopyBMC.
- Whether, and how, it has been reported to the hardware vendor.

Software mitigations may be applied on a case-by-case basis. Vulnerabilities
that affect upstream OpenBMC as well are coordinated with the OpenBMC
security team.
