#!/usr/bin/env bash
# Required environment:
#   GH_TOKEN        GitHub app token
#   REPO            owner/name (usually canopybmc/canopybmc)
#   READY_LABEL     label requesting the merge
#   OVERRIDE_LABEL  label allowing a merge despite FirmwareCI failures
#   FAILED_LABEL    label marking a pull request rejected by the gate
#   FWCI_APP        app slug that publishes the FirmwareCI check runs
#   BUILD_WORKFLOW  workflow file that builds and uploads to FirmwareCI
#   MERGE_METHOD    merge, rebase or squash
#   BOT_LOGIN       login of the bot enabling auto-merge
#   TEAM            slug of the organization team allowed to set READY_LABEL and OVERRIDE_LABEL
# Optional environment:
#   PR_NUMBER       only evaluate this pull request
#   GRACE_SECONDS   wait after build completion for FirmwareCI checks to appear
#   BASE_BRANCHES   space-separated glob patterns of base branches the gate merges into
#                   (default: repository default branch). They also need a ruleset requiring
#                   pull request reviews, otherwise pull requests are rejected.

set -euo pipefail

GRACE_SECONDS="${GRACE_SECONDS:-90}"
BASE_BRANCHES="${BASE_BRANCHES:-$(gh api "repos/${REPO}" --jq .default_branch)}"
read -ra base_patterns <<<"$BASE_BRANCHES"
DOCS_ONLY_REGEX='^(doc/.*|README\.md)$'
FAILED_CONCLUSIONS='["failure","timed_out","cancelled","action_required","startup_failure","stale"]'
ORG="${REPO%%/*}"

declare -A team_member_cache

log() {
	echo "PR #${pr}: $*"
}

strip_bot() {
	echo "${1%\[bot\]}"
}

disable_auto_merge() {
	local enabled_by="$1"

	if [[ -n "$enabled_by" && "$(strip_bot "$enabled_by")" == "$(strip_bot "$BOT_LOGIN")" ]]; then
		log "disabling auto-merge"
		gh pr merge "$pr" --repo "$REPO" --disable-auto
	fi
}

is_team_member() {
	local user="$1"
	local state
	local err

	if [[ -z "${team_member_cache[$user]+set}" ]]; then
		err=$(mktemp)
		if state=$(gh api "orgs/${ORG}/teams/${TEAM}/memberships/${user}" --jq .state 2>"$err"); then
			team_member_cache[$user]=$([[ "$state" == "active" ]] && echo true || echo false)
		elif grep -q "HTTP 404" "$err"; then
			team_member_cache[$user]=false
		else
			cat "$err" >&2
			rm -f "$err"
			return 2
		fi
		rm -f "$err"
	fi

	[[ "${team_member_cache[$user]}" == "true" ]]
}

# Login of whoever added the label most recently.
label_actor() {
	local label="$1"

	gh api --paginate "repos/${REPO}/issues/${pr}/events?per_page=100" \
		--jq ".[] | select(.event == \"labeled\" and .label.name == \"${label}\") | .actor.login" |
		tail -n 1
}

