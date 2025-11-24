# Soundman Desktop App

C++ + JUCE によるリアルタイム音声解析アプリケーション

## 概要

このアプリケーションは、リアルタイムのオーディオ入力を解析し、以下の機能を提供します：
- リアルタイム波形表示
- FFTスペクトラム解析
- レベルメーター
- 各種音響測定

## 必要な環境

- **Windows**: Visual Studio 2019以上
- **macOS**: Xcode 12以上
- **Linux**: GCC 9以上 / Clang 10以上
- CMake 3.18以上
- JUCE 7.x (自動的にダウンロードされます)

## ビルド手順

### 1. ビルドディレクトリの作成

```bash
mkdir Build
cd Build
```

### 2. CMake の設定

```bash
cmake ..
```

### 3. ビルド

#### Windows (Visual Studio)
```bash
cmake --build . --config Release
```

#### macOS / Linux
```bash
cmake --build . -- -j4
```

## 実行

ビルドが成功すると、以下の場所に実行ファイルが生成されます：

### Windows
```
Build\Release\SoundmanDesktop.exe
```

### macOS
```
Build/SoundmanDesktop.app
```

### Linux
```
Build/SoundmanDesktop
```

## プロジェクト構造

```
desktop-app/
├── CMakeLists.txt          # CMake設定
├── Source/                 # ソースコード
│   ├── Main.cpp           # エントリーポイント
│   ├── Core/              # コアロジック
│   ├── DSP/               # 信号処理
│   ├── UI/                # ユーザーインターフェース
│   ├── Data/              # データ管理
│   └── Utils/             # ユーティリティ
├── Resources/             # リソースファイル
├── Tests/                 # テスト
└── Build/                 # ビルド出力
```

## 開発

### デバッグビルド

```bash
cd Build
cmake -DCMAKE_BUILD_TYPE=Debug ..
cmake --build .
```

### IDE で開く

#### Visual Studio (Windows)
```bash
cd Build
cmake .. -G "Visual Studio 16 2019"
# 生成された .sln ファイルを開く
```

#### Xcode (macOS)
```bash
cd Build
cmake .. -G "Xcode"
# 生成された .xcodeproj を開く
```

## トラブルシューティング

### JUCE が見つからない場合

CMakeLists.txt で JUCE のパスを指定してください：

```cmake
add_subdirectory(/path/to/JUCE)
```

または、FetchContent（自動ダウンロード）を有効にしてください。

### オーディオデバイスが認識されない場合

- **Windows**: ASIO ドライバーをインストール
- **Linux**: ALSA または JACK をインストール
- **macOS**: システム環境設定でマイクのアクセス許可を確認

## 次のステップ

現在は基本的な雛形のみです。以下の順序で実装を進めてください：

1. ✓ プロジェクトセットアップ（完了）
2. AudioEngine の実装（Source/Core/AudioEngine.h/cpp）
3. WaveformDisplay の実装（Source/UI/WaveformDisplay.h/cpp）
4. FFTAnalyzer の実装（Source/DSP/FFTAnalyzer.h/cpp）
5. LevelMeter の実装（Source/DSP/LevelMeter.h/cpp）

詳細は [docs/ROADMAP.md](../docs/ROADMAP.md) を参照してください。

## ライセンス

MIT License
