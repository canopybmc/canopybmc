# Merging pull requests

> [!NOTE]
> This is primarily relevant for maintainers with merge permission!

FirmwareCI reports one check run per test. Given that the names are dynamic and
may change we cannot add them to the "required status checks" list, as it would
otherwise break the merge process.

To workaround this GitHub limitation, a workflow has been implemented that
enforces the status checks and merges PRs via a label instead.

## Usage

1) Check that the target branch is `main` or a `stable/*` branch.
2) Ensure that you are part of the `9e-developers` team.
3) Add the `merge/ready` label to the pull request.
4) Once the build workflow and every FirmwareCI check for the latest revision
   passed, `canopybmc-bot` enables auto-merge (rebase). GitHub then merges as
   soon as the usual branch protection (approval, resolved conversations) is
   satisfied.

If the build or any FirmwareCI check fails, the bot replaces `merge/ready` with
`merge/failed` and leaves a comment listing the failed checks.
Once a fix got pushed (or the CI re-triggered), add `merge/ready` again.
`merge/failed` is then removed automatically when new commits are pushed or
`merge/ready` is added again, so it always refers to the current revision.

If the failures are known and unrelated to the change, add
`merge/ignore-firmwareci` together with `merge/ready`. The pull request is then
merged once all FirmwareCI checks have finished, regardless of their result.
The override label is removed automatically when new commits are pushed.

## Handling of draft PRs

Draft PRs will be ignore. The workflow only runs once the pull request is marked
ready for review. When a pull request is converted to draft, auto-merge enabled
by the bot is disabled so it cannot be merged right away once it is marked ready
again.

## Target branches

Only pull requests into the default branch (`main`) and stable release
branches (`stable/*`, e.g. `stable/2026.06` for backports) can be merged with
labels. These are the branches `build.yml` builds and tests with FirmwareCI.

Additionally, the target branch must be covered by a ruleset requiring a pull
request review. Otherwise auto-merge would merge immediately without approval.

For pull requests into any other branch the bot removes `merge/ready` and asks
to merge manually. This also applies when the base branch of a pull request is
changed later.

Stable branches must be created from a `main` revision that already includes
`stable/**` in the `build.yml` triggers, otherwise pull requests into them are
not built and the gate waits forever.

The allowed branches are configured in `BASE_BRANCHES` in
`.github/workflows/firmwareci-merge-gate.yml` and must match the `push` and
`pull_request` branch filters in `.github/workflows/build.yml`.

## Description

The workflow evaluates:
- `merge/ready` or `merge/ignore-firmwareci` being added or removed
- new commits being pushed, or a draft being marked ready for review
- start and completion of the `Build, test and release` workflow, including
  re-runs
- creation, re-request and completion of any FirmwareCI check run
- manual `workflow_dispatch`, optionally for a single pull request

Re-triggering a FirmwareCI test creates a new check run with the same name, thus
only the newest check is token into account. As a re-run is pending again it'll
disable auto-merge until it finished. A passing re-run will superseed the failed
run.

Rejecting means replacing `merge/ready` with `merge/failed` and commenting.

Auto-merge enabled by the bot is disabled again when `merge/ready` is removed
or new commits are pushed, so a revision is never merged before its own
FirmwareCI run finished.

The bot token (`CANOPYBMC_BOT_*` secrets) is used so the resulting push to
`main` triggers the build workflow.
