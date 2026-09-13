#!/usr/bin/env bash
set -euo pipefail

# Restore the Mozilla 0.9 Navigator/Communicator chrome directly from the
# official Mozilla 0.9 source archive. This deliberately keeps the period
# sources intact, except for the known stray duplicate URL-bar onkeypress line
# in navigator.xul that current UXP cannot parse.

ARCHIVE_URL="https://archive.mozilla.org/pub/mozilla/releases/mozilla0.9/src/mozilla-source-0.9.tar.gz"
ARCHIVE_SHA256="0074c7c502793b71f874f46c1f1cd7a45fcddee56638cb48d41f9781e3c0c206"

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(git -C "$SCRIPT_DIR" rev-parse --show-toplevel)"
APP="$REPO_ROOT/xulrunner/mozilla09"
WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT

ARCHIVE="$WORK/mozilla-source-0.9.tar.gz"
SRC="$WORK/src/mozilla"

echo "Downloading Mozilla 0.9 source archive..."
curl -fL --retry 4 "$ARCHIVE_URL" -o "$ARCHIVE"
echo "$ARCHIVE_SHA256  $ARCHIVE" | sha256sum -c -
mkdir -p "$WORK/src"
tar -xzf "$ARCHIVE" -C "$WORK/src"

NAV="$APP/chrome/navigator"
COMM="$APP/chrome/communicator"
mkdir -p "$NAV/content" "$NAV/locale" "$NAV/skin/classic"
mkdir -p "$COMM/content" "$COMM/locale" "$COMM/skin/classic"
mkdir -p "$APP/chrome/navigator-region/locale"
mkdir -p "$APP/chrome/communicator-region/locale"

# Navigator content and Unix platform bindings.
cp -a "$SRC/xpfe/browser/resources/content/." "$NAV/content/"
cp -f "$SRC/xpfe/browser/resources/content/unix/platformNavigationBindings.xul" \
      "$NAV/content/platformNavigationBindings.xul"

# Navigator locale and region resources.
cp -a "$SRC/xpfe/browser/resources/locale/en-US/." "$NAV/locale/"
cp -f "$SRC/xpfe/browser/resources/locale/en-US/region.properties" \
      "$APP/chrome/navigator-region/locale/region.properties"

# Shared Communicator content and Unix platform bindings.
cp -a "$SRC/xpfe/communicator/resources/content/." "$COMM/content/"
cp -a "$SRC/xpfe/communicator/resources/locale/en-US/." "$COMM/locale/"
cp -f "$SRC/xpfe/communicator/resources/content/unix/platformBrowserBindings.xul" \
      "$COMM/content/platformBrowserBindings.xul"
cp -f "$SRC/xpfe/communicator/resources/content/unix/platformEditorBindings.xul" \
      "$COMM/content/platformEditorBindings.xul"
cp -f "$SRC/xpfe/communicator/resources/content/unix/platformGlobalOverlay.xul" \
      "$COMM/content/platformGlobalOverlay.xul"
cp -f "$SRC/xpfe/communicator/resources/locale/en-US/region.dtd" \
      "$APP/chrome/communicator-region/locale/region.dtd"
cp -f "$SRC/xpfe/communicator/resources/locale/en-US/taskbar.rdf" \
      "$APP/chrome/communicator-region/locale/taskbar.rdf"

# Sidebar overlay and direct resources use chrome://communicator/... URLs.
mkdir -p "$COMM/content/sidebar" "$COMM/locale/sidebar"
cp -a "$SRC/xpfe/components/sidebar/resources/." "$COMM/content/sidebar/"
find "$SRC/xpfe/components/sidebar/resources" -path '*/locale/*' -type f \
  \( -name '*.dtd' -o -name '*.properties' \) \
  -exec cp -f {} "$COMM/locale/sidebar/" \;

# Bookmarks overlay and direct resources also use communicator URLs.
mkdir -p "$COMM/content/bookmarks" "$COMM/locale/bookmarks"
cp -a "$SRC/xpfe/components/bookmarks/resources/." "$COMM/content/bookmarks/"
find "$SRC/xpfe/components/bookmarks/resources" -path '*/locale/*' -type f \
  \( -name '*.dtd' -o -name '*.properties' \) \
  -exec cp -f {} "$COMM/locale/bookmarks/" \;

# Network security chrome was packaged into communicator in Mozilla 0.9.
cp -f "$SRC/netwerk/resources/content/securityOverlay.xul" "$COMM/content/securityOverlay.xul"
cp -f "$SRC/netwerk/resources/content/securityUI.js" "$COMM/content/securityUI.js"
cp -f "$SRC/netwerk/resources/content/PSMTaskMenu.xul" "$COMM/content/PSMTaskMenu.xul"
cp -f "$SRC/netwerk/resources/locale/en-US/PSMTaskMenu.dtd" "$COMM/locale/PSMTaskMenu.dtd"
cp -f "$SRC/netwerk/resources/locale/en-US/security.properties" "$COMM/locale/security.properties"
cp -f "$SRC/netwerk/resources/locale/en-US/securityOverlay.dtd" "$COMM/locale/securityOverlay.dtd"

# Classic skin. The non-Mac Mozilla 0.9 classic build selected the win
# platform CSS, while the common image/CSS assets lived one directory up.
cp -a "$SRC/themes/classic/navigator/." "$NAV/skin/classic/"
cp -a "$SRC/themes/classic/navigator/win/." "$NAV/skin/classic/"
cp -a "$SRC/themes/classic/communicator/." "$COMM/skin/classic/"
cp -a "$SRC/themes/classic/communicator/win/." "$COMM/skin/classic/"
cp -a "$SRC/themes/classic/communicator/bookmarks/win/." "$COMM/skin/classic/bookmarks/"
cp -a "$SRC/themes/classic/communicator/sidebar/win/." "$COMM/skin/classic/sidebar/"

# The 0.9 source snapshot has one malformed duplicate line immediately after
# the URL bar textbox. Remove only that line; everything else remains original.
sed -i '/onkeypress="if (event.keyCode == 13) { addToUrlbarHistory(); BrowserLoadURL(); }" flex="1"\/>/d' \
       "$NAV/content/navigator.xul"

# Do not stage CVS/build-system metadata as runtime chrome.
find "$APP/chrome" -type d -name CVS -prune -exec rm -rf {} +
find "$APP/chrome" -type f \( \
  -name 'Makefile.in' -o -name 'makefile.win' -o -name 'jar.mn' -o \
  -name 'MANIFEST' -o -name '.cvsignore' -o -name 'contents.rdf' \
\) -delete

cat > "$APP/chrome.manifest" <<'EOF'
content navigator chrome/navigator/content/
locale navigator en-US chrome/navigator/locale/
skin navigator classic/1.0 chrome/navigator/skin/classic/
content communicator chrome/communicator/content/
locale communicator en-US chrome/communicator/locale/
skin communicator classic/1.0 chrome/communicator/skin/classic/
locale navigator-region en-US chrome/navigator-region/locale/
locale communicator-region en-US chrome/communicator-region/locale/
EOF

echo "Mozilla 0.9 chrome restored under $APP/chrome"

if [[ "${1:-}" == "--commit" ]]; then
  git -C "$REPO_ROOT" add xulrunner/mozilla09
  if git -C "$REPO_ROOT" diff --cached --quiet; then
    echo "No changes to commit."
    exit 0
  fi
  git -C "$REPO_ROOT" commit -m "Restore full Mozilla 0.9 Navigator chrome sources"
  git -C "$REPO_ROOT" push origin HEAD:motif-backend
fi
