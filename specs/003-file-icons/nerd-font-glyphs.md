# Nerd Font v3 Glyph Reference for TCDir

> **Nerd Fonts Version:** 3.4.0
> **Source:** [glyphnames.json](https://github.com/ryanoasis/nerd-fonts/blob/master/glyphnames.json)
> **Cross-ref:** [Terminal-Icons](https://github.com/devblackops/Terminal-Icons) default theme (`devblackops.psd1`)

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

## Selection Priority

1. **Seti-UI** (`nf-seti-*` / `nf-custom-*`) — single-width, purpose-built for file explorers
2. **Material Design** (`nf-md-*`) — enormous set, good coverage
3. **Devicons** (`nf-dev-*`) — language/tool logos
4. **Octicons** (`nf-oct-*`) — GitHub-style icons
5. **Font Awesome** (`nf-fa-*`) — broad general-purpose
6. **Codicons** (`nf-cod-*`) — VS Code icons

All glyphs below are **single-width** in Nerd Font–patched monospace fonts.

---

## 1. Programming Languages — Source Code

| Category | Extensions | NF Class Name | Code Point | Glyph | Notes |
|----------|-----------|---------------|------------|-------|-------|
| C | `.c` | `nf-custom-c` | U+E61E | | Seti-UI custom |
| C++ | `.cpp .cxx .cc .c++ .hpp .hxx .hh .h++` | `nf-custom-cpp` | U+E61D | | Seti-UI custom |
| C# | `.cs .csx` | `nf-seti-c_sharp` | U+E648 | | Seti-UI |
| C (alt) | `.c` | `nf-md-language_c` | U+F0671 | 󰙱 | Material Design (5-digit) |
| C++ (alt) | `.cpp` | `nf-md-language_cpp` | U+F0672 | 󰙲 | Material Design (5-digit) |
| C# (alt) | `.cs` | `nf-md-language_csharp` | U+F031B | 󰌛 | Material Design (5-digit) |
| F# | `.fs .fsi .fsx .fsscript` | `nf-dev-fsharp` | U+E7A7 | | Devicons |
| Java | `.java .jar .class` | `nf-seti-java` | U+E66D | | Seti-UI |
| Java (alt) | `.java` | `nf-fae-java` | U+E256 | | FA Extension |
| Python | `.py .pyw .pyi` | `nf-seti-python` | U+E606 | | Seti-UI |
| JavaScript | `.js .mjs .cjs` | `nf-seti-javascript` | U+E60C | | Seti-UI |
| TypeScript | `.ts .mts .cts` | `nf-seti-typescript` | U+E628 | | Seti-UI |
| JSX/TSX | `.jsx .tsx` | `nf-dev-react` | U+E7BA | | Devicons |
| Rust | `.rs` | `nf-seti-rust` | U+E68B | | Seti-UI |
| Go | `.go` | `nf-seti-go2` | U+E65E | | Seti-UI |
| Ruby | `.rb .erb .gemspec` | `nf-custom-ruby` | U+E605 | | Seti-UI custom *(alias of seti-ruby)* |
| Lua | `.lua` | `nf-seti-lua` | U+E620 | | Seti-UI |
| Perl | `.pl .pm` | `nf-seti-perl` | U+E67E | | Seti-UI |
| PHP | `.php` | `nf-seti-php` | U+E608 | | Seti-UI *(verify code)* |
| Haskell | `.hs .lhs` | `nf-seti-haskell` | U+E61F | | Seti-UI |
| Dart | `.dart` | `nf-seti-dart` | U+E64C | | Seti-UI |
| Kotlin | `.kt .kts` | `nf-custom-kotlin` | U+E634 | | Seti-UI custom |
| Swift | `.swift` | `nf-seti-swift` | U+E699 | | Seti-UI |
| R | `.r .R .rmd` | `nf-seti-r` | U+E68A | | Seti-UI |
| Assembly | `.asm .s .S .cod .i .lst` | `nf-custom-asm` | U+E6AB | | Seti-UI custom |
| Assembly (alt) | `.asm` | `nf-seti-asm` | U+E637 | | Seti-UI |
| Zig | `.zig` | `nf-seti-zig` | U+E6A9 | | Seti-UI |
| Nim | `.nim .nims` | `nf-seti-nim` | U+E677 | | Seti-UI |
| Clojure | `.clj .cljs .cljc .edn` | `nf-seti-clojure` | U+E642 | | Seti-UI |
| OCaml | `.ml .mli` | `nf-seti-ocaml` | U+E67A | | Seti-UI |
| WASM | `.wasm .wat` | `nf-seti-wasm` | U+E6A1 | | Seti-UI |

---

## 2. Web / Markup / Styling

| Category | Extensions | NF Class Name | Code Point | Glyph | Notes |
|----------|-----------|---------------|------------|-------|-------|
| HTML | `.html .htm .xhtml` | `nf-seti-html` | U+E60E | | Seti-UI |
| CSS | `.css` | `nf-seti-css` | U+E614 | | Seti-UI |
| SCSS/Sass | `.scss .sass` | `nf-dev-sass` | U+E74B | | Devicons |
| LESS | `.less` | `nf-dev-less` | U+E758 | | Devicons |
| SVG | `.svg` | `nf-seti-svg` | U+E698 | | Seti-UI |
| Vue | `.vue` | `nf-seti-vue` | U+E6A0 | | Seti-UI |
| Svelte | `.svelte` | `nf-seti-svelte` | U+E697 | | Seti-UI |

---

## 3. Config / Data / Serialization

| Category | Extensions | NF Class Name | Code Point | Glyph | Notes |
|----------|-----------|---------------|------------|-------|-------|
| JSON | `.json .jsonc .json5` | `nf-seti-json` | U+E60B | | Seti-UI |
| XML | `.xml .xsl .xslt .xsd .dtd` | `nf-seti-xml` | U+E619 | | Seti-UI |
| YAML | `.yml .yaml` | `nf-seti-yml` | U+E6A8 | | Seti-UI |
| TOML | `.toml` | `nf-custom-toml` | U+E6B2 | | Seti-UI custom |
| Config | `.ini .cfg .conf .config` | `nf-seti-config` | U+E615 | | Seti-UI |
| `.resx` | `.resx` | `nf-md-xml` | U+F05C0 | 󰗀 | Material Design |
| Database | `.db .sqlite .sqlite3 .mdb` | `nf-seti-db` | U+E64D | | Seti-UI |
| SQL | `.sql` | `nf-fa-database` | U+F1C0 | | Font Awesome |
| CSV | `.csv .tsv` | `nf-seti-csv` | U+E64A | | Seti-UI *(verify code)* |

---

## 4. Documents / Office

| Category | Extensions | NF Class Name | Code Point | Glyph | Notes |
|----------|-----------|---------------|------------|-------|-------|
| Markdown | `.md .markdown .mdx` | `nf-seti-markdown` | U+E609 | | Seti-UI |
| Text | `.txt .text .nfo` | `nf-md-file_document` | U+F0219 | 󰈙 | Material Design |
| Log | `.log` | `nf-fa-list` | U+F03A | | Font Awesome |
| PDF | `.pdf` | `nf-seti-pdf` | U+E67D | | Seti-UI |
| Word | `.doc .docx .rtf` | `nf-seti-word` | U+E6A5 | | Seti-UI |
| Word (alt) | `.doc .docx` | `nf-md-file_word` | U+F022C | 󰈬 | Material Design |
| Excel | `.xls .xlsx` | `nf-seti-xls` | U+E6A6 | | Seti-UI |
| Excel (alt) | `.xls .xlsx` | `nf-md-file_excel` | U+F021B | 󰈛 | Material Design |
| PowerPoint | `.ppt .pptx` | `nf-md-file_powerpoint` | U+F0227 | 󰈧 | Material Design |
| TeX/LaTeX | `.tex .bib .cls .sty` | `nf-seti-tex` | U+E69B | | Seti-UI |
| License | `LICENSE` | `nf-seti-license` | U+E60A | | Seti-UI |

---

## 5. Scripting / Shell

| Category | Extensions | NF Class Name | Code Point | Glyph | Notes |
|----------|-----------|---------------|------------|-------|-------|
| PowerShell | `.ps1 .psm1 .psd1` | `nf-seti-powershell` | U+E683 | | Seti-UI |
| PowerShell (alt)| `.ps1` | `nf-md-console_line` | U+F07B7 | 󰞷 | Material Design |
| Batch/CMD | `.bat .cmd` | `nf-custom-msdos` | U+E629 | | Seti-UI custom |
| Shell | `.sh .bash .zsh .fish` | `nf-seti-shell` | U+E691 | | Seti-UI |
| Shell (alt) | `.sh` | `nf-oct-terminal` | U+F489 | | Octicons |
| Makefile | `Makefile` | `nf-seti-makefile` | U+E673 | | Seti-UI |

---

## 6. Build Artifacts / Compiled / Intermediate

| Category | Extensions | NF Class Name | Code Point | Glyph | Notes |
|----------|-----------|---------------|------------|-------|-------|
| Executable | `.exe .com` | `nf-md-application` | U+F08C6 | 󰣆 | Material Design |
| DLL/Library | `.dll .so .dylib` | `nf-fa-archive` | U+F187 | | Font Awesome |
| Static Lib | `.lib .a` | `nf-fa-archive` | U+F187 | | Font Awesome |
| Object | `.obj .o` | `nf-oct-file_binary` | U+F471 | | Octicons |
| PDB | `.pdb` | `nf-cod-file_binary` | U+EAE8 | | Codicons |
| PCH | `.pch .gch` | `nf-cod-file_binary` | U+EAE8 | | Codicons |
| Resource | `.rc .res` | `nf-md-file_code` | U+F022E | 󰈮 | Material Design |
| Solution | `.sln` | `nf-dev-visualstudio` | U+E70C | | Devicons |
| VS Project | `.csproj .vcxproj .vbproj .fsproj .proj` | `nf-dev-visualstudio` | U+E70C | | Devicons |
| VS User | `.user .suo` | `nf-dev-visualstudio` | U+E70C | | Devicons |
| VS Code | `.code-workspace` | `nf-md-microsoft_visual_studio_code` | U+F0A1E | 󰨞 | Material Design |

---

## 7. Archives / Compressed

| Category | Extensions | NF Class Name | Code Point | Glyph | Notes |
|----------|-----------|---------------|------------|-------|-------|
| Archives (Seti) | `.zip .7z .tar .gz .bz2 .xz .rar .zst .cab .msi` | `nf-seti-zip` | U+E6AA | | Seti-UI |
| Archives (Oct) | *same* | `nf-oct-file_zip` | U+F410 | | Octicons (used by Terminal-Icons) |
| Archives (MD) | *same* | `nf-md-zip_box` | U+F05C4 | 󰗄 | Material Design |

---

## 8. Images / Media

| Category | Extensions | NF Class Name | Code Point | Glyph | Notes |
|----------|-----------|---------------|------------|-------|-------|
| Image | `.png .jpg .jpeg .gif .bmp .ico .tiff .webp .svg` | `nf-seti-image` | U+E60D | | Seti-UI |
| Image (alt) | *same* | `nf-md-file_image` | U+F021F | 󰈟 | Material Design |
| Image (FA) | *same* | `nf-fa-image` | U+F03E | | Font Awesome |
| Video | `.mp4 .avi .mkv .mov .wmv .flv .webm` | `nf-seti-video` | U+E69F | | Seti-UI |
| Video (alt) | *same* | `nf-md-movie` | U+F0381 | 󰎁 | Material Design |
| Audio | `.mp3 .wav .ogg .flac .aac .wma .m4a` | `nf-seti-audio` | U+E638 | | Seti-UI |
| Audio (alt) | *same* | `nf-fa-music` | U+F001 | | Font Awesome |
| Font | `.ttf .otf .woff .woff2 .eot` | `nf-seti-font` | U+E659 | | Seti-UI |

---

## 9. Version Control / Project

| Category | Extensions / Pattern | NF Class Name | Code Point | Glyph | Notes |
|----------|---------------------|---------------|------------|-------|-------|
| Git ignore | `.gitignore .gitattributes` | `nf-seti-git` | U+E65D | | Seti-UI *(verify)* |
| Docker | `Dockerfile docker-compose.*` | `nf-seti-docker` | U+E650 | | Seti-UI |
| Terraform | `.tf .tfvars` | `nf-seti-terraform` | U+E69A | | Seti-UI |
| GraphQL | `.graphql .gql` | `nf-seti-graphql` | U+E662 | | Seti-UI |
| Prisma | `.prisma` | `nf-seti-prisma` | U+E684 | | Seti-UI |
| Webpack | `webpack.config.*` | `nf-seti-webpack` | U+E6A3 | | Seti-UI |
| EditorConfig | `.editorconfig` | `nf-seti-editorconfig` | U+E652 | | Seti-UI |
| Lock files | `*.lock` | `nf-seti-lock` | U+E672 | | Seti-UI |
| TODO | `TODO TODO.md` | `nf-seti-todo` | U+E69C | | Seti-UI |

---

## 10. Directory Icons

| Category | NF Class Name | Code Point | Glyph | Notes |
|----------|---------------|------------|-------|-------|
| **Default directory** | `nf-custom-folder` | U+E5FF | | Seti-UI custom (closed) |
| Default dir (open) | `nf-custom-folder_open` | U+E5FE | | Seti-UI custom (open) |
| Dir (Seti) | `nf-seti-folder` | U+E613 | | Seti-UI |
| Dir (Oct) | `nf-oct-file_directory` | U+F413 | | Octicons (used by Terminal-Icons) |
| Dir filled (Oct) | `nf-oct-file_directory_fill` | U+F4D3 | | Octicons |
| Dir (Cod) | `nf-cod-folder` | U+EA83 | | Codicons |
| Dir open (MD) | `nf-md-folder_open` | U+F0770 | 󰝰 | Material Design |
| **Dir symlink** | `nf-cod-file_symlink_directory` | U+EAED | | Codicons (used by Terminal-Icons) |
| **Dir junction** | `nf-fa-external_link` | U+F08E | | Font Awesome (used by Terminal-Icons) |

---

## 11. Well-Known Directory Icons

| Directory Name | NF Class Name | Code Point | Glyph | Notes |
|---------------|---------------|------------|-------|-------|
| `.git` | `nf-custom-folder_git` | U+E5FB | | Seti-UI custom |
| `.github` | `nf-custom-folder_github` | U+E5FD | | Seti-UI custom |
| `.vscode` | `nf-custom-folder_config` | U+E5FC | | Seti-UI custom |
| `.config` | `nf-seti-config` | U+E615 | | Seti-UI |
| `node_modules` | `nf-custom-folder_npm` | U+E5FA | | Seti-UI custom |
| `src` | `nf-oct-terminal` | U+F489 | | Octicons (Terminal-Icons) |
| `lib` / `libs` | `nf-cod-folder_library` | U+EBDF | | Codicons *(verify)* |
| `bin` | `nf-oct-file_binary` | U+F471 | | Octicons |
| `docs` / `doc` | `nf-oct-repo` | U+F401 | | Octicons |
| `tests` / `test` | `nf-md-test_tube` | U+F0668 | 󰙨 | Material Design |
| `output` / `out` | `nf-cod-output` | U+EB9D | | Codicons |
| `images` / `img` | `nf-md-folder_image` | U+F024F | 󰉏 | Material Design |
| `scripts` | `nf-seti-shell` | U+E691 | | Seti-UI (suggested) |
| `build` / `dist` | `nf-fa-cogs` | U+F085 | | Font Awesome (suggested) |
| `obj` | `nf-oct-file_binary` | U+F471 | | Octicons (suggested) |
| `packages` | `nf-seti-npm` | U+E616 | | Seti-UI (suggested) |
| `x64` / `ARM64` | `nf-md-cog` | U+F0493 | 󰒓 | Material Design (suggested) |

---

## 12. File Default / Fallback Icons

| Category | NF Class Name | Code Point | Glyph | Notes |
|----------|---------------|------------|-------|-------|
| **Default file** | `nf-fa-file` | U+F15B | | Font Awesome (used by Terminal-Icons) |
| Default file (alt) | `nf-seti-default` | U+E64E | | Seti-UI |
| Default file (alt2) | `nf-custom-default` | U+E612 | | Seti-UI custom |
| **File symlink** | `nf-oct-file_symlink_file` | U+F481 | | Octicons (used by Terminal-Icons) |
| Binary file | `nf-oct-file_binary` | U+F471 | | Octicons |
| Code file | `nf-md-file_code` | U+F022E | 󰈮 | Material Design |
| Hidden file | `nf-seti-config` | U+E615 | | Seti-UI (suggested) |
| Home | `nf-custom-home` | U+E617 | | Seti-UI custom |

---

## 13. Cloud / OneDrive Status Icons

These are **not** mapped by Terminal-Icons (file-system attribute, not extension). These are suggested assignments for `FILE_ATTRIBUTE_RECALL_ON_DATA_ACCESS` / cloud tiering status in TCDir.

| Status | NF Class Name | Code Point | Glyph | Notes |
|--------|---------------|------------|-------|-------|
| Synced / Local | `nf-md-cloud_check` | U+F0160 | 󰅠 | Fully downloaded, synced |
| Cloud-only / Dehydrated | `nf-md-cloud_outline` | U+F0163 | 󰅣 | Not local, available on demand |
| Syncing | `nf-md-cloud_sync` | U+F063F | 󰘿 | Currently syncing |
| Sync error | `nf-md-cloud_alert` | U+F09E0 | 󰧠 | Sync problem |
| Offline / Unavailable | `nf-md-cloud_off_outline` | U+F0164 | 󰅤 | Cloud disconnected |
| Pinned / Always keep | `nf-md-cloud_download` | U+F0162 | 󰅢 | Pinned for offline |
| Freeing space | `nf-md-cloud_upload` | U+F0167 | 󰅧 | Being dehydrated |
| Unknown cloud state | `nf-md-cloud_question` | U+F0A39 | 󰨹 | Unknown status |
| Cloud (generic) | `nf-md-cloud` | U+F015F | 󰅟 | Generic cloud indicator |
| Cloud (FA) | `nf-fa-cloud` | U+F0C2 | | Font Awesome (simpler glyph) |

---

## 14. Status / Indicator Icons (Potential Use)

| Purpose | NF Class Name | Code Point | Glyph | Notes |
|---------|---------------|------------|-------|-------|
| Error | `nf-seti-error` | U+E654 | | Seti-UI |
| Error (cod) | `nf-cod-error` | U+EA87 | | Codicons |
| Info | `nf-seti-info` | U+E66A | | Seti-UI |
| Warning | `nf-fa-exclamation_triangle` | U+F071 | | Font Awesome |
| Success / Check | `nf-fa-check_circle` | U+F058 | | Font Awesome |
| Failure / X | `nf-fa-times_circle` | U+F057 | | Font Awesome |
| Lock | `nf-fa-lock` | U+F023 | | Font Awesome |
| Read-only | `nf-md-lock` | U+F033E | 󰌾 | Material Design |
| Refresh / Sync | `nf-fa-refresh` | U+F021 | | Font Awesome |

---

## Quick Lookup: Terminal-Icons Default Theme Assignments

This table shows the glyph assignments from the [Terminal-Icons](https://github.com/devblackops/Terminal-Icons) PowerShell module default theme, which is a well-tested reference for console file icons.

### File Extension → Icon (Terminal-Icons defaults)

| Extension(s) | TI Glyph Name | Hex Code | Prefix Set |
|-------------|---------------|----------|------------|
| `.c` | `nf-md-language_c` | F0671 | Material |
| `.cpp .cxx .c++` | `nf-md-language_cpp` | F0672 | Material |
| `.cs .csx` | `nf-md-language_csharp` | F031B | Material |
| `.resx` | `nf-md-xml` | F05C0 | Material |
| `.fs .fsi .fsx` | `nf-dev-fsharp` | E7A7 | Devicons |
| `.h .hpp .hxx` | `nf-md-language_c` | F0671 | Material |
| `.sln` | `nf-dev-visualstudio` | E70C | Devicons |
| `.csproj .vcxproj .fsproj` | `nf-dev-visualstudio` | E70C | Devicons |
| `.user .suo` | `nf-dev-visualstudio` | E70C | Devicons |
| `.js .mjs .cjs` | `nf-dev-javascript` | E781 | Devicons |
| `.ts .mts` | `nf-seti-typescript` | E628 | Seti-UI |
| `.jsx .tsx` | `nf-dev-react` | E7BA | Devicons |
| `.html .htm` | `nf-seti-html` | E60E | Seti-UI |
| `.css` | `nf-dev-css3` | E749 | Devicons |
| `.scss .sass` | `nf-dev-sass` | E74B | Devicons |
| `.less` | `nf-dev-less` | E758 | Devicons |
| `.py .pyw` | `nf-dev-python` | E73C | Devicons |
| `.java .jar` | `nf-fae-java` | E256 | FA Extension |
| `.json .jsonc` | `nf-seti-json` | E60B | Seti-UI |
| `.xml .xsl .xslt` | `nf-md-xml` | F05C0 | Material |
| `.yml .yaml` | `nf-md-format_align_left` | F0262 | Material |
| `.md .markdown` | `nf-dev-markdown` | E73E | Devicons |
| `.txt` | `nf-md-file_document` | F0219 | Material |
| `.log .wrn .err` | `nf-fa-list` | F03A | Font Awesome |
| `.ps1 .psm1 .psd1` | `nf-md-console_line` | F07B7 | Material |
| `.bat .cmd` | `nf-custom-msdos` | E629 | Seti-UI custom |
| `.sh .bash .zsh` | `nf-oct-terminal` | F489 | Octicons |
| `.exe` | `nf-md-application` | F08C6 | Material |
| `.dll` | `nf-fa-archive` | F187 | Font Awesome |
| `.doc .docx` | `nf-md-file_word` | F022C | Material |
| `.xls .xlsx` | `nf-md-file_excel` | F021B | Material |
| `.ppt .pptx` | `nf-md-file_powerpoint` | F0227 | Material |
| `.pdf` | `nf-md-file_pdf_box` | F0226 | Material |
| `.zip .7z .tar .gz .rar` | `nf-oct-file_zip` | F410 | Octicons |
| `.png .jpg .gif .bmp .ico` | `nf-md-file_image` | F021F | Material |
| `.svg` | `nf-md-file_image` | F021F | Material |
| `.mp3 .wav .flac .ogg` | `nf-fa-music` | F001 | Font Awesome |
| `.mp4 .avi .mkv .mov` | `nf-fa-film` | F008 | Font Awesome |
| `.rs` | `nf-dev-rust` | E7A8 | Devicons |
| `.go` | `nf-dev-go` | *(check)* | Devicons |
| `.rb .erb` | `nf-dev-ruby` | *(check)* | Devicons |
| `.lua` | `nf-seti-lua` | E620 | Seti-UI |
| `.sql` | `nf-dev-database` | E706 | Devicons |

### Directory Type → Icon (Terminal-Icons defaults)

| Type | TI Glyph Name | Hex Code | Prefix Set |
|------|---------------|----------|------------|
| Default directory | `nf-oct-file_directory` | F413 | Octicons |
| Directory symlink | `nf-cod-file_symlink_directory` | EAED | Codicons |
| Directory junction | `nf-fa-external_link` | F08E | Font Awesome |
| Default file | `nf-fa-file` | F15B | Font Awesome |
| File symlink | `nf-oct-file_symlink_file` | F481 | Octicons |

### Well-Known Directory Names (Terminal-Icons defaults)

| Name | TI Glyph Name | Hex Code |
|------|---------------|----------|
| `.git` | `nf-custom-folder_git` | E5FB |
| `.github` | `nf-custom-folder_github` | E5FD |
| `.vscode` | `nf-custom-folder_config` | E5FC |
| `.config` | `nf-seti-config` | E615 |
| `node_modules` | `nf-custom-folder_npm` | E5FA |
| `src` | `nf-oct-terminal` | F489 |
| `bin` | `nf-oct-file_binary` | F471 |
| `docs` | `nf-oct-repo` | F401 |
| `tests` | `nf-md-test_tube` | F0668 |
| `output` | `nf-cod-output` | EB9D |
| `images` | `nf-md-folder_image` | F024F |

---

## Notes

### Code Point Ranges
- **4-digit hex** (e.g., `E5FA`): BMP code points. Encoded as a single `wchar_t` on Windows.
- **5-digit hex** (e.g., `F0219`): Supplementary plane code points. Require a **surrogate pair** (two `wchar_t`) on Windows.
  - All `nf-md-*` icons use 5-digit code points (U+F0001 – U+F1AF0).
  - Conversion: `U+F0219` → high surrogate `0xDB80`, low surrogate `0xDE19`
  - Formula: `hi = 0xD800 + ((cp - 0x10000) >> 10)`, `lo = 0xDC00 + ((cp - 0x10000) & 0x3FF)`

### Width Considerations
- All icons listed are single-width in properly patched Nerd Fonts.
- Some terminals may render supplementary-plane characters (5-digit) with inconsistent width.
- Test with your target font (e.g., CaskaydiaCove Nerd Font, FiraCode Nerd Font).

### Terminal-Icons vs. Seti-UI Choice
- Terminal-Icons often uses `nf-md-*` or `nf-dev-*` rather than `nf-seti-*` for some categories.
- For TCDir, prefer `nf-seti-*` / `nf-custom-*` (4-digit BMP) to avoid surrogate pair complexity, unless the `nf-md-*` icon looks significantly better.
- The Seti-UI set was specifically designed for file explorer use cases.
