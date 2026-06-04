#!/bin/bash
# ============================================================
# build-docker.sh — Reproducible WWC3 binary build inside Docker
# ============================================================
# Usage:  ./build-docker.sh [version]
# Example: ./build-docker.sh 1.0.0
# Artifacts are written to /tmp/wwc3-v<version>/
# ============================================================
set -euo pipefail

VERSION="${1:-1.0.0}"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
OUTDIR="/tmp/wwc3-v$VERSION"

echo "=== WWC3 v$VERSION Reproducible Build ==="
echo "Project dir: $PROJECT_DIR"
echo "Output dir:  $OUTDIR"
echo ""

# ---- Step 1: Build inside Docker ----
echo "--- Step 1/5: Building in Docker (Ubuntu 16.04 + Qt5 static) ---"
cd "$PROJECT_DIR"

docker build -f - -t wwc3-builder . <<'DOCKERFILE'
FROM ubuntu:16.04
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y \
    build-essential libssl-dev libdb++-dev libboost-all-dev \
    libqrencode-dev libminiupnpc-dev \
    qt5-default qtbase5-dev qtbase5-dev-tools qttools5-dev-tools libqt5opengl5-dev \
    && apt-get clean && rm -rf /var/lib/apt/lists/*
WORKDIR /build
COPY . .
RUN sed -i 's/^#  Windows begin/win32 {\n#  Windows begin/; s/^#  Windows end/#  Windows end\n}/' wayawolfcoin.pro || true
RUN mkdir -p src/obj/crypto
RUN cd src && make -f makefile.unix USE_UPNP=1 STATIC=1 -j$(nproc)
RUN qmake "RELEASE=1" wayawolfcoin.pro && make -j$(nproc)
DOCKERFILE

# ---- Step 2: Extract binaries ----
echo "--- Step 2/5: Extracting binaries ---"
mkdir -p "$OUTDIR"
docker run --rm wwc3-builder cat /build/src/wayawolfcoind > "$OUTDIR/wayawolfcoind-linux-x86_64"
docker run --rm wwc3-builder cat /build/Wayawolfcoin-qt > "$OUTDIR/Wayawolfcoin-qt-linux-x86_64"
chmod +x "$OUTDIR/wayawolfcoind-linux-x86_64" "$OUTDIR/Wayawolfcoin-qt-linux-x86_64"

# ---- Step 3: Create tarballs ----
echo "--- Step 3/5: Creating tar.gz archives ---"
cd "$OUTDIR"
tar czf wayawolfcoind-linux-x86_64.tar.gz wayawolfcoind-linux-x86_64
tar czf Wayawolfcoin-qt-linux-x86_64.tar.gz Wayawolfcoin-qt-linux-x86_64

# ---- Step 4: Generate SHA256 checksums ----
echo "--- Step 4/5: Generating SHA256 checksums ---"
sha256sum wayawolfcoind-linux-x86_64.tar.gz > wayawolfcoind-linux-x86_64.tar.gz.sha256
sha256sum Wayawolfcoin-qt-linux-x86_64.tar.gz > Wayawolfcoin-qt-linux-x86_64.tar.gz.sha256
sha256sum wayawolfcoind-linux-x86_64 > wayawolfcoind-linux-x86_64.sha256
sha256sum Wayawolfcoin-qt-linux-x86_64 > Wayawolfcoin-qt-linux-x86_64.sha256
sha256sum * > SHA256SUMS
cd "$PROJECT_DIR"

# ---- Step 5: Clean up Docker ----
echo "--- Step 5/5: Cleaning up Docker ---"
docker rmi wwc3-builder 2>/dev/null || true

echo ""
echo "=== Build complete ==="
echo "Output: $OUTDIR"
echo "Artifacts are not tracked by git (outside repository)."
echo ""
echo "Files:"
ls -lh "$OUTDIR"
echo ""
echo "SHA256SUMS:"
cat "$OUTDIR/SHA256SUMS"
