# プロジェクト構成

## ルートディレクトリ構造

```
Soundman/
├── docs/                       # ドキュメント
│   ├── FEATURES.md
│   ├── ARCHITECTURE.md
│   ├── PROJECT_STRUCTURE.md
│   └── ROADMAP.md
├── desktop-app/                # C++ + JUCE デスクトップアプリ
│   ├── CMakeLists.txt
│   ├── Source/
│   ├── Resources/
│   └── Build/
├── web-app/                    # Vue.js + TypeScript Webアプリ
│   ├── package.json
│   ├── vite.config.ts
│   ├── src/
│   └── public/
├── shared/                     # 共有リソース
│   ├── schemas/               # JSONスキーマ定義
│   └── docs/                  # 共通ドキュメント
├── data/                       # ユーザーデータ（.gitignore）
│   ├── library.json
│   ├── projects/
│   └── cache/
└── README.md
```

---

## デスクトップアプリ（desktop-app/）

### 推奨ディレクトリ構造

```
desktop-app/
├── CMakeLists.txt              # CMake設定ファイル
├── README.md
├── .gitignore
│
├── Source/                     # ソースコード
│   ├── Main.cpp               # エントリーポイント
│   │
│   ├── Core/                  # コアロジック
│   │   ├── AudioEngine.h
│   │   ├── AudioEngine.cpp
│   │   ├── AudioDeviceManager.h
│   │   ├── AudioDeviceManager.cpp
│   │   └── FileManager.h
│   │
│   ├── DSP/                   # デジタル信号処理
│   │   ├── FFTAnalyzer.h
│   │   ├── FFTAnalyzer.cpp
│   │   ├── LevelMeter.h
│   │   ├── LevelMeter.cpp
│   │   ├── Filters.h
│   │   └── Filters.cpp
│   │
│   ├── UI/                    # ユーザーインターフェース
│   │   ├── MainComponent.h
│   │   ├── MainComponent.cpp
│   │   ├── WaveformDisplay.h
│   │   ├── WaveformDisplay.cpp
│   │   ├── SpectrumDisplay.h
│   │   ├── SpectrumDisplay.cpp
│   │   ├── LevelMeterComponent.h
│   │   ├── LevelMeterComponent.cpp
│   │   └── SettingsPanel.h
│   │
│   ├── Data/                  # データ管理
│   │   ├── DataManager.h
│   │   ├── DataManager.cpp
│   │   ├── MetadataWriter.h
│   │   └── MetadataWriter.cpp
│   │
│   └── Utils/                 # ユーティリティ
│       ├── Logger.h
│       ├── Logger.cpp
│       ├── Config.h
│       └── Config.cpp
│
├── Resources/                  # リソースファイル
│   ├── Icons/
│   ├── Fonts/
│   └── Images/
│
├── Build/                      # ビルド出力（.gitignore）
│   ├── Debug/
│   └── Release/
│
├── Tests/                      # テストコード
│   ├── CMakeLists.txt
│   ├── AudioEngineTests.cpp
│   └── FFTAnalyzerTests.cpp
│
└── Docs/                       # アプリ固有のドキュメント
    ├── API.md
    └── Development.md
```

### 主要クラス設計

#### AudioEngine
```cpp
// Source/Core/AudioEngine.h
class AudioEngine : public juce::AudioIODeviceCallback
{
public:
    AudioEngine();
    ~AudioEngine();

    void prepare(double sampleRate, int blockSize);
    void start();
    void stop();

    // AudioIODeviceCallback
    void audioDeviceIOCallback(
        const float** inputChannelData,
        int numInputChannels,
        float** outputChannelData,
        int numOutputChannels,
        int numSamples) override;

    void audioDeviceAboutToStart(juce::AudioIODevice* device) override;
    void audioDeviceStopped() override;

private:
    juce::AudioDeviceManager deviceManager;
    FFTAnalyzer fftAnalyzer;
    LevelMeter levelMeter;

    double currentSampleRate = 44100.0;
    int currentBlockSize = 512;
};
```

#### FFTAnalyzer
```cpp
// Source/DSP/FFTAnalyzer.h
class FFTAnalyzer
{
public:
    FFTAnalyzer(int fftOrder = 11); // 2^11 = 2048

    void processBlock(const float* inputData, int numSamples);
    void getFrequencyData(std::vector<float>& magnitudes);

    void setFFTOrder(int order);
    void setWindowType(WindowType type);

private:
    juce::dsp::FFT fft;
    juce::dsp::WindowingFunction<float> window;

    std::array<float, 4096> fftData;
    std::vector<float> frequencyBins;

    int fftSize = 2048;
    int fftOrder = 11;
};
```

