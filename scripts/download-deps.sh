#!/bin/bash
# =============================================================================
# third_party/ に依存ライブラリをダウンロードするスクリプト
#
# エアギャップ環境では、インターネット接続可能なマシンでこのスクリプトを実行し、
# third_party/ ディレクトリごとリポジトリにコミットしてください。
# =============================================================================
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
THIRD_PARTY_DIR="$SCRIPT_DIR/../third_party"

echo "=== Downloading dependencies to $THIRD_PARTY_DIR ==="

# cpp-httplib v0.18.3
if [ ! -d "$THIRD_PARTY_DIR/cpp-httplib" ]; then
    echo "[1/3] Downloading cpp-httplib v0.18.3..."
    git clone --depth 1 --branch v0.18.3 \
        https://github.com/yhirose/cpp-httplib.git \
        "$THIRD_PARTY_DIR/cpp-httplib"
    rm -rf "$THIRD_PARTY_DIR/cpp-httplib/.git"
else
    echo "[1/3] cpp-httplib already exists, skipping."
fi

# nlohmann-json v3.11.3
if [ ! -d "$THIRD_PARTY_DIR/nlohmann-json" ]; then
    echo "[2/3] Downloading nlohmann-json v3.11.3..."
    git clone --depth 1 --branch v3.11.3 \
        https://github.com/nlohmann/json.git \
        "$THIRD_PARTY_DIR/nlohmann-json"
    rm -rf "$THIRD_PARTY_DIR/nlohmann-json/.git"
else
    echo "[2/3] nlohmann-json already exists, skipping."
fi

# Google Test v1.14.0
if [ ! -d "$THIRD_PARTY_DIR/googletest" ]; then
    echo "[3/3] Downloading Google Test v1.14.0..."
    git clone --depth 1 --branch v1.14.0 \
        https://github.com/google/googletest.git \
        "$THIRD_PARTY_DIR/googletest"
    rm -rf "$THIRD_PARTY_DIR/googletest/.git"
else
    echo "[3/3] googletest already exists, skipping."
fi

echo ""
echo "=== Done! ==="
echo "Dependencies are ready in $THIRD_PARTY_DIR"
echo ""
echo "For air-gapped environments:"
echo "  git add third_party/"
echo "  git commit -m 'Add vendored dependencies'"
echo "  git push"
