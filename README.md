# OBS HEIC Image Source

Windows版OBS Studioへ `.heic` / `.heif` をドラッグ＆ドロップして、画像ソースとして追加するプラグインです。

## ダウンロード

[GitHub Releases](https://github.com/poirotec/obs-heic/releases/latest) から最新版の `OBS-HEIC-Plugin-Setup-*.exe` をダウンロードしてください。

## 動作環境

- Windows 10 / 11 (64-bit)
- OBS Studio 31以降

追加コーデックは不要です。同梱したlibheif/libde265で、iPhoneのタイル画像やHDRゲインマップを含むHEICもデコードします。

## インストール

OBSを終了してから、ダウンロードした `OBS-HEIC-Plugin-Setup-*.exe` を実行します。ユーザーアカウント制御が表示されたら許可し、画面の案内に従ってください。その後OBSを起動します。

ポータブル版OBSなどへ手動で導入する場合は、Releasesにある `OBS-HEIC-Plugin-*-Manual.zip` を展開し、ZIP内の `obs-heic` フォルダーを次へコピーします。

```text
%ProgramData%\obs-studio\plugins\
```

## 使い方

エクスプローラーからOBSのプレビュー領域へHEIC/HEIFファイルをドロップします。複数ファイルや、PNG/JPEGとの混在ドロップにも対応します。

追加後は「HEIC画像」ソースとして扱われます。ソース一覧の `+` から手動追加することもできます。

## ビルド

Visual Studio 2022とCMake 3.28以降を用意し、次を実行します。初回構成時に公式OBS SDK依存物がダウンロードされます。

```powershell
vcpkg install "libheif[core]:x64-windows"
cmake --preset windows-x64 -DLIBHEIF_ROOT=C:\path\to\vcpkg\installed\x64-windows
cmake --build --preset windows-x64
```

配布用インストーラーを作る場合はInno Setup 6を用意し、次を実行します。

```powershell
.\installer\build-installer.ps1 `
  -VcpkgRoot C:\path\to\vcpkg\installed\x64-windows `
  -InnoCompiler 'C:\Program Files (x86)\Inno Setup 6\ISCC.exe'
```

一般ユーザー向けの詳しい説明は `README.txt` を参照してください。

## 制限

- Windows専用です。
- HEIC内の先頭画像を静止画として読み込みます。Live Photosの動画部分や複数画像シーケンスは再生しません。
- 画像ファイルを書き換えた場合は、ソースのプロパティを開いて再選択してください。

## 不具合報告

[GitHub Issues](https://github.com/poirotec/obs-heic/issues) へ、OBSのバージョン、Windowsのバージョン、再現手順、OBSログを添えて報告してください。HEIC画像を公開できない場合は添付しなくて構いません。

## ライセンス

GPL-2.0-or-later
