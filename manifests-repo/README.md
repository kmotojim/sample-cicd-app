# Sample CI/CD App - Kubernetes Manifests

Sample CI/CD Application の Kubernetes マニフェストリポジトリです。
ArgoCD による GitOps パターンで、dev / prod 環境へのデプロイを管理します。

## ディレクトリ構成

```
manifests-repo/
├── argocd/
│   ├── appproject.yaml                # ArgoCD プロジェクト定義
│   ├── application-dev.yaml           # dev 環境 (develop ブランチ)
│   └── application-prod.yaml          # prod 環境 (main ブランチ)
├── base/
│   ├── kustomization.yaml             # Kustomize ベース設定
│   ├── namespace.yaml
│   ├── deployment.yaml
│   ├── service.yaml
│   └── route.yaml
├── overlays/
│   ├── dev/                           # dev 環境: replica=1
│   │   ├── kustomization.yaml
│   │   └── namespace.yaml
│   └── prod/                          # prod 環境: replica=2, リソース増
│       ├── kustomization.yaml
│       └── namespace.yaml
└── tekton/
    ├── ci-pipeline.yaml               # CI パイプライン (build/test/push)
    ├── ci-event-listener.yaml         # Gitea push webhook 受信
    ├── ci-trigger-binding.yaml        # Gitea ペイロードマッピング
    ├── ci-trigger-template.yaml       # CI PipelineRun 生成
    ├── smoke-test-pipeline.yaml       # スモークテスト (curl + PR 作成)
    ├── smoke-test-event-listener.yaml # ArgoCD sync 通知受信
    ├── smoke-test-trigger-binding.yaml
    └── smoke-test-trigger-template.yaml
```

## ブランチ戦略

| ブランチ | 環境 | ArgoCD Application | Kustomize パス |
|---------|------|-------------------|---------------|
| `develop` | dev | `sample-cicd-app-dev` | `overlays/dev` |
| `main` | prod | `sample-cicd-app-prod` | `overlays/prod` |

## 環境差分

| 設定 | dev | prod |
|------|-----|------|
| Namespace | `sample-cicd-dev` | `sample-cicd-prod` |
| Replicas | 1 | 2 |
| Memory requests | 128Mi | 256Mi |
| Memory limits | 256Mi | 512Mi |
| CPU requests | 10m | 200m |
| CPU limits | 500m | 1000m |

## CI/CD フロー

1. CI パイプラインがイメージをビルド・push
2. CI パイプラインが `base/kustomization.yaml` の image tag を更新して develop に push
3. ArgoCD が develop ブランチの変更を検知 → dev 環境に自動デプロイ
4. スモークテストパイプラインが実行 → 成功時に develop → main の PR を自動作成
5. PR を手動マージ → ArgoCD が main ブランチの変更を検知 → prod 環境にデプロイ

## 手動デプロイ (Kustomize)

```bash
# dev 環境
oc apply -k overlays/dev/

# prod 環境
oc apply -k overlays/prod/
```

## セットアップ手順

> **セットアップの全手順はソースリポジトリの [README.md](../README.md) に集約されています。**
> 本ドキュメントはマニフェストリポ単体のリファレンスです。個別に読む必要はありません。