# Removes the label unless it was added by a team member.
check_label_actor() {
	local label="$1"
	local actor
	local rc=0
	local by=""
	local body

	# Called in a condition, so set -e does not apply here.
	actor=$(label_actor "$label") || exit 1
	if [[ -n "$actor" ]]; then
		by=" (added by @${actor})"
		is_team_member "$actor" || rc=$?
		if ((rc == 0)); then
			return 0
		elif ((rc != 1)); then
			exit "$rc"
		fi
	fi

	log "rejecting: ${label} added by ${actor:-unknown}, not a member of ${ORG}/${TEAM}"
	body=$(
		cat <<-EOF
			:no_entry: **\`${label}\` can only be set by members of the \`${ORG}/${TEAM}\` team**

			Removed \`${label}\`${by}.
		EOF
	)

	gh pr edit "$pr" --repo "$REPO" --remove-label "$label"
	gh pr comment "$pr" --repo "$REPO" --body "$body"
	return 1
}

enable_auto_merge() {
	local sha="$1"
	local enabled_by="$2"

	if [[ -n "$enabled_by" ]]; then
		log "auto-merge already enabled by ${enabled_by}"
		return
	fi

	log "enabling auto-merge (${MERGE_METHOD})"
	gh pr merge "$pr" --repo "$REPO" --auto "--${MERGE_METHOD}" --match-head-commit "$sha"
}

reject() {
	local reason="$1"
	local details="$2"
	local body

	log "rejecting: ${reason}"
	body=$(
		cat <<-EOF
			:x: **${reason}**

			Replaced \`${READY_LABEL}\` with \`${FAILED_LABEL}\`, this pull request will not be merged automatically.

			${details}

			Please check the failures first. Once fixed, push a new revision or re-run the failed FirmwareCI tests, and re-add \`${READY_LABEL}\`.
			If the failures are known and unrelated to this change, add \`${OVERRIDE_LABEL}\` together with \`${READY_LABEL}\` to merge anyway.
		EOF
	)

	gh pr edit "$pr" --repo "$REPO" --remove-label "$READY_LABEL" --add-label "$FAILED_LABEL"
	gh pr comment "$pr" --repo "$REPO" --body "$body"
}

base_allowed() {
	local base="$1"
	local pattern

	for pattern in "${base_patterns[@]}"; do
		# shellcheck disable=SC2053 # glob match intended
		if [[ "$base" == $pattern ]]; then
			return 0
		fi
	done
	return 1
}

# Without a review requirement, auto-merge would merge immediately.
base_requires_review() {
	local base="$1"
	local rules

	rules=$(gh api "repos/${REPO}/rules/branches/${base}" --jq '[.[] | select(.type == "pull_request")] | length') || exit 1
	((rules > 0))
}

reject_base() {
	local base="$1"
	local reason="$2"
	local body

	log "rejecting: base branch ${base}: ${reason}"
	body=$(
		cat <<-EOF
			:warning: **Automatic merging is not supported for pull requests into \`${base}\`**

			${reason} Removed \`${READY_LABEL}\`, please merge manually.
		EOF
	)

	gh pr edit "$pr" --repo "$REPO" --remove-label "$READY_LABEL"
	gh pr comment "$pr" --repo "$REPO" --body "$body"
}

evaluate() {
	local info
	local sha
	local draft
	local labels
	local enabled_by
	local has_ready
	local has_override
	local has_failed
	local base

	info=$(gh pr view "$pr" --repo "$REPO" --json baseRefName,headRefOid,isDraft,state,labels,autoMergeRequest)

	if [[ "$(jq -r .state <<<"$info")" != "OPEN" ]]; then
		log "pr status is not open"
		return
	fi

	base=$(jq -r .baseRefName <<<"$info")
	sha=$(jq -r .headRefOid <<<"$info")
	draft=$(jq -r .isDraft <<<"$info")
	labels=$(jq -c '[.labels[].name]' <<<"$info")
	enabled_by=$(jq -r '.autoMergeRequest.enabledBy.login // ""' <<<"$info")
	has_ready=$(jq --arg l "$READY_LABEL" 'index($l) != null' <<<"$labels")
	has_override=$(jq --arg l "$OVERRIDE_LABEL" 'index($l) != null' <<<"$labels")
	has_failed=$(jq --arg l "$FAILED_LABEL" 'index($l) != null' <<<"$labels")

	# Drafts are skipped until marked ready for review, which triggers a new
	# evaluation. Only make sure they will not be merged right away then.
	if [[ "$draft" == "true" ]]; then
		log "draft, skipping"
		disable_auto_merge "$enabled_by"
		return
	fi

	if [[ "$has_ready" != "true" ]]; then
		disable_auto_merge "$enabled_by"
		return
	fi

	if ! check_label_actor "$READY_LABEL"; then
		disable_auto_merge "$enabled_by"
		return
	fi

	if ! base_allowed "$base"; then
		disable_auto_merge "$enabled_by"
		reject_base "$base" "Only pull requests into \`${BASE_BRANCHES// /\`, \`}\` are built and tested by FirmwareCI."
		return
	fi

	if ! base_requires_review "$base"; then
		disable_auto_merge "$enabled_by"
		reject_base "$base" "The branch has no ruleset requiring a pull request review, so auto-merge would merge without approval."
		return
	fi

	if [[ "$has_override" == "true" ]] && ! check_label_actor "$OVERRIDE_LABEL"; then
		has_override=false
	fi

	# Re-adding the ready label starts a new attempt.
	if [[ "$has_failed" == "true" ]]; then
		log "removing ${FAILED_LABEL}"
		gh pr edit "$pr" --repo "$REPO" --remove-label "$FAILED_LABEL"
	fi

	local files
	files=$(gh api --paginate "repos/${REPO}/pulls/${pr}/files?per_page=100" --jq '.[].filename')
	if [[ -n "$files" ]] && ! grep -qvE "$DOCS_ONLY_REGEX" <<<"$files"; then
		log "documentation-only change, no CI expected"
		enable_auto_merge "$sha" "$enabled_by"
		return
	fi

	local run
	local run_status
	local run_conclusion
	local run_updated
	local run_url

	run=$(gh api "repos/${REPO}/actions/workflows/${BUILD_WORKFLOW}/runs?head_sha=${sha}&per_page=100" \
		--jq '.workflow_runs | sort_by(.created_at) | last // empty')
	if [[ -z "$run" ]]; then
		log "no ${BUILD_WORKFLOW} run for ${sha} yet, waiting"
		disable_auto_merge "$enabled_by"
		return
	fi

	run_status=$(jq -r .status <<<"$run")
	run_conclusion=$(jq -r .conclusion <<<"$run")
	run_updated=$(jq -r .updated_at <<<"$run")
	run_url=$(jq -r .html_url <<<"$run")

	if [[ "$run_status" != "completed" ]]; then
		log "${BUILD_WORKFLOW} is ${run_status}, waiting"
		disable_auto_merge "$enabled_by"
		return
	fi

	if [[ "$run_conclusion" != "success" ]]; then
		disable_auto_merge "$enabled_by"
		reject "Build failed" "The [build workflow run](${run_url}) concluded with \`${run_conclusion}\`, so FirmwareCI did not test all images."
		return
	fi

	# FirmwareCI creates its check runs right after the upload at the end of
	# the build workflow. Give it a moment so none are missed.
	local age
	age=$(($(date +%s) - $(date -d "$run_updated" +%s)))
	if ((age < GRACE_SECONDS)); then
		log "build finished ${age}s ago, waiting for FirmwareCI check runs"
		sleep $((GRACE_SECONDS - age))
	fi

	local checks
	local pending
	local failed
	checks=$(gh api --paginate "repos/${REPO}/commits/${sha}/check-runs?filter=latest&per_page=100" \
		--jq ".check_runs[] | select(.app.slug == \"${FWCI_APP}\") | {name, status, conclusion, url: .details_url}" |
		jq -s .)

	if [[ "$(jq length <<<"$checks")" -eq 0 ]]; then
		log "no FirmwareCI check runs yet, waiting..."
		disable_auto_merge "$enabled_by"
		return
	fi

	pending=$(jq '[.[] | select(.status != "completed")] | length' <<<"$checks")
	if ((pending > 0)); then
		log "${pending} FirmwareCI check run(s) pending, waiting..."
		disable_auto_merge "$enabled_by"
		return
	fi

	failed=$(jq -r --argjson bad "$FAILED_CONCLUSIONS" \
		'.[] | select(.conclusion as $c | $bad | index($c)) | "- [\(.name)](\(.url)): `\(.conclusion)`"' <<<"$checks")
	if [[ -n "$failed" ]]; then
		if [[ "$has_override" == "true" ]]; then
			log "FirmwareCI failed, but ${OVERRIDE_LABEL} is set"
		else
			disable_auto_merge "$enabled_by"
			reject "FirmwareCI failed" "The following FirmwareCI checks failed for ${sha}:"$'\n\n'"${failed}"
			return
		fi
	fi

	enable_auto_merge "$sha" "$enabled_by"
}

if [[ -n "${PR_NUMBER:-}" ]]; then
	prs="$PR_NUMBER"
else
	# Labeled non-draft pull requests plus those the bot enabled auto-merge on,
	# so that auto-merge gets disabled again once the label is removed.
	prs=$(gh pr list --repo "$REPO" --state open --limit 200 \
		--json number,isDraft,labels,autoMergeRequest |
		jq -r --arg l "$READY_LABEL" --arg bot "$(strip_bot "$BOT_LOGIN")" '
			.[] | select(
				(any(.labels[]; .name == $l) and (.isDraft | not))
				or ((.autoMergeRequest.enabledBy.login // "") | sub("\\[bot\\]$"; "")) == $bot
			) | .number')
fi

for pr in $prs; do
	evaluate
done
