# Nerd Font v3 Glyph Reference for TCDir

> **Nerd Fonts Version:** 3.4.0
> **Source:** [glyphnames.json](https://github.com/ryanoasis/nerd-fonts/blob/master/glyphnames.json)
> **Baseline:** [Terminal-Icons](https://github.com/devblackops/Terminal-Icons) default theme (`devblackops.psd1`)

## Alignment Policy

TCDir uses **Terminal-Icons defaults** as its baseline icon set. Every extension,
well-known directory, and type default matches TI unless explicitly listed in
the **Deviations** section below.

### Deviations from Terminal-Icons (5 total)

These are intentional overrides chosen for better legibility at small sizes
in console output:

| Category | TI Choice | TCDir Override | Reason |
|----------|-----------|----------------|--------|
| `.git` folder | `nf-custom-folder_git` U+E5FB | `nf-seti-git` U+E65D | More recognisable at small size |
| `.github` folder | `nf-custom-folder_github` U+E5FD | `nf-seti-github` U+E65B | More recognisable at small size |
| `.vscode` folder | `nf-custom-folder_config` U+E5FC | `nf-dev-vscode` U+E8DA | Distinct VS Code logo |
| `node_modules` | `nf-custom-folder_npm` U+E5FA | `nf-seti-npm` U+E616 | More recognisable npm logo |
| `.py .pyw` | `nf-dev-python` U+E73C | `nf-seti-python` U+E606 | Better Python logo at small size |

---

## Icon Set Ranges

| Set | NF Prefix | Codepoint Range |
|-----|-----------|-----------------|
| Seti-UI + Custom | `nf-seti-*` / `nf-custom-*` | U+E5FA – U+E6B9 |
| Devicons | `nf-dev-*` | U+E700 – U+E8EF |
| Font Awesome | `nf-fa-*` | U+ED00 – U+F2FF |
| Font Awesome Extension | `nf-fae-*` | U+E200 – U+E2A9 |
| Octicons | `nf-oct-*` | U+F400 – U+F532 |
| Codicons | `nf-cod-*` | U+EA60 – U+EC1E |
| Material Design | `nf-md-*` | U+F0001 – U+F1AF0 |

All glyphs below are **single-width** in Nerd Font–patched monospace fonts.

---

## 1. Programming Languages — Source Code

| Category | Extensions | Constant | NF Class Name | Code Point | Notes |
|----------|-----------|----------|---------------|------------|-------|
| C | `.c .h` | `MdLanguageC` | `nf-md-language_c` | U+F0671 | TI default |
| C++ | `.cpp .cxx .c++ .hpp .hxx` | `MdLanguageCpp` | `nf-md-language_cpp` | U+F0672 | TI default |
| C# | `.cs .csx` | `MdLanguageCsharp` | `nf-md-language_csharp` | U+F031B | TI default |
| XAML | `.xaml` | `MdLanguageXaml` | `nf-md-language_xaml` | U+F0673 | TI default |
| F# | `.fs .fsx .fsi` | `DevFsharp` | `nf-dev-fsharp` | U+E7A7 | TI default |
| JavaScript | `.js .mjs .cjs` | `DevJavascriptAlt` | `nf-dev-javascript_alt` | U+E74E | TI default (NF v3 name) |
| TypeScript | `.ts` | `SetiTypescript` | `nf-seti-typescript` | U+E628 | TI default |
| JSX/TSX | `.jsx .tsx` | `DevReact` | `nf-dev-react` | U+E7BA | TI default |
| Python | `.py .pyw` | `SetiPython` | `nf-seti-python` | U+E606 | **DEVIATION** |
| Java | `.java .jar .class` | `FaeJava` | `nf-fae-java` | U+E256 | TI default |
| Rust | `.rs` | `DevRust` | `nf-dev-rust` | U+E7A8 | TI default |
| Go | `.go` | `DevGo` | `nf-dev-go` | U+E724 | TI default |
| Ruby | `.rb .erb` | `OctRuby` | `nf-oct-ruby` | U+F43B | TI default |
| Lua | `.lua` | `SetiLua` | `nf-seti-lua` | U+E620 | TI default |
| Perl | `.pl .pm` | `DevPerl` | `nf-dev-perl` | U+E769 | TI default |
| PHP | `.php` | `DevPhp` | `nf-dev-php` | U+E73D | TI default |
| Haskell | `.hs` | `DevHaskell` | `nf-dev-haskell` | U+E777 | TI default |
| Dart | `.dart` | `DevDart` | `nf-dev-dart` | U+E798 | TI default |
| Kotlin | `.kt .kts` | `CustomKotlin` | `nf-custom-kotlin` | U+E634 | TI default |
| Swift | `.swift` | `SetiSwift` | `nf-seti-swift` | U+E699 | Not in TI; Seti |
| Scala | `.scala .sc .sbt` | `DevScala` | `nf-dev-scala` | U+E737 | TI default |
| Clojure | `.clj .cljs .cljc` | `DevClojure` | `nf-dev-clojure` | U+E768 | TI default |
| Elixir | `.ex .exs` | `CustomElixir` | `nf-custom-elixir` | U+E62D | TI default |
| Erlang | `.erl` | `DevErlang` | `nf-dev-erlang` | U+E7B1 | TI default |
| Groovy | `.groovy` | `DevGroovy` | `nf-dev-groovy` | U+E775 | TI default |
| Julia | `.jl` | `SetiJulia` | `nf-seti-julia` | U+E624 | TI default |
| R | `.r .rmd` | `MdLanguageR` | `nf-md-language_r` | U+F07D4 | TI default |
| Elm | `.elm` | `CustomElm` | `nf-custom-elm` | U+E62C | TI default |
| Assembly | `.asm .cod .i` | `CustomAsm` | `nf-custom-asm` | U+E6AB | TCDir custom |

---

## 2. Web / Markup / Styling

| Category | Extensions | Constant | NF Class Name | Code Point | Notes |
|----------|-----------|----------|---------------|------------|-------|
| HTML | `.html .htm .xhtml` | `SetiHtml` | `nf-seti-html` | U+E60E | TI default |
| CSS | `.css` | `DevCss3` | `nf-dev-css3` | U+E749 | TI default |
| SCSS/Sass | `.scss .sass` | `DevSass` | `nf-dev-sass` | U+E74B | TI default |
| LESS | `.less` | `DevLess` | `nf-dev-less` | U+E758 | TI default |
| Vue | `.vue` | `MdVuejs` | `nf-md-vuejs` | U+F0844 | TI default |
| Svelte | `.svelte` | `SetiSvelte` | `nf-seti-svelte` | U+E697 | Not in TI; Seti |

---

## 3. Config / Data / Serialization

| Category | Extensions | Constant | NF Class Name | Code Point | Notes |
|----------|-----------|----------|---------------|------------|-------|
| JSON | `.json` | `SetiJson` | `nf-seti-json` | U+E60B | TI default |
| XML | `.xml .xsd .xsl .xslt .dtd .plist .manifest` | `MdXml` | `nf-md-xml` | U+F05C0 | TI default |
| YAML | `.yml .yaml` | `MdFormatAlignLeft` | `nf-md-format_align_left` | U+F0262 | TI default |
| Config | `.ini .cfg .conf .config .properties .settings .reg` | `FaGear` | `nf-fa-gear` | U+F013 | TI default |
| TOML | `.toml` | `FaGear` | `nf-fa-gear` | U+F013 | Treated as config |
| RESX | `.resx` | `MdXml` | `nf-md-xml` | U+F05C0 | TI default |
| Database | `.sql .sqlite .mdb .accdb .pgsql` | `DevDatabase` | `nf-dev-database` | U+E706 | TI default |
| Database (alt) | `.db` | `SetiDb` | `nf-seti-db` | U+E64D | Seti |
| CSV/TSV | `.csv .tsv` | `MdFileExcel` | `nf-md-file_excel` | U+F021B | TI default |

---

## 4. Documents / Office

| Category | Extensions | Constant | NF Class Name | Code Point | Notes |
|----------|-----------|----------|---------------|------------|-------|
| Word | `.doc .docx .rtf` | `MdFileWord` | `nf-md-file_word` | U+F022C | TI default |
| Excel | `.xls .xlsx` | `MdFileExcel` | `nf-md-file_excel` | U+F021B | TI default |
| PowerPoint | `.ppt .pptx` | `MdFilePowerpoint` | `nf-md-file_powerpoint` | U+F0227 | TI default |
| PDF | `.pdf` | `FaFilePdfO` | `nf-fa-file_pdf_o` | U+F1C1 | TI default |
| Markdown | `.md .markdown .rst` | `DevMarkdown` | `nf-dev-markdown` | U+E73E | TI default |
| Text | `.txt .text .!!! .1st .me .now` | `MdFileDocument` | `nf-md-file_document` | U+F0219 | TI default |
| Log | `.log .wrn .err` | `FaList` | `nf-fa-list` | U+F03A | TI default |
| Email | `.eml` | `FaEnvelope` | `nf-fa-envelope` | U+F0E0 | TI default |
| Notebook | `.ipynb` | `MdNotebook` | `nf-md-notebook` | U+F082E | TI default |

---

## 5. Scripting / Shell

| Category | Extensions | Constant | NF Class Name | Code Point | Notes |
|----------|-----------|----------|---------------|------------|-------|
| Shell | `.sh .bash .zsh .fish` | `OctTerminal` | `nf-oct-terminal` | U+F489 | TI default |
| PowerShell | `.ps1 .psm1 .psd1 .ps1xml` | `MdConsoleLine` | `nf-md-console_line` | U+F07B7 | TI default |
| Batch/CMD | `.bat .cmd` | `CustomMsdos` | `nf-custom-msdos` | U+E629 | TI default |

---

## 6. Build Artifacts / Compiled

| Category | Extensions | Constant | NF Class Name | Code Point | Notes |
|----------|-----------|----------|---------------|------------|-------|
| Executable | `.exe .sys` | `MdApplication` | `nf-md-application` | U+F08C6 | TI default |
| DLL | `.dll` | `FaArchive` | `nf-fa-archive` | U+F187 | TI default |
| Object/Lib | `.obj .lib .res .pch` | `OctFileBinary` | `nf-oct-file_binary` | U+F471 | TI default |
| PDB | `.pdb` | `DevDatabase` | `nf-dev-database` | U+E706 | Symbol database |
| Resource | `.rc` | `SetiConfig` | `nf-seti-config` | U+E615 | Resource script |
| Visual Studio | `.sln .vcproj .vcxproj .csproj .csxproj .user .ncb .suo .code-workspace` | `DevVisualStudio` | `nf-dev-visualstudio` | U+E70C | TI default |
| F# Project | `.fsproj` | `DevFsharp` | `nf-dev-fsharp` | U+E7A7 | Matches language |

---

## 7. Installers / Packages

| Category | Extensions | Constant | NF Class Name | Code Point | Notes |
|----------|-----------|----------|---------------|------------|-------|
| Installer | `.msi .msix .deb .rpm` | `MdPackageVariant` | `nf-md-package_variant` | U+F03D6 | TI default |

---

## 8. Archives / Compressed

| Category | Extensions | Constant | NF Class Name | Code Point | Notes |
|----------|-----------|----------|---------------|------------|-------|
| Archives | `.7z .arj .gz .rar .tar .zip .xz .bz2 .tgz .cab .zst` | `OctFileZip` | `nf-oct-file_zip` | U+F410 | TI default |

---

## 9. Images / Media

| Category | Extensions | Constant | NF Class Name | Code Point | Notes |
|----------|-----------|----------|---------------|------------|-------|
| Image | `.png .jpg .jpeg .gif .bmp .ico .tif .tiff .webp .psd .cur .raw` | `FaFileImageO` | `nf-fa-file_image_o` | U+F1C5 | TI default |
| SVG | `.svg` | `MdSvg` | `nf-md-svg` | U+F0721 | TI default |
| Audio | `.mp3 .wav .flac .m4a .wma .aac .ogg .opus .aiff` | `FaFileAudioO` | `nf-fa-file_audio_o` | U+F1C7 | TI default |
| Video | `.mp4 .avi .mkv .mov .wmv .webm .flv .mpg .mpeg` | `FaFileVideoO` | `nf-fa-file_video_o` | U+F1C8 | TI default |
| Font | `.ttf .otf .woff .woff2 .eot .ttc` | `FaFont` | `nf-fa-font` | U+F031 | TI default |

---

## 10. Security / Certificates

| Category | Extensions | Constant | NF Class Name | Code Point | Notes |
|----------|-----------|----------|---------------|------------|-------|
| Certificate | `.cer .cert .crt .pfx` | `FaCertificate` | `nf-fa-certificate` | U+F0A3 | TI default |
| Key | `.pem .pub .key .asc .gpg` | `FaKey` | `nf-fa-key` | U+F084 | TI default |
| Lock | `.lock` | `FaLock` | `nf-fa-lock` | U+F023 | TI default |

---

## 11. DevOps / Infrastructure

| Category | Extensions | Constant | NF Class Name | Code Point | Notes |
|----------|-----------|----------|---------------|------------|-------|
| Docker | `.dockerfile .dockerignore` | `DevDocker` | `nf-dev-docker` | U+E7B0 | TI default |
| Terraform | `.tf .tfvars` | `SetiTerraform` | `nf-seti-terraform` | U+E69A | NF v3 safe (TI uses E7A3 which is `dev-cplusplus` in v3) |
| Bicep | `.bicep` | `SetiBicep` | `nf-seti-bicep` | U+E63B | Azure IaC |
| Gradle | `.gradle` | `MdElephant` | `nf-md-elephant` | U+F07C6 | TI default |
| Makefile | (well-known) | `SetiMakefile` | `nf-seti-makefile` | U+E673 | TI default |

---

## 12. Type Default / Fallback Icons

| Category | Constant | NF Class Name | Code Point | Notes |
|----------|----------|---------------|------------|-------|
| **Default file** | `FaFile` | `nf-fa-file` | U+F15B | TI default |
| **Default directory** | `CustomFolder` | `nf-custom-folder` | U+E5FF | Seti-UI custom (closed) |
| **Directory symlink** | `CodFileSymlinkDir` | `nf-cod-file_symlink_directory` | U+EAED | TI default |
| **Directory junction** | `FaExternalLink` | `nf-fa-external_link` | U+F08E | TI default |

---

## 13. Well-Known Directory Icons

| Directory Name | Constant | NF Class Name | Code Point | Notes |
|---------------|----------|---------------|------------|-------|
| `.git` | `SetiGit` | `nf-seti-git` | U+E65D | **DEVIATION** |
| `.github` | `SetiGithub` | `nf-seti-github` | U+E65B | **DEVIATION** |
| `.vscode` | `DevVscode` | `nf-dev-vscode` | U+E8DA | **DEVIATION** |
| `.vscode-insiders` | `DevVscode` | `nf-dev-vscode` | U+E8DA | Same as .vscode |
| `node_modules` | `SetiNpm` | `nf-seti-npm` | U+E616 | **DEVIATION** |
| `.config` | `SetiConfig` | `nf-seti-config` | U+E615 | TI default |
| `.cargo` | `CustomFolderConfig` | `nf-custom-folder_config` | U+E5FC | Rust config |
| `.cache` | `MdCached` | `nf-md-cached` | U+F00E8 | TI default |
| `.docker` | `DevDocker` | `nf-dev-docker` | U+E7B0 | TI default |
| `.aws` | `DevAws` | `nf-dev-aws` | U+E7AD | TI default |
| `.azure` | `MdMicrosoftAzure` | `nf-md-microsoft_azure` | U+F0805 | TI default |
| `.kube` | `MdShipWheel` | `nf-md-ship_wheel` | U+F0833 | TI default |
| `src` / `source` | `OctTerminal` | `nf-oct-terminal` | U+F489 | TI default |
| `development` | `OctTerminal` | `nf-oct-terminal` | U+F489 | TI default |
| `projects` | `SetiProject` | `nf-seti-project` | U+E601 | TI default |
| `docs` / `doc` / `documents` | `OctRepo` | `nf-oct-repo` | U+F401 | TI default |
| `bin` | `OctFileBinary` | `nf-oct-file_binary` | U+F471 | TI default |
| `build` / `dist` / `out` / `output` | `CodOutput` | `nf-cod-output` | U+EB9D | TI default |
| `artifacts` | `CodPackage` | `nf-cod-package` | U+EB29 | TI default |
| `test` / `tests` / `__tests__` / `spec` / `specs` | `MdTestTube` | `nf-md-test_tube` | U+F0668 | TI default |
| `benchmark` | `MdTimer` | `nf-md-timer` | U+F13AB | TI default |
| `lib` / `libs` | `CodFolderLibrary` | `nf-cod-folder_library` | U+EBDF | TI default |
| `packages` | `SetiNpm` | `nf-seti-npm` | U+E616 | TI default |
| `scripts` | `SetiShell` | `nf-seti-shell` | U+E691 | TI default |
| `images` / `img` / `photos` / `pictures` / `assets` | `MdFolderImage` | `nf-md-folder_image` | U+F024F | TI default |
| `videos` / `movies` | `MdMovie` | `nf-md-movie` | U+F0381 | TI default |
| `media` | `OctFileMedia` | `nf-oct-file_media` | U+F40F | NF v3 safe (TI uses E732 which is `dev-archlinux` in v3) |
| `music` / `songs` | `MdMusicBoxMultiple` | `nf-md-music_box_multiple` | U+F0333 | TI default |
| `fonts` | `FaFont` | `nf-fa-font` | U+F031 | TI default |
| `downloads` | `MdFolderDownload` | `nf-md-folder_download` | U+F024D | TI default |
| `desktop` | `MdDesktopClassic` | `nf-md-desktop_classic` | U+F07C0 | TI default |
| `favorites` | `MdFolderStar` | `nf-md-folder_star` | U+F069D | TI default |
| `contacts` | `MdContacts` | `nf-md-contacts` | U+F06CB | TI default |
| `onedrive` | `MdMicrosoftOnedrive` | `nf-md-microsoft_onedrive` | U+F03CA | NF v3 safe (TI uses E762 which is `dev-behance` in v3) |
| `users` | `FaUsers` | `nf-fa-users` | U+F0C0 | TI default |
| `windows` | `FaWindows` | `nf-fa-windows` | U+F17A | TI default |
| `apps` / `applications` | `MdApps` | `nf-md-apps` | U+F003B | TI default |
| `demo` / `samples` | `CodPreview` | `nf-cod-preview` | U+EB2F | TI default |
| `shortcuts` / `links` | `CodFileSymlinkDir` | `nf-cod-file_symlink_directory` | U+EAED | TI default |
| `github` | `FaGithubAlt` | `nf-fa-github_alt` | U+F113 | TI default |

---

## 14. Cloud / OneDrive Status Icons

These are **not** mapped by Terminal-Icons (file-system attribute, not extension).
Used for `FILE_ATTRIBUTE_RECALL_ON_DATA_ACCESS` / cloud tiering status in TCDir.

| Status | Constant | NF Class Name | Code Point | Glyph | Notes |
|--------|----------|---------------|------------|-------|-------|
| Synced / Local | `MdCloudCheck` | `nf-md-cloud_check` | U+F0160 | 󰅠 | Fully downloaded |
| Cloud-only | `MdCloudOutline` | `nf-md-cloud_outline` | U+F0163 | 󰅣 | Not local |
| Pinned | `MdPin` | `nf-md-pin` | U+F0403 | 󰐃 | Always keep on device |

---

## NF v2→v3 Code Point Conflicts

Terminal-Icons was built against NF v2 naming. Some hex codes were reassigned
in NF v3. TCDir uses NF v3–safe equivalents for these:

| TI Hex | NF v2 Name | NF v3 Name (conflict) | TCDir Replacement | New Hex |
|--------|-----------|----------------------|-------------------|---------|
| E7A3 | `dev-code_badge` | `dev-cplusplus` | `nf-seti-terraform` | E69A |
| E732 | `dev-html5_multimedia` | `dev-archlinux` | `nf-oct-file_media` | F40F |
| E762 | `dev-onedrive` | `dev-behance` | `nf-md-microsoft_onedrive` | F03CA |

No conflict for: `nf-dev-javascript_alt` (E74E), `nf-dev-css3` (E749),
`nf-dev-vscode` (E8DA).

---

## Notes

### Code Point Ranges
- **4-digit hex** (e.g., `E5FA`): BMP code points. Encoded as a single `wchar_t` on Windows.
- **5-digit hex** (e.g., `F0219`): Supplementary plane code points. Require a **surrogate pair** (two `wchar_t`) on Windows.
  - All `nf-md-*` icons use 5-digit code points (U+F0001 – U+F1AF0).
  - Formula: `hi = 0xD800 + ((cp - 0x10000) >> 10)`, `lo = 0xDC00 + ((cp - 0x10000) & 0x3FF)`

### Width Considerations
- All icons listed are single-width in properly patched Nerd Fonts.
- Some terminals may render supplementary-plane characters (5-digit) with inconsistent width.
- Test with your target font (e.g., CaskaydiaCove Nerd Font, FiraCode Nerd Font).
