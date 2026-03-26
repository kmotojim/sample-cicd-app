# Sample CI/CD Application

> **このドキュメントがすべての手順のエントリポイントです。**
> セットアップは本 README の「セットアップ手順」セクションを上から順に進めてください。
> マニフェストリポ (`sample-cicd-app-manifests`) や `third_party/README.md` は補足的なリファレンスであり、個別に読む必要はありません。

OpenShift 上で **DevSpaces** + **OpenShift Pipeline (Tekton)** + **OpenShift GitOps (ArgoCD)** を体験するためのサンプルアプリケーションです。

C++ で実装された車両ダッシュボードの REST API バックエンドと Web UI を題材に、CI/CD パイプラインの一連のフローを学習できます。

## アーキテクチャ概要

```
開発者 (DevSpaces)
  │
  │ git push
  ▼
Gitea (ソースリポ: sample-cicd-app)
  │
  │ Webhook
  ▼
Tekton CI Pipeline
  ├── git-clone
  ├── cmake build & test
  ├── コンテナイメージ build & push
  ├── マニフェストリポの image tag 更新 (develop ブランチ)
  │       │
  │       ▼
  │   Gitea (マニフェストリポ: sample-cicd-app-manifests)
  │       │
  │       │ develop ブランチ監視
  │       ▼
  │   ArgoCD → dev 環境にデプロイ
  │       │
  ├── dev 環境ヘルスチェック待機
  ├── スモークテスト (curl ヘルスチェック & API 確認)
  └── 成功時: develop → main の PR を自動作成
         │
         │ 手動マージ
         ▼
ArgoCD → prod 環境にデプロイ
```

## リポジトリ構成

本プロジェクトは **2つの Gitea リポジトリ** で構成されます:


| リポジトリ                                                                                | 内容                                      | ブランチ          |
| ------------------------------------------------------------------------------------ | --------------------------------------- | ------------- |
| `sample-cicd-app` (本リポジトリ)                                                           | C++ ソースコード                              | main          |
| `[sample-cicd-app-manifests](https://github.com/kmotojim/sample-cicd-app-manifests)` | K8s マニフェスト、ArgoCD、Tekton (CI + スモークテスト) | develop, main |


## 技術スタック

- **言語**: C++17
- **ビルド**: CMake 3.15+
- **HTTP サーバー**: cpp-httplib v0.18.3
- **JSON**: nlohmann-json v3.11.3
- **テスト**: Google Test v1.14.0
- **コンテナ**: UDI (Builder) + UBI9-minimal (Runtime) マルチステージビルド
- **CI**: OpenShift Pipeline (Tekton)
- **CD**: OpenShift GitOps (ArgoCD) + Kustomize
- **IDE**: OpenShift DevSpaces

## API エンドポイント


| メソッド | パス                                       | 説明      |
| ---- | ---------------------------------------- | ------- |
| GET  | `/health`                                | ヘルスチェック |
| GET  | `/api/vehicle/state`                     | 車両状態取得  |
| POST | `/api/vehicle/accelerate`                | 加速      |
| POST | `/api/vehicle/decelerate`                | 減速      |
| POST | `/api/vehicle/gear/{P|R|N|D}`            | ギア変更    |
| POST | `/api/vehicle/seatbelt/{true|false}`     | シートベルト  |
| POST | `/api/vehicle/engine-error/{true|false}` | エンジン異常  |
| POST | `/api/vehicle/reset`                     | リセット    |


---

# セットアップ手順

> **前提**: すべての操作は OpenShift にログイン済みの状態で行います。
> `oc login` でクラスタにログインしてから開始してください。

## 前提条件チェックリスト

以下がインストール・設定済みであることを確認してください:

```bash
# OpenShift Pipeline Operator
oc get csv -n openshift-operators | grep openshift-pipelines

# OpenShift GitOps Operator
oc get csv -n openshift-operators | grep openshift-gitops

# OpenShift DevSpaces Operator
oc get csv -n openshift-operators | grep devspaces

# Gitea が起動していること
# Gitea の URL にブラウザでアクセスできることを確認
```

### Gitea の準備

- Gitea にユーザーアカウントが作成済みであること
- **API トークン** を発行済みであること（CI パイプラインからの push や PR 作成で使用）

トークンの発行方法:

1. Gitea Web UI → 右上のユーザーアイコン → **「設定」**
2. **「アプリケーション」** タブを開く
3. トークン名（例: `cicd-pipeline`）を入力
4. **権限を選択** で以下を設定:


| カテゴリ                   | 権限            | 用途                                        |
| ---------------------- | ------------- | ----------------------------------------- |
| **リポジトリ (repository)** | **読み取りと書き込み** | CI パイプラインからマニフェストリポへの `git push`          |
| **Issue (issue)**      | **読み取りと書き込み** | スモークテストから develop → main の PR 作成・既存 PR 確認 |


