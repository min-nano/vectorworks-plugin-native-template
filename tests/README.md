# テスト方針

このプラグインのテストは **Vectorworks SDK を必要としない** ことを最優先に設計して
います。SDK（約 800 MB）を落とさずに、素の Linux ランナーで速く回せるので、フォークから
の PR でもカバレッジまで含めて CI が通ります（`.github/workflows/test.yml`）。

## 何をテストしているか

テスト対象は `src/UpdaterParse.h` に集約された **純粋ロジック**（`std::string` /
`std::vector` だけに依存し、`gSDK`・`dladdr`・Win32・VWFC ダイアログに一切触れない関数）
です。アップデータの実質的な分岐はここに寄せてあり、`Updater.cpp` は残った

- 自分自身のバイナリ位置の解決（`dladdr` / `GetModuleFileName`）
- スクリプトの起動（`popen` / `_popen`）
- ネイティブダイアログの表示（`gSDK->AlertInform` / `AlertQuestion`、`VWDialog`）

という **プラットフォーム／SDK 固有のグルーだけ** を担います。

`UpdaterParse.h` の関数は 3 層に分かれます。

| 層 | 関数 | 役割 |
|----|------|------|
| スクリプト出力の解析 | `Trim` / `ValueOf` / `ParseDevBuilds` | `key=value` 行・`build\t…` 行の解析 |
| コマンドライン生成 | `ShellQuote` / `CmdQuote` | `/bin/sh`・cmd.exe 用の安全なクオート |
| 自パスからの導出 | `Mac*FromBinary` / `Win*FromPath/Dir` | 同梱スクリプト・Plug-Ins フォルダのパス導出 |
| **更新フローの判断** | `EvaluateStable` / `DevSwitchCandidates` / `ResolveDevSelection` / `InstallReportedOk` / `InstallErrorText` | 「更新があるか」「切替候補はどれか」「選択→ビルド」「インストール成否」 |

最後の「更新フローの判断」層は、もともと `Updater.cpp` の `gSDK` 呼び出しの合間に
インラインで書かれていた分岐です。純粋関数として切り出したことで単体テストの対象になり、
`Updater.cpp` 側は判断結果を受けてダイアログを出すだけになりました。

## テストの実行

```sh
cmake -S . -B build -DVW_BUILD_PLUGIN=OFF -DVW_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

カバレッジ（gcov/gcovr。GCC か Clang が必要）:

```sh
cmake -S . -B buildcov -DVW_BUILD_PLUGIN=OFF -DVW_BUILD_TESTS=ON -DVW_ENABLE_COVERAGE=ON
cmake --build buildcov
ctest --test-dir buildcov
gcovr --root . --filter 'src/.*' buildcov --txt --print-summary
```

テスト自体は依存ゼロの小さなハーネス（`TestFramework.h`）で書きます。`TEST(name){ … }`
の中で `CHECK` / `CHECK_EQ` を使い、`TEST_MAIN()` を 1 か所だけ置きます。

## 残りの部分をどうカバーするか（提案）

上記の切り出しで、アップデータの「判断」はほぼ全て SDK 抜きでテストできています。まだ
テストが届いていないのは次の 2 種類だけで、いずれも **SDK / OS のモックが要る** 領域です。

### 1. `Updater.cpp` のプラットフォーム・グルー

`BundledScriptPath` / `BundlePluginsDir` / `RunScript` と、`Inform` / `Ask` /
`Install` の骨格。ロジックは薄く（導出は `*FromBinary` 系に、判定は `Evaluate*` /
`InstallReportedOk` などに委譲済み）、残るのは外部 API の呼び出しそのものです。

SDK 抜きでここまで踏み込むなら、**シーム（seam）を挿してフェイクに差し替える** のが
現実的です。案:

- **`gSDK` のフェイク化** … `Inform` / `Ask` が使うのは `AlertInform` /
  `AlertQuestion` の 2 メソッドだけです。この 2 つを持つ小さなインターフェイス
  （例 `IUpdaterUI`）を定義し、本番は `gSDK` に委譲する実装、テストは呼び出しを記録して
  戻り値を返すフェイク実装を渡します。これで `RunStableStartupCheck` /
  `RunDevStartupCheck` の「更新あり→肯定→インストール成功→完了ダイアログ」といった
  **フロー全体** を、ダイアログ文言と分岐まで含めて検証できます。
- **スクリプト実行のシーム化** … `RunScript` を関数ポインタ／`std::function` 越しに
  呼ぶようにして、テストでは実プロセスを起こさず固定の stdout を返すフェイクに差し替え。
  `q-stable` / `q-dev` / `do-install` の各出力に対する挙動を、`popen` なしで通せます。
- **VWFC ダイアログ**（`CBuildPickerDialog`）は VWFC の型に強く依存するため、単体テスト
  よりも「選択インデックス → ビルド」の写像（`ResolveDevSelection`、テスト済み）に責務を
  寄せ、ダイアログ自体は薄いままにしておくのが費用対効果に見合います。

いずれも「SDK をフルにモックする」のではなく、**プラグインが実際に触る数個の関数だけを
インターフェイス化して差し替える** 方針です。SDK ヘッダ全体のスタブ化は保守コストが高い
割にリターンが小さいので推奨しません。

### 2. シェル／PowerShell スクリプト（`scripts/vw-update.sh` / `vw-update.ps1`）

GitHub API 取得・zip 展開・インストールを担う部分。ここは C++ ではないので上記の対象外
ですが、`bats`（bash）や Pester（PowerShell）で、`curl` / `plutil` をスタブに差し替えた
うえで `asset_url` / `installed_commit` / `q_stable` などの関数単位テストが可能です。
本テンプレートの CI には含めていません（別ジョブとして追加できます）。

## まとめ

- **原則**: 分岐ロジックは `UpdaterParse.h` の純粋関数に寄せ、SDK 抜きで網羅的にテストする。
- **次の一手（任意）**: それでも残る `gSDK` / `RunScript` のグルーは、SDK 全体をモックする
  のではなく、プラグインが触る少数の呼び出しにシームを挿してフェイク化すれば、
  フロー全体まで安全にテストを広げられる。
