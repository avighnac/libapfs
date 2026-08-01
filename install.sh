#!/usr/bin/env bash
set -euo pipefail

REPO="avighnac/libapfs"
BIN_NAME="apfs"

BIN_INSTALL_PATH="/usr/local/bin/$BIN_NAME"
INCLUDE_INSTALL_PATH="/usr/local/include"
LIB_INSTALL_PATH="/usr/local/lib"

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

usage() {
  printf "Usage: %s --cli | --lib\n" "$0"
  printf "\n"
  printf "  --cli    Install the apfs command-line executable\n"
  printf "  --lib    Install libapfs.a and its header files\n"
}

if [ "$#" -ne 1 ]; then
  usage
  exit 1
fi

case "$1" in
  --cli)
    MODE="cli"
    ;;
  --lib)
    MODE="lib"
    ;;
  -h|--help)
    usage
    exit 0
    ;;
  *)
    err "unknown option: $1"
    usage
    exit 1
    ;;
esac

need curl
need jq
need uname
need chmod
need mktemp
need rm
need mv
need mkdir

if [ "$MODE" = "lib" ]; then
  need unzip
  need cp
fi

OS_RAW="$(uname -s)"
ARCH_RAW="$(uname -m)"

case "$OS_RAW" in
  Linux)
    OS="linux"
    ;;
  Darwin)
    OS="macos"
    ;;
  *)
    err "unsupported OS: $OS_RAW"
    exit 1
    ;;
esac

case "$ARCH_RAW" in
  x86_64|amd64)
    ARCH="x86-64"
    ;;
  arm64|aarch64)
    ARCH="arm64"
    ;;
  *)
    err "unsupported architecture: $ARCH_RAW"
    exit 1
    ;;
esac

log "detected platform: $OS / $ARCH"

RELEASES_JSON="$(
  curl -fsSL \
    -H "Accept: application/vnd.github+json" \
    -H "X-GitHub-Api-Version: 2022-11-28" \
    "https://api.github.com/repos/$REPO/releases?per_page=100"
)"

# GitHub does not always return releases in publication order.
# Explicitly select the non-draft release with the newest published_at value.
TAG="$(
  printf '%s\n' "$RELEASES_JSON" |
    jq -r '
      map(
        select(
          .draft == false and
          .published_at != null
        )
      )
      | max_by(.published_at)
      | .tag_name // empty
    '
)"

if [ -z "$TAG" ]; then
  err "could not determine latest release tag"
  exit 1
fi

if [ "$MODE" = "cli" ]; then
  ASSET_NAME="apfs-$OS-$ARCH"
else
  ASSET_NAME="libapfs-$OS-$ARCH.zip"
fi

DOWNLOAD_URL="https://github.com/$REPO/releases/download/$TAG/$ASSET_NAME"

TMP_DIR="$(mktemp -d)"
trap 'rm -rf "$TMP_DIR"' EXIT

TMP_FILE="$TMP_DIR/$ASSET_NAME"

log "latest release: ${BOLD}$TAG${RESET}"
log "downloading ${BOLD}$ASSET_NAME${RESET}"

if ! curl -fsSL "$DOWNLOAD_URL" -o "$TMP_FILE"; then
  err "failed to download release asset"
  err "release: $TAG"
  err "asset: $ASSET_NAME"
  exit 1
fi

if [ "$MODE" = "cli" ]; then
  chmod +x "$TMP_FILE"

  log "installing to ${BOLD}$BIN_INSTALL_PATH${RESET}"

  if [ -d /usr/local/bin ] && [ -w /usr/local/bin ]; then
    mv "$TMP_FILE" "$BIN_INSTALL_PATH"
    chmod 755 "$BIN_INSTALL_PATH"
  else
    sudo mkdir -p /usr/local/bin
    sudo mv "$TMP_FILE" "$BIN_INSTALL_PATH"
    sudo chmod 755 "$BIN_INSTALL_PATH"
  fi

  ok "installed command-line executable successfully"
  printf "\n"

  "$BIN_INSTALL_PATH"
else
  EXTRACT_PATH="$TMP_DIR/extracted"

  mkdir -p "$EXTRACT_PATH"
  unzip -q "$TMP_FILE" -d "$EXTRACT_PATH"

  if [ ! -d "$EXTRACT_PATH/include/libapfs" ]; then
    err "downloaded package does not contain include/libapfs"
    exit 1
  fi

  if [ ! -f "$EXTRACT_PATH/libapfs.a" ]; then
    err "downloaded package does not contain libapfs.a"
    exit 1
  fi

  log "installing headers to ${BOLD}$INCLUDE_INSTALL_PATH/libapfs${RESET}"
  log "installing library to ${BOLD}$LIB_INSTALL_PATH/libapfs.a${RESET}"

  if [ -d /usr/local/include ] &&
     [ -w /usr/local/include ] &&
     [ -d /usr/local/lib ] &&
     [ -w /usr/local/lib ]; then

    mkdir -p "$INCLUDE_INSTALL_PATH" "$LIB_INSTALL_PATH"

    rm -rf "$INCLUDE_INSTALL_PATH/libapfs"
    cp -R "$EXTRACT_PATH/include/libapfs" "$INCLUDE_INSTALL_PATH/"

    cp "$EXTRACT_PATH/libapfs.a" "$LIB_INSTALL_PATH/libapfs.a"
  else
    sudo mkdir -p "$INCLUDE_INSTALL_PATH" "$LIB_INSTALL_PATH"

    sudo rm -rf "$INCLUDE_INSTALL_PATH/libapfs"
    sudo cp -R "$EXTRACT_PATH/include/libapfs" "$INCLUDE_INSTALL_PATH/"

    sudo cp "$EXTRACT_PATH/libapfs.a" "$LIB_INSTALL_PATH/libapfs.a"
  fi

  ok "installed libapfs successfully"
fi