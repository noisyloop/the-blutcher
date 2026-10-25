#!/bin/bash
set -euo pipefail
SOURCE="$(cd "$(dirname "$0")" && pwd)"
echo 'The Blucher — Noisyloop development build installer'
echo 'Quit Ableton Live first. Press Return to install for your user account.'
read -r _
install_bundle() {
  local name="$1" destination="$2"
  if [[ ! -d "$SOURCE/$name" ]]; then
    echo "Missing bundle: $name" >&2
    exit 1
  fi
  mkdir -p "$(dirname "$destination")"
  if [[ -e "$destination" ]]; then
    mv "$destination" "$destination.backup-$(date +%Y%m%d%H%M%S)"
  fi
  ditto "$SOURCE/$name" "$destination"
  xattr -dr com.apple.quarantine "$destination" 2>/dev/null || true
  echo "Installed $name"
}
install_bundle 'The Blucher.vst3' "$HOME/Library/Audio/Plug-Ins/VST3/The Blucher.vst3"
install_bundle 'The Blucher.component' "$HOME/Library/Audio/Plug-Ins/Components/The Blucher.component"
install_bundle 'The Blucher.app' "$HOME/Applications/The Blucher.app"
echo 'Restart Ableton, enable VST3 system folders, and Option-click Rescan if needed.'
echo 'Press Return to exit.'
read -r _
