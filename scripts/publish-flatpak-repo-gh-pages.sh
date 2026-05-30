#!/bin/sh
set -eu

REMOTE="${REMOTE:-origin}"
BRANCH="${BRANCH:-gh-pages}"
REPO_URL="${REPO_URL:-https://Abid-speedcuber.github.io/mcpelauncher-ui-qt/repo/}"
WORKTREE="${WORKTREE:-.flatpak-repo-pages}"

if [ ! -d repo/objects ]; then
    echo "Missing ./repo. Run ./scripts/build-flatpak-bundle.sh first." >&2
    exit 1
fi

flatpak build-update-repo repo
./scripts/write-flatpakrepo.sh "$REPO_URL" mcpelauncher-minimal.flatpakrepo

if [ -e "$WORKTREE/.git" ]; then
    git -C "$WORKTREE" fetch "$REMOTE" "$BRANCH" >/dev/null 2>&1 || true
    if git -C "$WORKTREE" rev-parse --verify "$BRANCH" >/dev/null 2>&1; then
        git -C "$WORKTREE" switch "$BRANCH"
    elif git ls-remote --exit-code --heads "$REMOTE" "$BRANCH" >/dev/null 2>&1; then
        git -C "$WORKTREE" switch -c "$BRANCH" --track "$REMOTE/$BRANCH"
    else
        git -C "$WORKTREE" switch --orphan "$BRANCH"
        git -C "$WORKTREE" rm -rf . >/dev/null 2>&1 || true
    fi
else
    if git ls-remote --exit-code --heads "$REMOTE" "$BRANCH" >/dev/null 2>&1; then
        if git rev-parse --verify "$BRANCH" >/dev/null 2>&1; then
            git worktree add "$WORKTREE" "$BRANCH"
        else
            git worktree add -b "$BRANCH" "$WORKTREE" "$REMOTE/$BRANCH"
        fi
    else
        git worktree add --detach "$WORKTREE"
        git -C "$WORKTREE" switch --orphan "$BRANCH"
        git -C "$WORKTREE" rm -rf . >/dev/null 2>&1 || true
    fi
fi

mkdir -p "$WORKTREE/repo"
rsync -a --delete repo/ "$WORKTREE/repo/"
cp mcpelauncher-minimal.flatpakrepo "$WORKTREE/"

cat > "$WORKTREE/index.html" <<'EOF'
<!doctype html>
<meta charset="utf-8">
<title>Unofficial Bedrock APK Launcher Flatpak repo</title>
<h1>Unofficial Bedrock APK Launcher Flatpak repo</h1>
<p><a href="mcpelauncher-minimal.flatpakrepo">Download the Flatpak remote file</a></p>
EOF

git -C "$WORKTREE" add .
git -C "$WORKTREE" commit -m "Publish Flatpak repository" || true
git -C "$WORKTREE" push "$REMOTE" "$BRANCH"

echo "Published ${REPO_URL}"
