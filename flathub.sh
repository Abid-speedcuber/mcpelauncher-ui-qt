#!/bin/sh
set -eu

APP_ID="io.github.Abid_speedcuber.mcpelauncher"
BUNDLE="${BUNDLE:-unofficial-bedrock-apk-launcher.flatpak}"
REMOTE="${REMOTE:-origin}"
BRANCH="${BRANCH:-gh-pages}"
REPO_URL="${REPO_URL:-https://Abid-speedcuber.github.io/mcpelauncher-ui-qt/repo/}"
REMOTE_FILE_URL="${REMOTE_FILE_URL:-https://Abid-speedcuber.github.io/mcpelauncher-ui-qt/mcpelauncher.flatpakrepo}"
TAG="${1:-${TAG:-}}"

need() {
    if ! command -v "$1" >/dev/null 2>&1; then
        echo "Missing required command: $1" >&2
        exit 1
    fi
}

need flatpak
need git
need rsync

if [ ! -d repo/objects ]; then
    echo "Missing ./repo. Run ./build.sh first." >&2
    exit 1
fi

if [ ! -f "$BUNDLE" ]; then
    echo "Missing ${BUNDLE}. Run ./build.sh first." >&2
    exit 1
fi

echo "==> Refreshing Flatpak repo metadata"
flatpak build-update-repo repo
./scripts/write-flatpakrepo.sh "$REPO_URL" mcpelauncher.flatpakrepo

echo "==> Publishing Flatpak update repo to ${REMOTE}/${BRANCH}"
REMOTE="$REMOTE" BRANCH="$BRANCH" REPO_URL="$REPO_URL" ./scripts/publish-flatpak-repo-gh-pages.sh

if [ -n "$TAG" ]; then
    need gh
    if ! gh auth status >/dev/null 2>&1; then
        echo "GitHub CLI is not logged in. Run: gh auth login" >&2
        exit 1
    fi
    if [ -n "$(git status --short)" ]; then
        echo "Working tree has uncommitted changes." >&2
        echo "Commit them first so release tag ${TAG} points at the exact source you built." >&2
        exit 1
    fi

    echo "==> Publishing GitHub release ${TAG}"
    if git rev-parse "$TAG" >/dev/null 2>&1; then
        echo "Using existing tag ${TAG}"
    else
        git tag -a "$TAG" -m "Release ${TAG}"
        git push "$REMOTE" "$TAG"
    fi

    gh release create "$TAG" \
        "$BUNDLE" \
        "mcpelauncher.flatpakrepo" \
        --title "Minecraft Pocket Edition Launcher ${TAG}" \
        --notes "Install the update remote once:

\`\`\`sh
flatpak remote-add --user --if-not-exists mcpelauncher ${REMOTE_FILE_URL}
flatpak install --user mcpelauncher ${APP_ID}
\`\`\`

After that, update with:

\`\`\`sh
flatpak update ${APP_ID}
\`\`\`

The standalone .flatpak bundle is also attached for manual installs." \
        || gh release upload "$TAG" "$BUNDLE" "mcpelauncher.flatpakrepo" --clobber
fi

echo
echo "Published:"
echo "  Repo URL:      ${REPO_URL}"
echo "  Remote file:   ${REMOTE_FILE_URL}"
echo "  Install app:   flatpak install --user mcpelauncher ${APP_ID}"
echo
echo "Note: this publishes your own Flatpak repository, not official Flathub."
echo "Official Flathub still requires a reviewed PR against flathub/flathub:new-pr."
