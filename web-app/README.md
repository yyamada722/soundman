# Soundman Web App

Vue.js + TypeScript による音声ファイルライブラリ管理アプリケーション

## 概要

ブラウザベースのサウンドライブラリ管理ツールです。以下の機能を提供します：
- ファイル一覧表示（グリッド/リスト）
- メタデータ編集（BPM、キー、タグ等）
- 検索・フィルタリング
- プレビュー再生
- プロジェクト管理

## 必要な環境

- Node.js 18以上
- npm 9以上 または pnpm 8以上
- モダンブラウザ（Chrome 90+, Firefox 88+, Safari 14+, Edge 90+）

## セットアップ

### 1. 依存関係のインストール

```bash
npm install
```

または

```bash
pnpm install
```

### 2. 開発サーバーの起動

```bash
npm run dev
```

ブラウザで http://localhost:3000 が自動的に開きます。

## スクリプト

```bash
# 開発サーバー起動
npm run dev

# 本番ビルド
npm run build

# ビルドのプレビュー
npm run preview

# ユニットテスト
npm run test:unit

# E2Eテスト
npm run test:e2e

# Lintチェック・修正
npm run lint

# コード整形
npm run format
```

## プロジェクト構造

```
web-app/
├── public/                 # 静的ファイル
├── src/
│   ├── assets/            # アセット（CSS、画像等）
│   ├── components/        # Vueコンポーネント
│   │   ├── common/       # 共通コンポーネント
│   │   ├── library/      # ライブラリ関連
│   │   ├── player/       # プレーヤー関連
│   │   └── metadata/     # メタデータ関連
│   ├── views/             # ページビュー
│   ├── stores/            # Piniaストア
│   ├── composables/       # Composition API
│   ├── services/          # サービス層
│   ├── types/             # TypeScript型定義
│   ├── utils/             # ユーティリティ
│   ├── router/            # Vue Router
│   ├── App.vue            # ルートコンポーネント
│   └── main.ts            # エントリーポイント
├── index.html
├── vite.config.ts
├── tsconfig.json
└── package.json
```

## 開発

### コンポーネントの作成

```bash
# 例：新しいコンポーネントを作成
touch src/components/library/FileCard.vue
```

### Storeの作成

Piniaを使用した状態管理：

```typescript
// src/stores/library.ts
import { defineStore } from 'pinia'

export const useLibraryStore = defineStore('library', {
  state: () => ({
    files: [],
  }),
  actions: {
    async loadLibrary() {
      // Implementation
    },
  },
})
```

### 型定義の作成

```typescript
// src/types/audio.ts
export interface AudioFile {
  id: string
  path: string
  name: string
  // ...
}
```

## データ共有

このWebアプリは、デスクトップアプリと`../data/`ディレクトリを共有してデータをやり取りします。

```
../data/
├── library.json          # ライブラリメタデータ
├── projects/            # プロジェクトファイル
└── cache/               # キャッシュデータ
```

## ビルド

本番用ビルドを作成：

```bash
npm run build
```

ビルド成果物は `dist/` ディレクトリに生成されます。

## 次のステップ

現在は基本的な雛形のみです。以下の順序で実装を進めてください：

1. ✓ プロジェクトセットアップ（完了）
2. 型定義の作成（`src/types/`）
3. ストアの実装（`src/stores/library.ts`）
4. サービス層の実装（`src/services/`）
5. UIコンポーネントの実装（`src/components/`）
6. ビューの実装（`src/views/LibraryView.vue`等）

詳細は [docs/ROADMAP.md](../docs/ROADMAP.md) を参照してください。

## トラブルシューティング

### ポートが既に使用されている場合

`vite.config.ts` でポート番号を変更：

```typescript
export default defineConfig({
  server: {
    port: 3001, // 別のポート番号に変更
  },
})
```

### 型エラーが発生する場合

```bash
npm run build
```

で型チェックを実行して、エラーを確認してください。

## ライセンス

MIT License
