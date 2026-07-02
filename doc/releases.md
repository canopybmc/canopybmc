# Releases

## Usage
Creating a release is a semi-automatic process.

Steps:
1) Make sure that all CI jobs are passing on your branch of choice (likely `main`).
2) In GitHub web UI, actions tab, manually trigger `release-prepare` workflow (do not forget to input the desired new version name).
3) Wait until a Pull Request (PR) is created called `Release: ...`.
4) Review the changes in this PR, and if everything is correct, then approve and merge the PR.
5) Wait for automatic new release.

After the PR is merged, the CI workflow to build, test and then release will be triggered. Once the CI completes successfully, there will be a new release in GitHub.


## Description

The release CI is a bit complicated, to accommodate the project specific quirks.

There are following workflows:
- `release-prepare.yml`
    - this workflow can be triggered manually, it will open a pull request with all changes needed for a new release
- `release-tag-create.yml`
    - this workflow is triggered automatically when the pull request made by `release-prepare.yml` is merged
        - the pull request title must start with `Release:`, be merged and author must be `github-actions[bot]` as defined in the `release-tag-create.yml` workflow
    - it creates a tag for the release
- `build.yml`
    - this workflow typically only does builds, but it also runs on pushed `tag`
    - when on `tag`, then it will also run the `release` job, which does the actual release itself thought [GH Release](https://github.com/marketplace/actions/gh-release) action


### Why build twice? In PR from `release-prepare.yml` and then again in `build.yml`?
The `build.yml` should be run on top of a `tag`, to get the correct version string in the compiled binaries.

However, the PR from `release-prepare.yml` the build is on a ordinary commit, as it should be - we want to review all the changes before actual release or `tag` creation.

### Using `paths-ignore` in `build.yml` for `push` trigger
According to [GitHub documentation](https://docs.github.com/en/actions/reference/workflows-and-actions/workflow-syntax#onpushpull_requestpull_request_targetpathspaths-ignore):

> Path filters are not evaluated for pushes of tags.