1. **「トークンを生成」** をクリック
2. 表示されたトークンを控えておく（**この画面でしか確認できません**。後の Step 2, Step 5 で使用）

### 必要なコンテナイメージ

エアギャップ環境では、以下のイメージがミラーレジストリに必要です:

#### アプリケーション / パイプラインで明示的に使用するイメージ

| イメージ | 用途 |
| --- | --- |
| `registry.redhat.io/devspaces/udi-rhel9:latest` | Containerfile ビルドステージ / Tekton マニフェスト更新ステップ / `udi-cpp-dev` のベースイメージ (DevSpaces) |
| `registry.access.redhat.com/ubi9/ubi-minimal:latest` | Containerfile ランタイムステージ / Tekton ヘルスチェック・スモークテスト・PR 作成ステップ |
| `udi-cpp-dev:latest` （自前ビルド） | Tekton cmake-build-test ステップ / DevSpaces ワークスペース。`Containerfile.devspaces` から事前にビルドが必要 |

#### パイプラインで暗黙的に必要なイメージ（OpenShift Pipelines 由来）

以下のイメージはマニフェスト YAML には記載されていませんが、パイプライン実行時に OpenShift Pipelines が内部的に使用します。

| イメージ | 用途 |
| --- | --- |
| `registry.redhat.io/openshift-pipelines/pipelines-git-init-rhel9:latest` | `git-clone` Task が使用 |
| `registry.redhat.io/rhel9/buildah:latest` | `buildah` Task が使用 |
| `registry.redhat.io/openshift-pipelines/pipelines-triggers-eventlistenersink-rhel9:latest` | EventListener Pod が使用 |

> **正確なイメージの確認方法:** 上記イメージのタグ/ダイジェストは OpenShift Pipelines のバージョンに依存します。実際の環境で以下のコマンドを実行して確認してください:
>
> ```bash
> # git-clone Task のイメージ
> oc get task git-clone -n openshift-pipelines -o jsonpath='{.spec.steps[*].image}'
>
> # buildah Task のイメージ
> oc get task buildah -n openshift-pipelines -o jsonpath='{.spec.steps[*].image}'
>
> # EventListener Sink のイメージ（既存の EventListener がある場合）
> oc get deploy -l app.kubernetes.io/managed-by=EventListener -A \
>   -o jsonpath='{range .items[*]}{.spec.template.spec.containers[*].image}{"\n"}{end}'
> ```

#### イメージ不足時のトラブルシューティング

オフライン環境でパイプライン実行時にイメージが見つからない場合、以下のようなエラーが発生します:

- **PipelineRun / TaskRun が `ImagePullBackOff` で停止する:**
  `oc get pods -n <NAMESPACE>` で Pod のステータスが `ImagePullBackOff` または `ErrImagePull` となります。`oc describe pod <POD名>` の Events セクションに `Failed to pull image "..."` というメッセージが表示されるので、そこに記載されたイメージをミラーレジストリに追加してください。

- **EventListener Pod が起動しない:**
  `oc get pods -n <NAMESPACE> -l eventlistener=ci-event-listener` で Pod が `ImagePullBackOff` の場合、EventListener Sink イメージのミラーリングが必要です。

- **buildah ステップで Containerfile の FROM が失敗する:**
  TaskRun のログに `error creating build container: ... manifest unknown` や `requested access to the resource is denied` と表示される場合、`Containerfile` の `FROM` で指定しているイメージ（`udi-rhel9`, `ubi9/ubi-minimal`）がミラーレジストリに存在しないことを意味します。

上記いずれの場合も、エラーメッセージに含まれるイメージ名を確認し、ミラーレジストリへの登録を行ってください。


---

## Step 1: リポジトリの取得と依存ライブラリのダウンロード

**インターネット接続可能なマシンで**実行してください:

```bash
# 2つのリポジトリをクローン
git clone https://github.com/kmotojim/sample-cicd-app.git
git clone https://github.com/kmotojim/sample-cicd-app-manifests.git

# 依存ライブラリをダウンロード
cd sample-cicd-app
./scripts/download-deps.sh
cd ..
```

## Step 2: YAML ファイルのカスタマイズ

Gitea に push する **前に**、プレースホルダーを環境に合わせて置き換えます。

| プレースホルダー | 説明 | 例 |
|---|---|---|
| `<GITEA_HOST>` | Gitea のホスト名 | `gitea.apps.example.com` |
| `<MIRROR_REGISTRY>` | ミラーレジストリのアドレス | `registry.apps.example.com` |
| `<NAMESPACE>` | Tekton リソースの namespace | `sample-cicd-pipeline` |
| `<APP_ROUTE_HOST>` | dev 環境アプリの Route ホスト | `smart-mobility-dashboard-sample-cicd-dev.apps.example.com` |
| `<GITEA_OWNER>` | Gitea のリポジトリオーナー | `myuser` |

