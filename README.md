# vectorworks-plugin-native-template

C++ SDK でネイティブな Vectorworks 2026 プラグインを作るための**テンプレート**です。

フォークしてすぐ使える出発点として、ビルドシステム・CI・リリース／アップデート
の仕組みまで揃った**最小構成の動くプラグイン**が入っています。サンプルのプラグ
インはメニューコマンドを 1 つ追加するだけで、実行すると「起動した」ことを知らせる
アラートダイアログを表示します。これを自分の拡張機能に置き換え、この土台の上に本来
の機能を作り込んでいってください。

> このプロジェクトは元々特定用途のプラグイン（IFC インポータ）として立ち上げられ、
> その後、再利用可能なテンプレートへと一般化されました。以下に出てくるサンプルの
> 識別子 — `SamplePlugin` / `SamplePluginDev`、バンドル ID `com.example.vectorworks.*`、
> メニューカテゴリ **Sample** — はすべてプレースホルダーです。実際のプラグインを
> 始めるときは、これらを（`scripts/vw-update.sh` の `VW_REPO` 既定値も含めて）
> 自分の名前に置き換えてください。具体的な手順は
> [テンプレートの使い方（改名手順）](#テンプレートの使い方改名手順)を参照してください。

## 構成

```
CMakeLists.txt              macOS プラグインバンドルの CMake ビルド
src/
  ModuleMain.cpp            モジュールのエントリポイント。拡張機能を登録する
  Extensions/ExtMenu.{h,cpp}  アラートを表示するメニューコマンド
  BuildConfig.h             stable / dev の識別切り替えスイッチ（VW_DEV_BUILD）
  PluginPrefix.h            共有プレフィックスヘッダ（SDK を取り込む）
  Module-Info.plist.in      バンドルの Info.plist テンプレート（ビルドごとに埋める）
resources/
  SamplePlugin.vwr/…             stable プラグインのメニュー文字列
  SamplePluginDev.vwr/…          dev プラグインのメニュー文字列
scripts/
  vw-update.sh              最新の CI ビルドをダウンロードしてインストールする
.github/workflows/build.yml CI: Apple Silicon の macOS ランナーでビルドする
```

同じソースから、1 つのスイッチ（`VW_DEV_BUILD`、`src/BuildConfig.h` を参照）で
**共存できる 2 つのプラグイン**をビルドします。

- **`SamplePlugin.vwlibrary`** — *stable* プラグイン。`main` からビルドされます。
  メニューカテゴリは **Sample**。
- **`SamplePluginDev.vwlibrary`** — *dev* プラグイン。フィーチャー／PR ブランチから
  ビルドされます。メニューカテゴリは **Sample (Dev)**。

バンドル名・`.vwr` 識別子・VCOM ユニバーサル名・拡張機能 UUID がそれぞれ別々なので、
両方を同時にインストールしてロードできます — stable は通常利用に、dev は作業中の
ブランチを試すために使えます。各バンドルのメニューコマンドは、アラート内に自分の
チャンネルとビルドコミットを表示します。そのコミットはバンドルの `Info.plist`
（`VWBuildChannel` / `VWBuildCommit`）にも刻まれるため、アップデータが何がインス
トールされているかを判別できます。

各メニューコマンドの表示テキストは、それぞれの `resources/<name>.vwr` フォルダから
来ます。ビルド時に SDK の `BuildVWR` ツールがこれを
`<name>.vwlibrary/Contents/Resources/<name>.vwr` にパッケージするので、各バンドルは
自己完結しています。

## テンプレートの使い方（改名手順）

このテンプレートを実際のプラグインに使うときは、プレースホルダーの識別子を自分の
ものに置き換えます。下表の左側を右側（例）に置換していってください。

| 対象 | プレースホルダー | 置換する場所 |
| --- | --- | --- |
| バンドル／出力名（stable） | `SamplePlugin` | `CMakeLists.txt`（`project()`・`add_vw_plugin`）、`src/BuildConfig.h`、`resources/` フォルダ名、`scripts/vw-update.sh`、`.github/workflows/build.yml`、本 README |
| バンドル／出力名（dev） | `SamplePluginDev` | 同上 |
| バンドル ID | `com.example.vectorworks.SamplePlugin(Dev)` | `CMakeLists.txt` の `add_vw_plugin` 第 3 引数 |
| メニューカテゴリ | `Sample` / `Sample (Dev)` | `resources/*/Strings/*.vwstrings` の `category` |
| C++ 名前空間 | `SamplePlugin` | `src/Extensions/ExtMenu.{h,cpp}`、`src/ModuleMain.cpp` |
| C++ クラス | `CExtMenuSample` / `CSampleMenu_EventSink` | `src/Extensions/ExtMenu.{h,cpp}` |
| VCOM ユニバーサル名 | `CExtMenuSample_SamplePlugin(Dev)` | `src/BuildConfig.h` |
| リポジトリ | `min-nano/vectorworks-plugin-native-template` | `scripts/vw-update.sh` の `VW_REPO` 既定値 |
| 表示名・ヘルプ文言 | 「起動確認」など | `resources/*/Strings/*.vwstrings` |

多くはテキストの一括置換で済みます（`SamplePluginDev` を先に置換してから
`SamplePlugin` を置換すると安全です）。ただし次の 2 点は手作業が必要です。

- **拡張機能 UUID は必ず新しく生成してください。** `src/Extensions/ExtMenu.cpp` に
  stable / dev それぞれ 1 つずつ UUID があります。UUID はプラグインごとに世界で一意
  でなければならず、コピーしたまま使うと他のプラグインと衝突します。macOS で
  `uuidgen` を実行して 2 つ生成し、`IMPLEMENT_VWMenuExtension` の該当行を置き換え、
  コメントの UUID 文字列も合わせて更新してください。

- **`.vwstrings` は UTF-16LE（BOM 付き・CRLF 改行）です。** バイナリ扱いのエディタや
  エンコーディングを保持できるツールで編集してください。エンコーディングが崩れると
  `BuildVWR` がメニュー文字列を正しく読めなくなります。

置換後は、旧識別子が残っていないか一括検索で確認しておくと安全です。

```sh
grep -rniE "sampleplugin|com\.example|CExtMenuSample|CSampleMenu" \
  --exclude-dir=.git .
```

## ローカルでのビルド

macOS と Xcode（Vectorworks 2026 は公式に **Xcode 16.2** を対象）、および
**Vectorworks 2026 mac SDK** が必要です。

1. SDK をダウンロードして展開します:
   <https://release.vectorworks.net/latest/Vectorworks/2026-NNA-eng-mac-SDK.zip>
   （約 800 MB）。展開すると `SDKLib/` を含むフォルダができます。

2. `VW_SDK_DIR` を `SDKLib` を含むフォルダに向けて、コンフィグとビルドを行います:

   ```sh
   cmake -S . -B build -DVW_SDK_DIR=/path/to/2026-NNA-eng-mac-SDK
   cmake --build build
   ```

   成果物は `build/SamplePlugin.vwlibrary` です。

既定では Apple Silicon（`arm64`）向けにビルドします。ユニバーサルバイナリにするには:

```sh
cmake -S . -B build -DVW_SDK_DIR=/path/to/sdk \
  -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"
```

## インストールと実行（社内 / 未署名での利用）

このプラグインは社内利用向けで、**未署名**（Apple Developer ID 署名なし、
Vectorworks 開発者クレデンシャルなし）で配布されます。インストールして実行するには:

1. **バンドルをローカルディスクに置きます**（iCloud Drive は不可 — iCloud が
   ダウンロード隔離フラグを付け直すことがあります）。置き場所は Vectorworks 2026
   のユーザフォルダ内の `Plug-Ins` ディレクトリです（Vectorworks ▸ 環境設定 ▸
   *ユーザフォルダ* から探せます）。`.vwr` リソースはバンドル内に含まれているので、
   `SamplePlugin.vwlibrary` フォルダだけで十分です。

2. Gatekeeper がダウンロードしたバンドルをブロックしないよう、**macOS の隔離フラグを
   解除します**:

   ```sh
   xattr -dr com.apple.quarantine SamplePlugin.vwlibrary
   ```

   CI ビルドは既に **アドホック署名済み**です（Apple Silicon がバイナリをロードする
   ために必須。無料であり、Developer ID 署名ではありません）。ローカルビルドの場合は
   リンカが自動でアドホック署名します。それでも macOS が「壊れている」と言う場合は、
   自分で署名し直してください:

   ```sh
   codesign --force --deep --sign - SamplePlugin.vwlibrary
   ```

3. **Vectorworks を起動します。** プラグインが未署名のため、Vectorworks 2026 は起動時
   に「不明／未署名のプラグイン」警告を表示し、既定で無効化することがあります。警告を
   了解してプラグインを有効化してください — これは社内向け・クレデンシャルなしの
   プラグインでは想定どおりの挙動で、社内利用では問題ありません。

4. **コマンドをワークスペースに追加します:** ツール ▸ ワークスペース ▸ 現在の
   ワークスペースを編集 ▸ *メニュー*。**Sample** カテゴリの中に **起動確認** コマンド
   があるので、メニューにドラッグしてください。実行するとアラートが表示されます。

Vectorworks 開発者クレデンシャル（2026 の「サテライト」ファイル）は、警告の出ない
*署名済み*プラグインを配布する場合にのみ必要で、ビルドや社内での実行には不要です。

## 継続的インテグレーション（CI）

`.github/workflows/build.yml` がプラグインをビルドします。`main` は保護された
デフォルトブランチで、機能開発は必ず PR 上で行うため、ブランチを二重にビルドしない
ようトリガを分けています。

- **`main` への push**（マージ）は **stable** リリースをビルドして公開します。
- **PR** はそのブランチをビルドして **dev** プレリリースを公開します。

ワークフローの内容:

- `macos-15`（Apple Silicon）で実行し、Xcode 16.2 を選択します。
- SDK は一度だけダウンロードし、（トリミングした）SDK を**キャッシュ**するので、
  約 800 MB の zip は以降の実行で再ダウンロードされません。強制的に再ダウンロードする
  にはワークフロー内の `VW_SDK_CACHE_KEY` を変更します。
- `SamplePlugin.vwlibrary` と `SamplePluginDev.vwlibrary` の**両方**をビルドし、コミット
  で刻印（`-DVW_BUILD_VERSION`）してアドホック署名し、アーキテクチャを確認して、ビルド
  成果物としてアップロードします。PR ではエフェメラルなマージコミットではなく、PR の
  **head** コミット（あなたが push したもの）をビルドします。
- **ダウンロード可能なリリースを公開**し、アップデータが取得できる安定した URL を用意
  します:
  - `main` はローリングな **`stable`** リリースを `SamplePlugin.vwlibrary.zip` で更新
    します。
  - PR はブランチごとの **`dev-<branch>`** プレリリースを `SamplePluginDev.vwlibrary.zip`
    で更新します（トークンで公開できないフォーク PR ではスキップされます）。

  どちらもローリング方式で、毎回タグを最新ビルドに貼り直します。**stable** の公開は
  GitHub API の長時間障害があってもリトライします（stable リリースの取りこぼしは
  気づかれにくいため）。**dev** の公開はリトライしません — dev ビルドはブランチ作業中
  にしか使わないので、一時的なエラーが出たらジョブを再実行すれば十分です。

`.github/workflows/cleanup-dev-release.yml` は、ブランチが削除されたときにその
`dev-<branch>` プレリリース（とタグ）を削除し、dev ビルドが溜まらないようにします。
`delete` イベントで起動されるため、デフォルトブランチ上のコピーから実行され、この
ワークフローが `main` に入った後に削除されたブランチだけを対象とします。

`.github/workflows/stable-release-healthcheck.yml` はスケジュール（6 時間ごと）で
安全網として実行されます。公開済みの `stable` リリースが `main` の先頭からずれている
場合 — つまり stable の公開を取りこぼした場合 — `main` で `build.yml` を再ディスパッチ
して再ビルド・再公開します。スケジュール／`delete` 系のワークフローと同様に
デフォルトブランチから実行されるため、`main` にマージされて初めて有効になります。

## 自動アップデート

`scripts/vw-update.sh` は最新の CI ビルドをダウンロードして Vectorworks 2026 の
`Plug-Ins` フォルダにインストールします。手動でのダウンロード・隔離解除・コピーを
せずに、新しいビルドを確認できます。

最新ビルドを確認して、より新しいものがあるかを知らせたうえで、**更新しない /
更新だけ / 更新して再起動** を選ばせます。バンドルの隔離解除とアドホック再署名は
スクリプトが行い、「更新して再起動」では Vectorworks を終了して再起動します（コンパイル
済みプラグインは起動時にしか読み込まれないため、新しいビルドを実際にロードするには
再起動が必要です）。

リポジトリは公開なので、認証や追加ツールは不要です。スクリプトは macOS に標準で付属
するもの（`curl`・`plutil`・`unzip`・`codesign`・`xattr`・`osascript`）だけを使います。

```sh
# stable チャンネル（main → SamplePlugin）:
./scripts/vw-update.sh stable

# dev チャンネル — どのブランチのビルドを入れるか選ぶ（→ SamplePluginDev）:
./scripts/vw-update.sh dev

# 引数なし（または Finder でダブルクリック）: 最初にチャンネルを尋ねます。
./scripts/vw-update.sh
```

環境変数で上書き可能: `VW_REPO`（owner/repo）、`VW_PLUGINS_DIR`（インストール先）、
`VW_APP_NAME`（再起動するアプリ）。2 つのチャンネルは別名のバンドルをインストールする
ので、stable と dev のプラグインが互いを上書きすることはありません。
