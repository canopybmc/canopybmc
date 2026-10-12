// @ts-check
// SPDX-License-Identifier: GPL-2.0-only
//
// GitHub API helpers for issue.js and pr.js. They add labels that exist in
// the repository, find the release a version or commit belongs to, and
// create or update the triage comment.
//
// Every function takes the Octokit client from actions/github-script as
// `github`.
'use strict';

const RELEASE_BRANCH_RE = /^\d{4}\.\d{2}$/;

// canopybmc/gh-provisioning manages the labels. Adding a missing label
// through the API creates it without warning, so every label goes through
// this filter.
async function existingLabels(github, { owner, repo }) {
  const labels = await github.paginate(github.rest.issues.listLabelsForRepo, {
    owner,
    repo,
    per_page: 100,
  });
  return new Set(labels.map((l) => l.name));
}

async function addLabels(github, { owner, repo }, number, labels) {
  if (labels.length === 0) return;
  await github.rest.issues.addLabels({ owner, repo, issue_number: number, labels });
}

async function commitsOnTop(github, { owner, repo }, branch, sha) {
  try {
    const { data } = await github.rest.repos.compareCommits({ owner, repo, base: branch, head: sha });
    return data.ahead_by;
  } catch (e) {
    if (e.status === 404 || e.status === 422) return null;
    throw e;
  }
}

async function pullRequestForCommit(github, { owner, repo }, sha) {
  const { data: commit } = await github.rest.repos.getCommit({ owner, repo, ref: sha });
  const head = commit.parents.length === 2 ? commit.parents[1].sha : commit.sha;
  const { data: prs } = await github.rest.repos.listPullRequestsAssociatedWithCommit({ owner, repo, commit_sha: head });
  return prs[0] || null;
}

async function commitExists(github, { owner, repo }, sha) {
  try {
    await github.rest.repos.getCommit({ owner, repo, ref: sha });
    return true;
  } catch (e) {
    if (e.status === 404 || e.status === 422) return false;
    throw e;
  }
}

// Returns { label, note } for the release a version belongs to. A build of
// an untagged commit belongs to the branch it is based on. That is the
// branch the commit has the fewest commits on top of.
async function releaseForVersion(github, repo, defaultBranch, version) {
  if (!version.hash) return { label: `release/${version.base}` };

  const branches = await github.paginate(github.rest.repos.listBranches, { ...repo, per_page: 100 });
  const candidates = [defaultBranch, ...branches.map((b) => b.name).filter((b) => RELEASE_BRANCH_RE.test(b))];
  let best = null;
  for (const branch of candidates) {
    const onTop = await commitsOnTop(github, repo, branch, version.hash);
    // A commit at the branch point is on both branches. Prefer the release
    // the version names.
    if (onTop !== null && (best === null || onTop < best.onTop ||
        (onTop === best.onTop && branch === version.base))) {
      best = { branch, onTop };
    }
  }
  if (best === null) {
    return {
      note: `Commit \`${version.hash}\` is not in this repository, so the triage can't tell which release ` +
        'is affected. Was this built from a fork or with local changes?',
    };
  }

  const label = best.branch === defaultBranch ? 'release/rolling' : `release/${best.branch}`;
  if (best.onTop === 0) return { label };

  const pr = await pullRequestForCommit(github, repo, version.hash);
  return {
    label,
    note: `Commit \`${version.hash}\` is not on any branch. It is ${best.onTop} commit(s) on top of ` +
      `\`${best.branch}\`` + (pr ? ` and comes from pull request #${pr.number}.` : '.'),
  };
}

// Creates the bot comment that contains `marker`, or updates it if it
// already exists.
async function upsertComment(github, { owner, repo }, number, marker, body) {
  const comments = await github.paginate(github.rest.issues.listComments, {
    owner,
    repo,
    issue_number: number,
    per_page: 100,
  });
  const existing = comments.find((c) => c.user?.type === 'Bot' && c.body?.includes(marker));
  const full = `${marker}\n${body}`;
  if (existing) {
    if (existing.body !== full) {
      await github.rest.issues.updateComment({ owner, repo, comment_id: existing.id, body: full });
    }
  } else {
    await github.rest.issues.createComment({ owner, repo, issue_number: number, body: full });
  }
}

module.exports = {
  RELEASE_BRANCH_RE,
  existingLabels,
  addLabels,
  commitExists,
  releaseForVersion,
  upsertComment,
};