> **macOS の場合**: `sed -i` ではなく `sed -i ''` を使用してください（BSD sed と GNU sed の違い）。
> 以下の例は Linux (GNU sed) 向けです。

### マニフェストリポの一括置換

```bash
cd sample-cicd-app-manifests
sed -i 's|<GITEA_HOST>|gitea.apps.example.com|g' argocd/*.yaml tekton/*.yaml
sed -i 's|<MIRROR_REGISTRY>|registry.apps.example.com|g' base/kustomization.yaml tekton/ci-pipeline.yaml
sed -i 's|<NAMESPACE>|sample-cicd-pipeline|g' tekton/*.yaml
sed -i 's|<APP_ROUTE_HOST>|smart-mobility-dashboard-sample-cicd-dev.apps.example.com|g' tekton/*.yaml
sed -i 's|<GITEA_OWNER>|myuser|g' argocd/*.yaml tekton/*.yaml

# 変更をコミット
git add -A
git commit -m "Customize placeholders for environment"
cd ..
```

### ソースリポの Containerfile 置換

```bash
cd sample-cicd-app
sed -i 's|<MIRROR_REGISTRY>|registry.example.com|g' Containerfile

# 依存ライブラリとあわせてコミット
git add -A
git commit -m "Add vendored dependencies and customize Containerfile"
cd ..
```

## Step 3: Gitea にリポジトリを作成・push

Gitea の Web UI から以下の2つの **空リポジトリ** を作成します（README の初期化はしない）:

1. **`sample-cicd-app`** (ソースコード用)
2. **`sample-cicd-app-manifests`** (マニフェスト用)

> **注意**: push 時に認証を求められます。Gitea のユーザー名と、前提条件で発行した **API トークン（パスワードの代わり）** を使用してください。

```bash
# ソースコードリポジトリを push
cd sample-cicd-app
git remote set-url origin https://<GITEA_HOST>/<GITEA_OWNER>/sample-cicd-app.git
git push -u origin main
cd ..

# マニフェストリポジトリを push
cd sample-cicd-app-manifests
git remote set-url origin https://<GITEA_HOST>/<GITEA_OWNER>/sample-cicd-app-manifests.git
git push -u origin main

# develop ブランチも作成
git checkout -b develop
git push -u origin develop
cd ..
```

## Step 4: DevSpaces で開発環境を起動

1. OpenShift Web Console から **DevSpaces** を開きます
2. **「Create Workspace」** をクリック
3. **Git Repo URL** に `https://<GITEA_HOST>/<GITEA_OWNER>/sample-cicd-app.git` を入力
4. ワークスペースが起動したら、ターミナルを開きます

### DevSpaces 内でのビルド・テスト

```bash
# ビルド
mkdir -p build && cd build
cmake ..
cmake --build . --parallel $(nproc)

# テスト実行
ctest --output-on-failure

# アプリケーション起動
cd ..
./build/smart-mobility-backend
```

ブラウザで DevSpaces のエンドポイント URL にアクセスすると、ダッシュボード UI が表示されます。

## Step 5: Tekton パイプラインのデプロイ

> **注意**: Step 5〜7 は **ローカルマシン**（Step 1 で両リポジトリをクローンしたディレクトリ）で実行します。DevSpaces 内ではありません。

CI パイプライン（スモークテスト・PR 作成含む）はすべてマニフェストリポの `tekton/` にあります。

```bash
# namespace 作成
oc new-project sample-cicd-pipeline

# Gitea 認証情報の Secret を作成（CI パイプラインの image tag 更新 + スモークテストの PR 作成で使用）
oc create secret generic gitea-credentials \
  --from-literal=token=<GITEA_API_TOKEN> \
  --from-file=.gitconfig=<(echo -e "[credential]\n\thelper = store") \
  --from-file=.git-credentials=<(echo "https://<GITEA_USERNAME>:<GITEA_PASSWORD>@<GITEA_HOST>") \
  -n sample-cicd-pipeline

# CI パイプラインをデプロイ (sample-cicd-app-manifests リポジトリ内)
oc apply -f sample-cicd-app-manifests/tekton/ci-pipeline.yaml -n sample-cicd-pipeline
oc apply -f sample-cicd-app-manifests/tekton/ci-trigger-binding.yaml -n sample-cicd-pipeline
oc apply -f sample-cicd-app-manifests/tekton/ci-trigger-template.yaml -n sample-cicd-pipeline
oc apply -f sample-cicd-app-manifests/tekton/ci-event-listener.yaml -n sample-cicd-pipeline
```

