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
  └── マニフェストリポの image tag 更新 (develop ブランチ)
         │
         ▼
Gitea (マニフェストリポ: sample-cicd-app-manifests)
  │
  │ develop ブランチ監視
  ▼
ArgoCD → dev 環境にデプロイ
  │
  │ sync 成功通知
  ▼
Tekton Smoke Test Pipeline
  ├── curl ヘルスチェック & API 確認
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
- **コンテナ**: UBI9 ベース (マルチステージビルド)
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


| イメージ                                                 | 用途                             |
| ---------------------------------------------------- | ------------------------------ |
| `registry.access.redhat.com/ubi9/ubi:latest`         | ビルドステージ                        |
| `registry.access.redhat.com/ubi9/ubi-minimal:latest` | ランタイムステージ                      |
| `registry.redhat.io/devspaces/udi-rhel9:latest`      | DevSpaces UDI ベース (C++ ツール追加用) |


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

```

## Step 2: Gitea にリポジトリを作成

Gitea の Web UI から以下の2つの **空リポジトリ** を作成します（README の初期化はしない）:

1. `**sample-cicd-app`** (ソースコード用)
2. `**sample-cicd-app-manifests**` (マニフェスト用)

> **注意**: push 時に認証を求められます。Gitea のユーザー名と、前提条件で発行した **API トークン（パスワードの代わり）** を使用してください。

```bash
# ソースコードリポジトリを push
cd sample-cicd-app
git remote set-url origin https://<GITEA_HOST>/<OWNER>/sample-cicd-app.git
git push -u origin main
cd ..

# マニフェストリポジトリを push
cd sample-cicd-app-manifests
git remote set-url origin https://<GITEA_HOST>/<OWNER>/sample-cicd-app-manifests.git
git push -u origin main

# develop ブランチも作成
git checkout -b develop
git push -u origin develop
cd ..
```

## Step 3: DevSpaces で開発環境を起動

1. OpenShift Web Console から **DevSpaces** を開きます
2. **「Create Workspace」** をクリック
3. **Git Repo URL** に `https://<GITEA_HOST>/<OWNER>/sample-cicd-app.git` を入力
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

## Step 4: YAML ファイルのカスタマイズ

デプロイ前に、プレースホルダーを実際の値に置き換えます:


| プレースホルダー            | 説明                     | 例                                                           |
| ------------------- | ---------------------- | ----------------------------------------------------------- |
| `<GITEA_HOST>`      | Gitea のホスト名            | `gitea.apps.example.com`                                    |
| `<MIRROR_REGISTRY>` | ミラーレジストリのアドレス          | `registry.apps.example.com`                                 |
| `<NAMESPACE>`       | Tekton リソースの namespace | `sample-cicd-pipeline`                                      |
| `<APP_ROUTE_HOST>`  | dev 環境アプリの Route ホスト   | `smart-mobility-dashboard-sample-cicd-dev.apps.example.com` |
| `<GITEA_OWNER>`     | Gitea のリポジトリオーナー       | `myuser`                                                    |


```bash
# マニフェストリポの一括置換
cd sample-cicd-app-manifests
sed -i 's|<GITEA_HOST>|gitea.apps.example.com|g' argocd/*.yaml tekton/*.yaml
sed -i 's|<MIRROR_REGISTRY>|registry.apps.example.com|g' base/kustomization.yaml tekton/ci-pipeline.yaml
sed -i 's|<NAMESPACE>|sample-cicd-pipeline|g' tekton/*.yaml
sed -i 's|<APP_ROUTE_HOST>|smart-mobility-dashboard-sample-cicd-dev.apps.example.com|g' tekton/*.yaml
sed -i 's|<GITEA_OWNER>|myuser|g' tekton/*.yaml
cd ..
```

ソースリポの Containerfile 内のイメージパスも必要に応じて変更してください:

```bash
cd sample-cicd-app
sed -i 's|registry.access.redhat.com|<MIRROR_REGISTRY>|g' Containerfile
cd ..
```

