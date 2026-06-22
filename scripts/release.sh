#!/bin/sh
set -eu

if [ $# -lt 1 ]; then
    echo "Usage: $0 v1.2.3" >&2
    echo "Builds the Flatpak, publishes the update repo, and uploads GitHub Release assets." >&2
    exit 1
fi

TAG="$1"
APP_ID="io.github.Abid_speedcuber.mcpelauncher"
BUNDLE="unofficial-bedrock-apk-launcher.flatpak"
REMOTE_URL="https://abid-speedcuber.github.io/mcpelauncher-ui-qt/mcpelauncher.flatpakrepo"

if ! command -v gh >/dev/null 2>&1; then
    echo "GitHub CLI (gh) is required to create/update the GitHub Release." >&2
    exit 1
fi

if ! gh auth status >/dev/null 2>&1; then
    echo "GitHub CLI is not logged in. Run: gh auth login" >&2
    exit 1
fi

if [ -n "$(git status --short)" ]; then
    echo "Working tree has uncommitted changes." >&2
    echo "Commit them first so the release tag points at the exact source you built." >&2
    exit 1
fi

git fetch origin
CURRENT_BRANCH="$(git branch --show-current)"
git push origin "$CURRENT_BRANCH"

if git rev-parse "$TAG" >/dev/null 2>&1; then
    echo "Using existing tag ${TAG}"
else
    git tag -a "$TAG" -m "Release ${TAG}"
    git push origin "$TAG"
fi

./scripts/build-flatpak-bundle.sh
./scripts/publish-flatpak-repo-gh-pages.sh

gh release create "$TAG" \
    "$BUNDLE" \
    "mcpelauncher.flatpakrepo" \
    --title "Minecraft Pocket Edition Launcher ${TAG}" \
    --notes "Install the update remote once:

\`\`\`sh
flatpak remote-add --user --if-not-exists mcpelauncher ${REMOTE_URL}
flatpak install --user mcpelauncher ${APP_ID}
\`\`\`

After that, update with:

\`\`\`sh
flatpak update ${APP_ID}
\`\`\`

The standalone .flatpak bundle is also attached for manual installs." \
    || gh release upload "$TAG" "$BUNDLE" "mcpelauncher.flatpakrepo" --clobber

echo "Release ${TAG} is published."
echo "Users with the remote can update with: flatpak update ${APP_ID}"