### Gitea Webhook の設定

1. Gitea の `sample-cicd-app` リポジトリの **Settings → Webhooks** を開きます
2. **「Add Webhook」→「Gitea」** を選択
3. 以下を設定:
  - **Target URL**: `oc get route ci-event-listener -n sample-cicd-pipeline -o jsonpath='{.status.ingress[0].host}'` の結果を `https://` 付きで入力
  - **Secret**: 空欄のまま（EventListener は CEL フィルターのみで検証）
  - **Trigger On**: Push Events
4. **「Add Webhook」** をクリック

## Step 6: ArgoCD Application のデプロイ

```bash
# ArgoCD が Gitea リポジトリにアクセスできるよう設定
# ArgoCD Web UI の Settings → Repositories から追加するか、以下を実行:
oc apply -f - <<EOF
apiVersion: v1
kind: Secret
metadata:
  name: sample-cicd-manifests-repo
  namespace: openshift-gitops
  labels:
    argocd.argoproj.io/secret-type: repository
stringData:
  type: git
  url: https://<GITEA_HOST>/<GITEA_OWNER>/sample-cicd-app-manifests.git
  username: <GITEA_USERNAME>
  password: <GITEA_PASSWORD>
EOF

# AppProject と Application をデプロイ (sample-cicd-app-manifests リポジトリ内)
oc apply -f sample-cicd-app-manifests/argocd/appproject.yaml
oc apply -f sample-cicd-app-manifests/argocd/application-dev.yaml
oc apply -f sample-cicd-app-manifests/argocd/application-prod.yaml
```

ArgoCD Web Console (`oc get route openshift-gitops-server -n openshift-gitops`) にアクセスして、Application が作成されていることを確認します。

## Step 7: 動作確認

### CI/CD フロー全体のテスト

1. **DevSpaces でコードを変更** (例: `src/main.cpp` のバージョン文字列を変更)
2. **Gitea に push**:
  ```bash
   git add -A && git commit -m "Update version" && git push
  ```
3. **OpenShift Web Console → Pipelines** で CI パイプラインの実行を確認
4. CI パイプラインが `update-manifests` まで進むと、ArgoCD が dev 環境にデプロイ
5. CI パイプラインの `wait-for-deploy` → `smoke-test` → `create-pr` ステップが順に完了
6. **Gitea** でマニフェストリポに **develop → main の PR** が自動作成されていることを確認
7. PR を手動でマージ
8. ArgoCD が prod 環境にデプロイすることを確認

---

# トラブルシューティング

## パイプラインが起動しない

```bash
# EventListener の Pod が起動しているか確認
oc get pods -l eventlistener=ci-event-listener -n sample-cicd-pipeline

# EventListener の Route URL を確認
oc get route ci-event-listener -n sample-cicd-pipeline

# EventListener のログを確認
oc logs -l eventlistener=ci-event-listener -n sample-cicd-pipeline
```

## ビルドが失敗する

```bash
# PipelineRun の状態を確認
oc get pipelinerun -n sample-cicd-pipeline

# 失敗した PipelineRun のログを確認
oc logs -l tekton.dev/pipelineRun=<PIPELINERUN_NAME> -n sample-cicd-pipeline --all-containers
```

## イメージ pull に失敗する

エアギャップ環境ではイメージがミラーレジストリに存在する必要があります:

```bash
# ミラーレジストリにイメージがあるか確認
oc debug node/<NODE_NAME> -- chroot /host podman pull <MIRROR_REGISTRY>/ubi9/ubi:latest
```

## ArgoCD が sync しない

```bash
# Application の状態を確認
oc get application -n openshift-gitops

# ArgoCD がリポジトリにアクセスできるか確認
# ArgoCD Web UI → Settings → Repositories で接続状態を確認
```

## DevSpaces が起動しない

```bash
# DevWorkspace の状態を確認
oc get devworkspace -A

# DevSpaces UDI イメージが pull できるか確認
oc get events -n <DEVSPACES_NAMESPACE> --sort-by='.lastTimestamp'
```

---

# ローカル開発 (DevSpaces 外)

```bash
# 依存ライブラリのダウンロード
./scripts/download-deps.sh

# ビルド
mkdir -p build && cd build
cmake ..
cmake --build . --parallel $(nproc)

# テスト
ctest --output-on-failure

# 実行
cd ..
./build/smart-mobility-backend
# ブラウザで http://localhost:8080 にアクセス
```

# コンテナビルド

> **注意**: Containerfile 内の `<MIRROR_REGISTRY>` を実際のレジストリに置換済みであることを確認してください（Step 2 参照）。

```bash
podman build -t smart-mobility-backend:latest -f Containerfile .
podman run -p 8080:8080 smart-mobility-backend:latest
```

