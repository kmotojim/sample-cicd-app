# Third-party Dependencies

> **通常はこのドキュメントを直接読む必要はありません。**
> セットアップ手順はソースリポジトリの [README.md](../README.md) の Step 1 を参照してください。

このディレクトリには、エアギャップ（インターネット非接続）環境でのビルドに必要な
サードパーティライブラリを格納します。

## 必要なライブラリ

| ライブラリ | バージョン | 用途 |
|-----------|-----------|------|
| [cpp-httplib](https://github.com/yhirose/cpp-httplib) | v0.18.3 | HTTP サーバー |
| [nlohmann-json](https://github.com/nlohmann/json) | v3.11.3 | JSON パーサー |
| [Google Test](https://github.com/google/googletest) | v1.14.0 | テストフレームワーク |

## ダウンロード手順

**インターネット接続可能なマシンで**以下を実行してください:

```bash
# スクリプトを使用（推奨）
./scripts/download-deps.sh

# または手動でダウンロード
git clone --depth 1 --branch v0.18.3 https://github.com/yhirose/cpp-httplib.git third_party/cpp-httplib
git clone --depth 1 --branch v3.11.3 https://github.com/nlohmann/json.git third_party/nlohmann-json
git clone --depth 1 --branch v1.14.0 https://github.com/google/googletest.git third_party/googletest

# .git ディレクトリを削除（サイズ削減）
rm -rf third_party/cpp-httplib/.git
rm -rf third_party/nlohmann-json/.git
rm -rf third_party/googletest/.git
```

ダウンロード後、リポジトリにコミットしてください:

```bash
git add third_party/
git commit -m "Add vendored third-party dependencies"
git push
```