### CMakeLists.txt 例

```cmake
cmake_minimum_required(VERSION 3.18)

project(SoundmanDesktop VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# JUCE を追加
add_subdirectory(JUCE)

# アプリケーションターゲット
juce_add_gui_app(SoundmanDesktop
    PRODUCT_NAME "Soundman"
    COMPANY_NAME "YourCompany"
    BUNDLE_ID "com.yourcompany.soundman"
)

# ソースファイル
target_sources(SoundmanDesktop PRIVATE
    Source/Main.cpp
    Source/Core/AudioEngine.cpp
    Source/DSP/FFTAnalyzer.cpp
    Source/DSP/LevelMeter.cpp
    Source/UI/MainComponent.cpp
    Source/UI/WaveformDisplay.cpp
    Source/UI/SpectrumDisplay.cpp
    Source/Data/DataManager.cpp
)

# JUCEモジュール
target_link_libraries(SoundmanDesktop
    PRIVATE
        juce::juce_audio_basics
        juce::juce_audio_devices
        juce::juce_audio_formats
        juce::juce_audio_processors
        juce::juce_audio_utils
        juce::juce_core
        juce::juce_data_structures
        juce::juce_dsp
        juce::juce_events
        juce::juce_graphics
        juce::juce_gui_basics
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# コンパイル定義
target_compile_definitions(SoundmanDesktop
    PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:SoundmanDesktop,JUCE_PRODUCT_NAME>"
        JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:SoundmanDesktop,JUCE_VERSION>"
)
```

---

## Webアプリ（web-app/）

### 推奨ディレクトリ構造

```
web-app/
├── package.json
├── vite.config.ts
├── tsconfig.json
├── .eslintrc.js
├── .prettierrc
├── .gitignore
├── README.md
│
├── public/                     # 静的ファイル
│   ├── favicon.ico
│   └── logo.svg
│
├── src/
│   ├── main.ts                # エントリーポイント
│   ├── App.vue                # ルートコンポーネント
│   │
│   ├── assets/                # アセット
│   │   ├── styles/
│   │   │   ├── main.css
│   │   │   └── variables.css
│   │   ├── images/
│   │   └── fonts/
│   │
│   ├── components/            # UIコンポーネント
│   │   ├── common/           # 共通コンポーネント
│   │   │   ├── Button.vue
│   │   │   ├── Input.vue
│   │   │   ├── Modal.vue
│   │   │   └── Card.vue
│   │   │
│   │   ├── library/          # ライブラリ関連
│   │   │   ├── FileCard.vue
│   │   │   ├── FileList.vue
│   │   │   ├── FileGrid.vue
│   │   │   └── FilterPanel.vue
│   │   │
│   │   ├── player/           # プレーヤー関連
│   │   │   ├── AudioPlayer.vue
│   │   │   ├── WaveformViewer.vue
│   │   │   └── PlaybackControls.vue
│   │   │
│   │   └── metadata/         # メタデータ関連
│   │       ├── MetadataEditor.vue
│   │       ├── TagInput.vue
│   │       └── BPMDetector.vue
│   │
│   ├── views/                 # ページビュー
│   │   ├── HomeView.vue
│   │   ├── LibraryView.vue
│   │   ├── ProjectsView.vue
│   │   ├── SearchView.vue
│   │   └── SettingsView.vue
│   │
│   ├── stores/                # Pinia ストア
│   │   ├── library.ts
│   │   ├── player.ts
│   │   ├── filters.ts
│   │   └── settings.ts
│   │
│   ├── composables/           # Composition API
│   │   ├── useAudioPlayer.ts
│   │   ├── useFileSystem.ts
│   │   ├── useMetadata.ts
│   │   └── useSearch.ts
│   │
│   ├── services/              # サービス層
│   │   ├── fileService.ts
│   │   ├── metadataService.ts
│   │   ├── audioService.ts
│   │   └── storageService.ts
│   │
│   ├── types/                 # TypeScript型定義
│   │   ├── audio.ts
│   │   ├── library.ts
│   │   ├── metadata.ts
│   │   └── project.ts
│   │
│   ├── utils/                 # ユーティリティ
│   │   ├── formatters.ts
│   │   ├── validators.ts
│   │   ├── audio.ts
│   │   └── constants.ts
│   │
│   └── router/                # Vue Router
│       └── index.ts
│
└── tests/                      # テスト
    ├── unit/
    │   ├── components/
    │   └── services/
    └── e2e/
        └── library.spec.ts
```