## Step 5: Tekton パイプラインのデプロイ

CI パイプラインとスモークテストパイプラインはすべてマニフェストリポの `tekton/` にあります。

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

# スモークテストパイプラインをデプロイ
oc apply -f sample-cicd-app-manifests/tekton/smoke-test-pipeline.yaml -n sample-cicd-pipeline
oc apply -f sample-cicd-app-manifests/tekton/smoke-test-trigger-binding.yaml -n sample-cicd-pipeline
oc apply -f sample-cicd-app-manifests/tekton/smoke-test-trigger-template.yaml -n sample-cicd-pipeline
oc apply -f sample-cicd-app-manifests/tekton/smoke-test-event-listener.yaml -n sample-cicd-pipeline
```

### Gitea Webhook の設定

1. Gitea の `sample-cicd-app` リポジトリの **Settings → Webhooks** を開きます
2. **「Add Webhook」→「Gitea」** を選択
3. 以下を設定:
  - **Target URL**: `oc get route ci-event-listener -n sample-cicd-pipeline -o jsonpath='{.status.ingress[0].host}'` の結果を `https://` 付きで入力
  - **Secret**: event-listener.yaml で設定したシークレット値
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
  url: https://<GITEA_HOST>/<OWNER>/sample-cicd-app-manifests.git
  username: <GITEA_USERNAME>
  password: <GITEA_PASSWORD>
EOF

# AppProject と Application をデプロイ (sample-cicd-app-manifests リポジトリ内)
oc apply -f sample-cicd-app-manifests/argocd/appproject.yaml
oc apply -f sample-cicd-app-manifests/argocd/application-dev.yaml
oc apply -f sample-cicd-app-manifests/argocd/application-prod.yaml
```

ArgoCD Web Console (`oc get route openshift-gitops-server -n openshift-gitops`) にアクセスして、Application が作成されていることを確認します。

## Step 7: ArgoCD Notifications の設定

ArgoCD が sync 成功時にスモークテスト EventListener に通知するよう設定します:

```bash
# スモークテスト EventListener の URL を取得
SMOKE_URL=$(oc get route smoke-test-event-listener -n sample-cicd-pipeline -o jsonpath='https://{.status.ingress[0].host}')

# ArgoCD Notifications の ConfigMap を更新
oc patch configmap argocd-notifications-cm -n openshift-gitops --type merge -p "{
  \"data\": {
    \"service.webhook.smoke-test-webhook\": \"url: ${SMOKE_URL}\nheaders:\\n- name: Content-Type\\n  value: application/json\",
    \"trigger.on-sync-succeeded\": \"- when: app.status.operationState.phase in ['Succeeded']\\n  send: [app-sync-succeeded]\",
    \"template.app-sync-succeeded\": \"webhook:\\n  smoke-test-webhook:\\n    method: POST\\n    body: |\\n      {\\\"revision\\\": \\\"{{.app.status.sync.revision}}\\\"}\"
  }
}"
```

## Step 8: 動作確認

### CI/CD フロー全体のテスト

1. **DevSpaces でコードを変更** (例: `src/main.cpp` のバージョン文字列を変更)
2. **Gitea に push**:
  ```bash
   git add -A && git commit -m "Update version" && git push
  ```
3. **OpenShift Web Console → Pipelines** で CI パイプラインの実行を確認
4. パイプライン完了後、**ArgoCD Web Console** で dev 環境へのデプロイを確認
5. dev 環境の Route URL にアクセスしてアプリの動作を確認
6. スモークテストパイプラインが自動実行されることを確認
7. **Gitea** でマニフェストリポに **develop → main の PR** が自動作成されていることを確認
8. PR を手動でマージ
9. ArgoCD が prod 環境にデプロイすることを確認

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

```bash
podman build -t smart-mobility-backend:latest -f Containerfile .
podman run -p 8080:8080 smart-mobility-backend:latest
```

