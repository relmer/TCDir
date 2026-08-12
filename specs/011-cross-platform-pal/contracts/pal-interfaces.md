# PAL Interface Contracts

The seams `TCDirCore` calls. Signatures are illustrative (wide types, `HRESULT` +
EHM per the constitution). Each seam has a **Windows backend** (passthrough to
Win32) and a **POSIX backend** (Linux/macOS). Exactly one is compiled per target.
`TCDirCore` depends only on these interfaces.

Conventions: out-params last; results captured into locals before EHM macros;
non-empty parens spaced (`Method (arg)`), empty not (`Get()`).

## IPlatform (root seam / factory)

```cpp
class IPlatform
{
public:
    virtual ~IPlatform () = default;

    virtual IFileEnumerator  & Enumerator    () = 0;
    virtual IAttributes      & Attributes    () = 0;
    virtual IConsole         & Console       () = 0;
    virtual IConsoleInput    & ConsoleInput  () = 0;
    virtual IOwnerResolver   & OwnerResolver () = 0;
    virtual IVolumeInfo      & VolumeInfo    () = 0;
    virtual IEnvironment     & Environment   () = 0;
    virtual IPlatformProfile & Profile       () = 0;
};

// One factory per build; selected at compile time.
IPlatform & GetPlatform ();   // Windows backend OR POSIX backend
```

## IFileEnumerator — bulk, per-directory (D4)

```cpp
class IFileEnumerator
{
public:
    // Enumerate ONE directory; fill a vector of fully classified, enriched
    // entries in a single native pass (type booleans, attr token, themed-match
    // set, link targets, /A visibility all applied here).
    virtual HRESULT EnumerateDirectory (const filesystem::path & dir,
                                        const FilterContext     & ctx,
                                        vector<FileEntry>       & out) = 0;

    // Classify an explicit, already-expanded path list (shell-glob case on POSIX).
    virtual HRESULT ClassifyPaths      (span<const filesystem::path> paths,
                                        const FilterContext        & ctx,
                                        vector<FileEntry>          & out) = 0;
};
```

**Contract**
- Returns `S_OK` even on partial failure (e.g. unreadable subentry); surfaces a
  status the core can report. `out` is filled once (reserve + move-once preserved).
- Applies `ctx.mask` (when the core delegates matching) case-sensitively (D8).
- Resolves `linkTarget` for symlink entries in-pass; sets `isDirectory`/`isSymlink`.
- Computes `themedMatchSet` against `ctx.themeRules`; never calls back per entry.

## IAttributes — PAL-owned, name-addressed (D3)

```cpp
class IAttributes
{
public:
    // Theming: validate a config attribute name → opaque rule-handle (invalid → error).
    virtual HRESULT ValidateAttributeRule (wstring_view name, AttrRuleHandle & out) = 0;

    // /A filter: parse user arg → opaque spec (pre-applied during enumeration).
    virtual HRESULT ParseAttributeFilter  (wstring_view arg, AttributeFilterSpec & out) = 0;

    // Late / explicit test (display-time or non-hot paths).
    virtual bool    Matches               (const FileEntry & e, const AttributeFilterSpec & f) const = 0;

    // Render the attribute column for an entry (Win: "RHSA"; POSIX: "drwxr-xr-x").
    virtual HRESULT RenderColumn          (const FileEntry & e, wstring & out) const = 0;
    virtual size_t  ColumnWidth           () const = 0;

    // Help text describing this platform's attributes (for -? / usage).
    virtual wstring HelpText              () const = 0;
};
```

**Contract**: the core passes attribute *names* and opaque tokens only; it never
sees `FILE_ATTRIBUTE_*`/`st_mode`. Column format and the attribute set are wholly
the backend's choice.

## IConsole / IConsoleInput (D5)

```cpp
class IConsole
{
public:
    virtual HRESULT Write        (wstring_view text) = 0;   // Win: WriteConsoleW/WriteFile; POSIX: UTF-8 to stdout
    virtual bool    IsRedirected () const = 0;              // GetConsoleMode / isatty
    virtual size_t  Width        () const = 0;              // ScreenBufferInfo / TIOCGWINSZ
    virtual HRESULT EnableVirtualTerminal () = 0;           // Win: SetConsoleMode; POSIX: no-op
};

class IConsoleInput   // raw key reads for TUI prompts
{
public:
    virtual HRESULT EnterRawMode () = 0;   // Win: SetConsoleMode; POSIX: termios
    virtual HRESULT ReadKey      (KeyEvent & out) = 0;
    virtual HRESULT RestoreMode  () = 0;
};
```

## IOwnerResolver / IVolumeInfo / IEnvironment

```cpp
class IOwnerResolver
{
public:
    virtual HRESULT ResolveOwner (const FileEntry & e, wstring & out) = 0;
    // Win: GetNamedSecurityInfoW + LookupAccountSidW; POSIX: getpwuid_r/getgrgid_r.
};

class IVolumeInfo
{
public:
    virtual HRESULT Query (const filesystem::path & root, VolumeInfo & out) = 0;
    // Win: GetVolumeInformation + GetDiskFreeSpaceEx; POSIX: statvfs + mount table.
};

class IEnvironment
{
public:
    virtual optional<wstring> Get          (wstring_view name) = 0;   // *EnvironmentVariableW / getenv
    virtual filesystem::path  ConfigDir    () = 0;                    // KnownFolder / $XDG_CONFIG_HOME
    virtual filesystem::path  ExecutablePath () = 0;                  // GetModuleFileNameW / /proc/self/exe
    virtual bool              IsElevated   () = 0;                    // token elevation / geteuid()==0
};
```

## IPlatformProfile

```cpp
class IPlatformProfile
{
public:
    virtual bool   HasCapability  (Capability c) const = 0;   // streams? cloud? font-install?
    virtual bool   ShowColumn     (ColumnId c) const = 0;     // FR-018 (hide meaningless columns)
    virtual HeaderMode VolumeHeaderMode () const = 0;         // FR-019
    virtual FooterMode TreeFooterMode   () const = 0;         // FR-020
};
```

## CLI contract (unchanged on Windows)

Existing switches behave identically on Windows (FR-014). New POSIX-relevant
behavior: hidden = dotfile; case-insensitive sort + case-sensitive match (D8);
volume header suppressed by default with an opt-in switch; gated features print a
clear "unavailable on this platform" message (FR-011).
