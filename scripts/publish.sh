#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
remote_url=${1:?Usage: bash scripts/publish.sh https://github.com/OWNER/REPOSITORY.git}
if [[ ! "$remote_url" =~ ^https://github\.com/[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+(\.git)?$ ]]; then
  echo 'Use a GitHub HTTPS URL without tokens or embedded credentials.' >&2; exit 1
fi
if [ -n "$(git status --porcelain)" ]; then
  echo 'Commit or preserve local changes before publishing.' >&2; exit 1
fi
refs=(main develop feature/hal-spi-tft feature/hal-adc-gpio feature/hal-i2c-pwm)
for ref in "${refs[@]}"; do git show-ref --verify --quiet "refs/heads/$ref"; done
existing=$(git ls-remote "$remote_url")
if [ -n "$existing" ]; then
  echo 'Remote is not empty. Stop and review its history before publishing.' >&2; exit 1
fi
if git remote get-url origin >/dev/null 2>&1; then
  if [ "$(git remote get-url origin)" != "$remote_url" ]; then
    echo 'origin already points elsewhere. Review before changing it.' >&2; exit 1
  fi
else
  git remote add origin "$remote_url"
fi
git push -u origin "${refs[@]}"
echo 'Branches pushed. Configure members and branch protection on GitHub.'
