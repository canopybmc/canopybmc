# Releases

Creating a release is a semi-automatic process.

Steps:
1) Make sure that all CI jobs are passing on your branch of choice (likely `main`).
2) In GitHub web UI, actions tab, manually trigger `release-prepare` workflow (do not forget to input the desired new version name).
3) Wait until a Pull Request (PR) is created called `Release: ...`.
4) Review the changes in this PR, and if everything is correct, then approve and merge the PR.
5) Wait for automatic new release.

After the PR is merged, the CI workflow to build, test and then release will be triggered. Once the CI completes successfully, there will be a new release in GitHub.
