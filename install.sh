#!/usr/bin/env bash
set -euo pipefail

REPO="avighnac/libapfs"
BIN_NAME="apfs"
INSTALL_PATH="/usr/local/bin/$BIN_NAME"

RED="$(printf '\033[31m')"
GREEN="$(printf '\033[32m')"
BLUE="$(printf '\033[34m')"
BOLD="$(printf '\033[1m')"
RESET="$(printf '\033[0m')"

log() { printf "${BLUE}==>${RESET} %s\n" "$*"; }
ok()  { printf "${GREEN}✔${RESET} %s\n" "$*"; }
err() { printf "${RED}✘${RESET} %s\n" "$*" >&2; }

need() {
  command -v "$1" >/dev/null 2>&1 || {
    err "missing dependency: $1"
    exit 1
  }
}

need curl
need uname
need chmod
need mktemp
need sed
need grep

OS_RAW="$(uname -s)"
ARCH_RAW="$(uname -m)"

case "$OS_RAW" in
  Linux)  OS="linux" ;;
  Darwin) OS="macos" ;;
  *)
    err "unsupported OS: $OS_RAW"
    exit 1
    ;;
esac

case "$ARCH_RAW" in
  x86_64|amd64)  ARCH="x86-64" ;;
  arm64|aarch64) ARCH="arm64" ;;
  *)
    err "unsupported architecture: $ARCH_RAW"
    exit 1
    ;;
esac

ASSET_NAME="apfs-$OS-$ARCH"

log "detected platform: $OS / $ARCH"

RELEASES_JSON="$(curl -fsSL "https://api.github.com/repos/$REPO/releases")"

TAG="$(printf '%s\n' "$RELEASES_JSON" | sed -n 's/.*"tag_name":[[:space:]]*"\([^"]*\)".*/\1/p' | head -n1)"

if [ -z "$TAG" ]; then
  err "could not determine latest release tag"
  exit 1
fi

DOWNLOAD_URL="https://github.com/$REPO/releases/download/$TAG/$ASSET_NAME"

TMP_FILE="$(mktemp)"
trap 'rm -f "$TMP_FILE"' EXIT

log "downloading ${BOLD}$ASSET_NAME${RESET} from ${BOLD}$TAG${RESET}"

curl -fsSL "$DOWNLOAD_URL" -o "$TMP_FILE"
chmod +x "$TMP_FILE"

log "installing to ${BOLD}$INSTALL_PATH${RESET}"

if [ -w /usr/local/bin ]; then
  mv "$TMP_FILE" "$INSTALL_PATH"
else
  sudo mv "$TMP_FILE" "$INSTALL_PATH"
  sudo chmod 755 "$INSTALL_PATH"
fi

ok "installed successfully"
printf "\n"
"$INSTALL_PATH"