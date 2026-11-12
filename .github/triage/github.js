// @ts-check
'use strict';

const RELEASE_BRANCH_RE = /^\d{4}\.\d{2}$/;

// Labels are managed in canopybmc/gh-provisioning. Adding a label that
// doesn't exist through the API would silently create it, so everything
// goes through this filter.
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

// Returns { label, note } for the release a version belongs to. Builds of
// untagged commits are assigned to the branch they are based on, which is
// the one with the fewest commits missing from it.
async function releaseForVersion(github, repo, defaultBranch, version) {
  if (!version.hash) return { label: `release/${version.base}` };

  const branches = await github.paginate(github.rest.repos.listBranches, { ...repo, per_page: 100 });
  const candidates = [defaultBranch, ...branches.map((b) => b.name).filter((b) => RELEASE_BRANCH_RE.test(b))];
  let best = null;
  for (const branch of candidates) {
    const onTop = await commitsOnTop(github, repo, branch, version.hash);
    // At the branch point a commit is on both, prefer the release the
    // version names.
    if (onTop !== null && (best === null || onTop < best.onTop ||
        (onTop === best.onTop && branch === version.base))) {
      best = { branch, onTop };
    }
  }
  if (best === null) {
    return {
      note: `Commit \`${version.hash}\` is not in this repository, so the affected release could not be ` +
        'determined. Was this built from a fork or local changes?',
    };
  }

  const label = best.branch === defaultBranch ? 'release/rolling' : `release/${best.branch}`;
  if (best.onTop === 0) return { label };

  const pr = await pullRequestForCommit(github, repo, version.hash);
  return {
    label,
    note: `Commit \`${version.hash}\` is not on a branch, it is ${best.onTop} commit(s) on top of ` +
      `\`${best.branch}\`` + (pr ? `, built from pull request #${pr.number}.` : '.'),
  };
}

// Creates or updates the single comment identified by `marker`.
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