### 主要型定義

#### types/audio.ts
```typescript
export interface AudioFile {
  id: string;
  path: string;
  name: string;
  type: string;
  duration: number;
  sampleRate: number;
  bitDepth: number;
  channels: number;
  size: number;
  tags: string[];
  bpm: number | null;
  key: string | null;
  metadata: AudioMetadata;
  analysis: AudioAnalysis;
  createdAt: string;
  modifiedAt: string;
}

export interface AudioMetadata {
  artist?: string;
  album?: string;
  year?: number;
  genre?: string;
  comment?: string;
}

export interface AudioAnalysis {
  rms: number;
  peak: number;
  spectralCentroid: number;
  zeroCrossingRate: number;
}
```

#### types/library.ts
```typescript
import type { AudioFile } from './audio';

export interface Library {
  version: string;
  lastModified: string;
  files: AudioFile[];
  tags: string[];
  projects: string[];
}

export interface LibraryFilter {
  searchQuery: string;
  tags: string[];
  fileTypes: string[];
  bpmRange: [number, number] | null;
  dateRange: [string, string] | null;
}
```

### Store例（Pinia）

#### stores/library.ts
```typescript
import { defineStore } from 'pinia';
import type { AudioFile, Library, LibraryFilter } from '@/types';
import { fileService } from '@/services/fileService';

export const useLibraryStore = defineStore('library', {
  state: () => ({
    library: null as Library | null,
    files: [] as AudioFile[],
    filteredFiles: [] as AudioFile[],
    selectedFiles: [] as string[],
    filter: {
      searchQuery: '',
      tags: [],
      fileTypes: [],
      bpmRange: null,
      dateRange: null,
    } as LibraryFilter,
    isLoading: false,
    error: null as string | null,
  }),

  getters: {
    fileCount: (state) => state.files.length,
    selectedCount: (state) => state.selectedFiles.length,
    allTags: (state) => state.library?.tags || [],
  },

  actions: {
    async loadLibrary() {
      this.isLoading = true;
      this.error = null;
      try {
        this.library = await fileService.loadLibrary();
        this.files = this.library.files;
        this.applyFilters();
      } catch (err) {
        this.error = err instanceof Error ? err.message : 'Failed to load library';
      } finally {
        this.isLoading = false;
      }
    },

    async addFile(file: AudioFile) {
      this.files.push(file);
      await this.saveLibrary();
      this.applyFilters();
    },

    async updateFile(id: string, updates: Partial<AudioFile>) {
      const index = this.files.findIndex((f) => f.id === id);
      if (index !== -1) {
        this.files[index] = { ...this.files[index], ...updates };
        await this.saveLibrary();
        this.applyFilters();
      }
    },

    applyFilters() {
      this.filteredFiles = this.files.filter((file) => {
        // 検索クエリ
        if (this.filter.searchQuery) {
          const query = this.filter.searchQuery.toLowerCase();
          if (!file.name.toLowerCase().includes(query) &&
              !file.tags.some(tag => tag.toLowerCase().includes(query))) {
            return false;
          }
        }

        // タグフィルタ
        if (this.filter.tags.length > 0) {
          if (!this.filter.tags.some(tag => file.tags.includes(tag))) {
            return false;
          }
        }

        // BPMフィルタ
        if (this.filter.bpmRange && file.bpm) {
          const [min, max] = this.filter.bpmRange;
          if (file.bpm < min || file.bpm > max) {
            return false;
          }
        }

        return true;
      });
    },

    async saveLibrary() {
      if (this.library) {
        this.library.files = this.files;
        this.library.lastModified = new Date().toISOString();
        await fileService.saveLibrary(this.library);
      }
    },
  },
});
```

### package.json 例

```json
{
  "name": "soundman-web",
  "version": "0.1.0",
  "type": "module",
  "scripts": {
    "dev": "vite",
    "build": "vue-tsc && vite build",
    "preview": "vite preview",
    "test:unit": "vitest",
    "test:e2e": "playwright test",
    "lint": "eslint . --ext .vue,.js,.jsx,.cjs,.mjs,.ts,.tsx,.cts,.mts --fix",
    "format": "prettier --write src/"
  },
  "dependencies": {
    "vue": "^3.4.0",
    "vue-router": "^4.2.5",
    "pinia": "^2.1.7",
    "howler": "^2.2.4",
    "wavesurfer.js": "^7.4.0",
    "chart.js": "^4.4.0",
    "date-fns": "^3.0.0"
  },
  "devDependencies": {
    "@vitejs/plugin-vue": "^5.0.0",
    "typescript": "^5.3.0",
    "vue-tsc": "^1.8.27",
    "vite": "^5.0.0",
    "vitest": "^1.0.0",
    "@playwright/test": "^1.40.0",
    "@vue/test-utils": "^2.4.0",
    "eslint": "^8.55.0",
    "eslint-plugin-vue": "^9.19.0",
    "@typescript-eslint/eslint-plugin": "^6.15.0",
    "@typescript-eslint/parser": "^6.15.0",
    "prettier": "^3.1.0",
    "tailwindcss": "^3.4.0",
    "autoprefixer": "^10.4.16",
    "postcss": "^8.4.32"
  }
}
```

