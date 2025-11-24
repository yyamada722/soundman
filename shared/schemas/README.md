# JSON Schema Definitions

このディレクトリには、デスクトップアプリとWebアプリ間で共有されるJSONデータのスキーマ定義が含まれています。

## スキーマファイル

### library-schema.json

ライブラリ全体のメタデータファイル（`data/library.json`）のスキーマです。

**用途:**
- ライブラリファイルの検証
- 自動補完のサポート
- ドキュメンテーション

**主要フィールド:**
- `version`: スキーマバージョン
- `lastModified`: 最終更新日時
- `files`: オーディオファイルの配列
- `tags`: 使用されているタグの一覧
- `projects`: プロジェクトIDの配列

### project-schema.json

プロジェクトファイル（`data/projects/*.json`）のスキーマです。

**用途:**
- プロジェクトファイルの検証
- プロジェクト設定の管理

**主要フィールド:**
- `id`: プロジェクトID（UUID）
- `name`: プロジェクト名
- `description`: 説明
- `files`: 含まれるファイルID
- `settings`: プロジェクト固有の設定

## スキーマの使用方法

### デスクトップアプリ（C++）

C++でJSONスキーマを検証する場合、以下のライブラリを使用できます：

```cpp
// nlohmann/json-schema-validator を使用
#include <nlohmann/json-schema.hpp>

// スキーマを読み込む
std::ifstream schemaFile("shared/schemas/library-schema.json");
json schemaJson;
schemaFile >> schemaJson;

// バリデータを作成
json_schema::json_validator validator;
validator.set_root_schema(schemaJson);

// データを検証
std::ifstream dataFile("data/library.json");
json dataJson;
dataFile >> dataJson;

try {
    validator.validate(dataJson);
    std::cout << "Valid!" << std::endl;
} catch (const std::exception& e) {
    std::cerr << "Validation failed: " << e.what() << std::endl;
}
```

### Webアプリ（TypeScript）

TypeScriptでJSONスキーマから型を生成できます：

```bash
# json-schema-to-typescript をインストール
npm install -D json-schema-to-typescript

# 型を生成
npx json2ts shared/schemas/library-schema.json > src/types/library.ts
```

または、`ajv`を使って実行時に検証：

```typescript
import Ajv from 'ajv'
import librarySchema from '@/../../shared/schemas/library-schema.json'

const ajv = new Ajv()
const validate = ajv.compile(librarySchema)

const data = { /* ... */ }
const valid = validate(data)

if (!valid) {
  console.error(validate.errors)
}
```

## スキーマの更新

スキーマを更新する場合は、以下の点に注意してください：

1. **バージョン管理**: `version`フィールドを適切に更新
2. **後方互換性**: 既存のデータが壊れないよう注意
3. **両アプリの更新**: デスクトップアプリとWebアプリ両方で対応
4. **ドキュメント更新**: `description`フィールドを常に最新に保つ

## エディタサポート

多くのエディタは、JSONスキーマを使った自動補完とバリデーションをサポートしています：

### VS Code

`data/library.json`の先頭に以下を追加：

```json
{
  "$schema": "../shared/schemas/library-schema.json",
  "version": "1.0.0",
  ...
}
```

これにより、エディタでの自動補完とリアルタイム検証が有効になります。

## 参考資料

- [JSON Schema 公式サイト](https://json-schema.org/)
- [Understanding JSON Schema](https://json-schema.org/understanding-json-schema/)
- [JSON Schema Validator](https://www.jsonschemavalidator.net/)
