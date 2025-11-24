# Soundman

サウンドリアルタイム解析ツール＆データ管理システム

## 概要

Soundmanは、リアルタイム音声解析機能とサウンドデータ管理機能を統合したハイブリッドアプリケーションです。プロフェッショナルな音響解析と効率的なサウンドライブラリ管理を実現します。

### 主な機能

**デスクトップアプリ（リアルタイム解析）**
- リアルタイム波形表示
- FFTスペクトラム解析
- レベルメーター
- オーディオ録音・再生
- 各種音響測定

**Webアプリ（データ管理）**
- サウンドファイルライブラリ管理
- メタデータ編集
- 高度な検索・フィルタリング
- プレビュー再生
- プロジェクト管理

## システム構成

```
┌─────────────────────────────────────────────────────────────┐
│                     Soundman System                          │
├─────────────────────────┬───────────────────────────────────┤
│  Desktop App            │  Web App                          │
│  (C++ + JUCE)          │  (Vue.js + TypeScript)            │
└─────────────┬───────────┴──────────┬────────────────────────┘
              │                      │
              └──────────┬───────────┘
                         │
              ┌──────────▼──────────┐
              │  Shared Data Layer  │
              │  (JSON + Files)     │
              └─────────────────────┘
```

## 技術スタック

### デスクトップアプリ
- **言語**: C++17以上
- **フレームワーク**: JUCE 7.x
- **ビルドシステム**: CMake 3.18以上

### Webアプリ
- **フレームワーク**: Vue.js 3.x (Composition API)
- **言語**: TypeScript 5.x
- **ビルドツール**: Vite

## プロジェクト構造

```
Soundman/
├── docs/                    # ドキュメント
│   ├── FEATURES.md          # 機能リスト
│   ├── ARCHITECTURE.md      # 技術仕様書
│   ├── PROJECT_STRUCTURE.md # プロジェクト構成
│   └── ROADMAP.md           # 実装ロードマップ
├── desktop-app/             # デスクトップアプリ
├── web-app/                 # Webアプリ
├── shared/                  # 共有リソース
├── data/                    # ユーザーデータ
└── README.md
```

## セットアップ

### 必要な環境

#### デスクトップアプリ
- **Windows**: Visual Studio 2019以上
- **macOS**: Xcode 12以上
- **Linux**: GCC 9以上 / Clang 10以上
- CMake 3.18以上
- JUCE 7.x

#### Webアプリ
- Node.js 18以上
- npm 9以上 または pnpm 8以上

### インストール手順

#### 1. リポジトリのクローン

```bash
git clone https://github.com/yourusername/soundman.git
cd soundman
```

#### 2. デスクトップアプリのビルド

```bash
cd desktop-app
mkdir Build && cd Build
cmake ..
cmake --build .
```

##### Windows
```bash
cmake --build . --config Release
```

##### macOS / Linux
```bash
cmake --build . -- -j4
```

#### 3. Webアプリのセットアップ

```bash
cd web-app
npm install
npm run dev
```

### 開発モードでの実行

#### デスクトップアプリ

```bash
cd desktop-app/Build
./SoundmanDesktop  # macOS/Linux
# または
.\Debug\SoundmanDesktop.exe  # Windows
```

#### Webアプリ

```bash
cd web-app
npm run dev
```

ブラウザで http://localhost:3000 を開きます。

## 使い方

### 基本的なワークフロー

1. **デスクトップアプリ**でオーディオを解析
   - マイク入力またはファイルを読み込み
   - リアルタイムで波形・スペクトラムを確認
   - 解析結果をエクスポート

2. **Webアプリ**でデータを管理
   - ライブラリにファイルを追加
   - メタデータを編集（BPM、キー、タグ等）
   - 検索・フィルタで目的のサウンドを発見
   - プロジェクトで整理

## ドキュメント

詳細なドキュメントは `docs/` ディレクトリにあります：

- [機能リスト](FEATURES.md) - 実装予定の全機能
- [技術仕様書](ARCHITECTURE.md) - システムアーキテクチャ
- [プロジェクト構成](PROJECT_STRUCTURE.md) - コード構造の詳細
- [実装ロードマップ](ROADMAP.md) - 開発計画

## 開発状況

現在のバージョン: **Alpha 0.1 (Phase 1 開発中)**

- [x] Phase 0: プロジェクトセットアップ
- [ ] Phase 1: MVP実装
- [ ] Phase 2: 機能拡張
- [ ] Phase 3: 高度な機能
- [ ] Phase 4: 最適化・品質向上
- [ ] Phase 5: リリース準備

詳細は [ROADMAP.md](ROADMAP.md) を参照してください。

## テスト

### デスクトップアプリ

```bash
cd desktop-app/Build
ctest
```

### Webアプリ

```bash
cd web-app
npm run test:unit      # ユニットテスト
npm run test:e2e       # E2Eテスト
```

## コントリビューション

コントリビューションを歓迎します！以下の手順でお願いします：

1. このリポジトリをフォーク
2. 機能ブランチを作成 (`git checkout -b feature/amazing-feature`)
3. 変更をコミット (`git commit -m 'Add some amazing feature'`)
4. ブランチにプッシュ (`git push origin feature/amazing-feature`)
5. プルリクエストを作成

## ライセンス

このプロジェクトは MIT ライセンスの下で公開されています。詳細は [LICENSE](LICENSE) ファイルを参照してください。

## 連絡先

プロジェクトリンク: [https://github.com/yourusername/soundman](https://github.com/yourusername/soundman)

## 謝辞

- [JUCE](https://juce.com/) - オーディオフレームワーク
- [Vue.js](https://vuejs.org/) - プログレッシブJavaScriptフレームワーク
- [Vite](https://vitejs.dev/) - 次世代フロントエンドツール