### vite.config.ts 例

```typescript
import { defineConfig } from 'vite';
import vue from '@vitejs/plugin-vue';
import { fileURLToPath, URL } from 'node:url';

export default defineConfig({
  plugins: [vue()],
  resolve: {
    alias: {
      '@': fileURLToPath(new URL('./src', import.meta.url)),
    },
  },
  server: {
    port: 3000,
    open: true,
  },
  build: {
    outDir: 'dist',
    sourcemap: true,
  },
});
```

---

## 共有スキーマ（shared/schemas/）

JSONスキーマを定義し、両アプリで検証に使用します。

### library-schema.json
```json
{
  "$schema": "http://json-schema.org/draft-07/schema#",
  "title": "Soundman Library",
  "type": "object",
  "required": ["version", "lastModified", "files"],
  "properties": {
    "version": {
      "type": "string",
      "pattern": "^\\d+\\.\\d+\\.\\d+$"
    },
    "lastModified": {
      "type": "string",
      "format": "date-time"
    },
    "files": {
      "type": "array",
      "items": {
        "$ref": "#/definitions/audioFile"
      }
    },
    "tags": {
      "type": "array",
      "items": { "type": "string" }
    },
    "projects": {
      "type": "array",
      "items": { "type": "string" }
    }
  },
  "definitions": {
    "audioFile": {
      "type": "object",
      "required": ["id", "path", "name", "type"],
      "properties": {
        "id": { "type": "string" },
        "path": { "type": "string" },
        "name": { "type": "string" },
        "type": { "type": "string" },
        "duration": { "type": "number" },
        "sampleRate": { "type": "integer" },
        "bitDepth": { "type": "integer" },
        "channels": { "type": "integer" },
        "size": { "type": "integer" },
        "tags": {
          "type": "array",
          "items": { "type": "string" }
        },
        "bpm": {
          "type": ["number", "null"]
        },
        "key": {
          "type": ["string", "null"]
        }
      }
    }
  }
}
```

---

## データディレクトリ（data/）

```
data/
├── library.json                # メインライブラリファイル
├── config/
│   ├── desktop-app.json       # デスクトップアプリ設定
│   └── web-app.json           # Webアプリ設定
├── projects/                   # プロジェクトファイル
│   ├── project-uuid-1.json
│   └── project-uuid-2.json
├── cache/                      # キャッシュ
│   ├── waveforms/             # 波形画像キャッシュ
│   │   ├── file-id-1.png
│   │   └── file-id-2.png
│   └── thumbnails/            # サムネイル
│       ├── file-id-1-thumb.png
│       └── file-id-2-thumb.png
└── .locks/                     # ロックファイル
    └── library.lock
```

---

## 開発ワークフロー

### 初期セットアップ

#### デスクトップアプリ
```bash
cd desktop-app
mkdir Build && cd Build
cmake ..
cmake --build .
```

#### Webアプリ
```bash
cd web-app
npm install
npm run dev
```

### 開発時の起動順序

1. Webアプリの開発サーバー起動
   ```bash
   cd web-app && npm run dev
   ```

2. デスクトップアプリをビルド・実行
   ```bash
   cd desktop-app/Build && cmake --build . && ./SoundmanDesktop
   ```

3. 両方のアプリが `data/` ディレクトリを参照

---

## .gitignore 推奨設定

### ルート .gitignore
```
# ユーザーデータ
data/
!data/.gitkeep

# ビルド成果物
desktop-app/Build/
web-app/dist/
web-app/node_modules/

# IDE
.vscode/
.idea/
*.suo
*.user

# OS
.DS_Store
Thumbs.db
```

---

## まとめ

この構成により、デスクトップアプリとWebアプリを独立して開発しながら、`data/`ディレクトリを介してシームレスにデータを共有できます。各アプリは明確な責任分離があり、保守性と拡張性が高い設計となっています。
