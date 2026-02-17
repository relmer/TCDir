#include "pch.h"

#include "IconMapping.h"





////////////////////////////////////////////////////////////////////////////////
//
//  Compile-time verification of CodePointToWideChars
//
//  Verify BMP code points produce count=1 and supplementary code points
//  produce correct surrogate pairs with count=2.
//
////////////////////////////////////////////////////////////////////////////////

// BMP code point (Nerd Font Private Use Area)
static_assert(CodePointToWideChars (NfIcon::DevCss3).count == 1,         "BMP code point must produce 1 wchar_t");
static_assert(CodePointToWideChars (NfIcon::DevCss3).chars[0] == 0xE749, "BMP code point chars[0] must equal the code point");

// Supplementary code point (Material Design — surrogate pair range)
static_assert(CodePointToWideChars (NfIcon::MdApplication).count == 2,         "Supplementary code point must produce 2 wchar_t");
static_assert(CodePointToWideChars (NfIcon::MdApplication).chars[0] == 0xDB82, "Supplementary code point high surrogate incorrect");
static_assert(CodePointToWideChars (NfIcon::MdApplication).chars[1] == 0xDCC6, "Supplementary code point low surrogate incorrect");

// Invalid code points
static_assert(CodePointToWideChars (0).count == 0,        "Zero code point must produce count=0");
static_assert(CodePointToWideChars (0xD800).count == 0,   "Surrogate code point must produce count=0");
static_assert(CodePointToWideChars (0x110000).count == 0, "Out-of-range code point must produce count=0");





////////////////////////////////////////////////////////////////////////////////
//
//  g_rgDefaultExtensionIcons
//
//  Default extension-to-icon mapping. Seeded into CConfig's
//  m_mapExtensionToIcon during Initialize.
//
////////////////////////////////////////////////////////////////////////////////

const SIconMappingEntry g_rgDefaultExtensionIcons[] =
{
    // C/C++ (Terminal-Icons: nf-md-language_c / nf-md-language_cpp)
    { L".c",       NfIcon::MdLanguageC            },
    { L".h",       NfIcon::MdLanguageC            },
    { L".cpp",     NfIcon::MdLanguageCpp          },
    { L".cxx",     NfIcon::MdLanguageCpp          },
    { L".c++",     NfIcon::MdLanguageCpp          },
    { L".hpp",     NfIcon::MdLanguageCpp          },
    { L".hxx",     NfIcon::MdLanguageCpp          },
    { L".asm",     NfIcon::CustomAsm              },
    { L".cod",     NfIcon::CustomAsm              },
    { L".i",       NfIcon::CustomAsm              },

    // C# / .NET (Terminal-Icons: nf-md-language_csharp, nf-md-xml for resx)
    { L".cs",      NfIcon::MdLanguageCsharp       },
    { L".csx",     NfIcon::MdLanguageCsharp       },
    { L".resx",    NfIcon::MdXml                  },
    { L".xaml",    NfIcon::MdLanguageXaml         },

    // JavaScript / TypeScript (Terminal-Icons: nf-dev-javascript_alt)
    { L".js",      NfIcon::DevJavascriptAlt       },
    { L".mjs",     NfIcon::DevJavascriptAlt       },
    { L".cjs",     NfIcon::DevJavascriptAlt       },
    { L".jsx",     NfIcon::DevReact               },
    { L".ts",      NfIcon::SetiTypescript         },
    { L".tsx",     NfIcon::DevReact               },

    // Web (Terminal-Icons: nf-dev-css3)
    { L".html",    NfIcon::SetiHtml               },
    { L".htm",     NfIcon::SetiHtml               },
    { L".xhtml",   NfIcon::SetiHtml               },
    { L".css",     NfIcon::DevCss3                },
    { L".scss",    NfIcon::DevSass                },
    { L".sass",    NfIcon::DevSass                },
    { L".less",    NfIcon::DevLess                },
    { L".vue",     NfIcon::MdVuejs                },
    { L".svelte",  NfIcon::SetiSvelte             },

    // Python (DEVIATION: nf-seti-python for legibility)
    { L".py",      NfIcon::SetiPython             },
    { L".pyw",     NfIcon::SetiPython             },
    { L".ipynb",   NfIcon::MdNotebook             },

    // Java (Terminal-Icons: nf-fae-java)
    { L".java",    NfIcon::FaeJava                },
    { L".jar",     NfIcon::FaeJava                },
    { L".class",   NfIcon::FaeJava                },
    { L".gradle",  NfIcon::MdElephant             },

    // Rust
    { L".rs",      NfIcon::DevRust                },

    // Go (Terminal-Icons: nf-dev-go)
    { L".go",      NfIcon::DevGo                  },

    // Ruby (Terminal-Icons: nf-oct-ruby)
    { L".rb",      NfIcon::OctRuby                },
    { L".erb",     NfIcon::OctRuby                },

    // F# (Terminal-Icons: nf-dev-fsharp)
    { L".fs",      NfIcon::DevFsharp              },
    { L".fsx",     NfIcon::DevFsharp              },
    { L".fsi",     NfIcon::DevFsharp              },

    // Lua
    { L".lua",     NfIcon::SetiLua                },

    // Perl (Terminal-Icons: nf-dev-perl)
    { L".pl",      NfIcon::DevPerl                },
    { L".pm",      NfIcon::DevPerl                },

    // PHP
    { L".php",     NfIcon::DevPhp                 },

    // Haskell (Terminal-Icons: nf-dev-haskell)
    { L".hs",      NfIcon::DevHaskell             },

    // Dart (Terminal-Icons: nf-dev-dart)
    { L".dart",    NfIcon::DevDart                },

    // Kotlin
    { L".kt",      NfIcon::CustomKotlin           },
    { L".kts",     NfIcon::CustomKotlin           },

    // Swift (not in TI; keep Seti)
    { L".swift",   NfIcon::SetiSwift              },

    // Scala (Terminal-Icons: nf-dev-scala)
    { L".scala",   NfIcon::DevScala               },
    { L".sc",      NfIcon::DevScala               },
    { L".sbt",     NfIcon::DevScala               },

    // Clojure (Terminal-Icons: nf-dev-clojure)
    { L".clj",     NfIcon::DevClojure             },
    { L".cljs",    NfIcon::DevClojure             },
    { L".cljc",    NfIcon::DevClojure             },

    // Elixir / Erlang
    { L".ex",      NfIcon::CustomElixir           },
    { L".exs",     NfIcon::CustomElixir           },
    { L".erl",     NfIcon::DevErlang              },

    // Groovy
    { L".groovy",  NfIcon::DevGroovy              },

    // Julia
    { L".jl",      NfIcon::SetiJulia              },

    // R
    { L".r",       NfIcon::MdLanguageR            },
    { L".rmd",     NfIcon::MdLanguageR            },

    // Elm
    { L".elm",     NfIcon::CustomElm              },

    // Data formats (Terminal-Icons: nf-md-xml, nf-md-format_align_left)
    { L".xml",     NfIcon::MdXml                  },
    { L".xsd",     NfIcon::MdXml                  },
    { L".xsl",     NfIcon::MdXml                  },
    { L".xslt",    NfIcon::MdXml                  },
    { L".dtd",     NfIcon::MdXml                  },
    { L".plist",   NfIcon::MdXml                  },
    { L".manifest",NfIcon::MdXml                  },
    { L".json",    NfIcon::SetiJson               },
    { L".toml",    NfIcon::FaGear                 },
    { L".yml",     NfIcon::MdFormatAlignLeft      },
    { L".yaml",    NfIcon::MdFormatAlignLeft      },

    // Config / Settings (Terminal-Icons: nf-fa-gear)
    { L".ini",     NfIcon::FaGear                 },
    { L".cfg",     NfIcon::FaGear                 },
    { L".conf",    NfIcon::FaGear                 },
    { L".config",  NfIcon::FaGear                 },
    { L".properties", NfIcon::FaGear              },
    { L".settings",NfIcon::FaGear                 },
    { L".reg",     NfIcon::FaGear                 },

    // Database / SQL (Terminal-Icons: nf-dev-database, nf-seti-db)
    { L".sql",     NfIcon::DevDatabase            },
    { L".sqlite",  NfIcon::DevDatabase            },
    { L".mdb",     NfIcon::DevDatabase            },
    { L".accdb",   NfIcon::DevDatabase            },
    { L".pgsql",   NfIcon::DevDatabase            },
    { L".db",      NfIcon::SetiDb                 },
    { L".csv",     NfIcon::MdFileExcel            },
    { L".tsv",     NfIcon::MdFileExcel            },

    // Build artifacts
    { L".obj",     NfIcon::OctFileBinary          },
    { L".lib",     NfIcon::OctFileBinary          },
    { L".res",     NfIcon::OctFileBinary          },
    { L".pch",     NfIcon::OctFileBinary          },
    { L".pdb",     NfIcon::DevDatabase            },

    // Logs
    { L".wrn",     NfIcon::FaList                 },
    { L".err",     NfIcon::FaList                 },
    { L".log",     NfIcon::FaList                 },

    // Shell (Terminal-Icons: nf-oct-terminal for sh)
    { L".bash",    NfIcon::OctTerminal            },
    { L".sh",      NfIcon::OctTerminal            },
    { L".zsh",     NfIcon::OctTerminal            },
    { L".fish",    NfIcon::OctTerminal            },
    { L".bat",     NfIcon::CustomMsdos            },
    { L".cmd",     NfIcon::CustomMsdos            },

    // PowerShell (Terminal-Icons: nf-md-console_line)
    { L".ps1",     NfIcon::MdConsoleLine          },
    { L".psd1",    NfIcon::MdConsoleLine          },
    { L".psm1",    NfIcon::MdConsoleLine          },
    { L".ps1xml",  NfIcon::MdConsoleLine          },

    // Executables
    { L".exe",     NfIcon::MdApplication          },
    { L".sys",     NfIcon::MdApplication          },
    { L".dll",     NfIcon::FaArchive              },

    // Installers (Terminal-Icons: nf-md-package_variant)
    { L".msi",     NfIcon::MdPackageVariant       },
    { L".msix",    NfIcon::MdPackageVariant       },
    { L".deb",     NfIcon::MdPackageVariant       },
    { L".rpm",     NfIcon::MdPackageVariant       },

    // Visual Studio
    { L".sln",     NfIcon::DevVisualStudio        },
    { L".vcproj",  NfIcon::DevVisualStudio        },
    { L".vcxproj", NfIcon::DevVisualStudio        },
    { L".csproj",  NfIcon::DevVisualStudio        },
    { L".csxproj", NfIcon::DevVisualStudio        },
    { L".fsproj",  NfIcon::DevFsharp              },
    { L".user",    NfIcon::DevVisualStudio        },
    { L".ncb",     NfIcon::DevVisualStudio        },
    { L".suo",     NfIcon::DevVisualStudio        },
    { L".code-workspace", NfIcon::DevVisualStudio },

    // Documents (Terminal-Icons: nf-md-file_word, nf-md-file_excel)
    { L".doc",     NfIcon::MdFileWord             },
    { L".docx",    NfIcon::MdFileWord             },
    { L".rtf",     NfIcon::MdFileWord             },
    { L".ppt",     NfIcon::MdFilePowerpoint       },
    { L".pptx",    NfIcon::MdFilePowerpoint       },
    { L".xls",     NfIcon::MdFileExcel            },
    { L".xlsx",    NfIcon::MdFileExcel            },
    { L".pdf",     NfIcon::FaFilePdfO             },

    // Markdown (Terminal-Icons: nf-dev-markdown)
    { L".md",      NfIcon::DevMarkdown            },
    { L".markdown",NfIcon::DevMarkdown            },
    { L".rst",     NfIcon::DevMarkdown            },

    // Text
    { L".txt",     NfIcon::MdFileDocument         },
    { L".text",    NfIcon::MdFileDocument         },
    { L".!!!",     NfIcon::MdFileDocument         },
    { L".1st",     NfIcon::MdFileDocument         },
    { L".me",      NfIcon::MdFileDocument         },
    { L".now",     NfIcon::MdFileDocument         },

    // Email
    { L".eml",     NfIcon::FaEnvelope             },

    // Images (Terminal-Icons: nf-fa-file_image_o)
    { L".png",     NfIcon::FaFileImageO           },
    { L".jpg",     NfIcon::FaFileImageO           },
    { L".jpeg",    NfIcon::FaFileImageO           },
    { L".gif",     NfIcon::FaFileImageO           },
    { L".bmp",     NfIcon::FaFileImageO           },
    { L".ico",     NfIcon::FaFileImageO           },
    { L".tif",     NfIcon::FaFileImageO           },
    { L".tiff",    NfIcon::FaFileImageO           },
    { L".webp",    NfIcon::FaFileImageO           },
    { L".psd",     NfIcon::FaFileImageO           },
    { L".cur",     NfIcon::FaFileImageO           },
    { L".raw",     NfIcon::FaFileImageO           },
    { L".svg",     NfIcon::MdSvg                  },

    // Audio (Terminal-Icons: nf-fa-file_audio_o)
    { L".mp3",     NfIcon::FaFileAudioO           },
    { L".wav",     NfIcon::FaFileAudioO           },
    { L".flac",    NfIcon::FaFileAudioO           },
    { L".m4a",     NfIcon::FaFileAudioO           },
    { L".wma",     NfIcon::FaFileAudioO           },
    { L".aac",     NfIcon::FaFileAudioO           },
    { L".ogg",     NfIcon::FaFileAudioO           },
    { L".opus",    NfIcon::FaFileAudioO           },
    { L".aiff",    NfIcon::FaFileAudioO           },

    // Video (Terminal-Icons: nf-fa-file_video_o)
    { L".mp4",     NfIcon::FaFileVideoO           },
    { L".avi",     NfIcon::FaFileVideoO           },
    { L".mkv",     NfIcon::FaFileVideoO           },
    { L".mov",     NfIcon::FaFileVideoO           },
    { L".wmv",     NfIcon::FaFileVideoO           },
    { L".webm",    NfIcon::FaFileVideoO           },
    { L".flv",     NfIcon::FaFileVideoO           },
    { L".mpg",     NfIcon::FaFileVideoO           },
    { L".mpeg",    NfIcon::FaFileVideoO           },

    // Fonts (Terminal-Icons: nf-fa-font)
    { L".ttf",     NfIcon::FaFont                 },
    { L".otf",     NfIcon::FaFont                 },
    { L".woff",    NfIcon::FaFont                 },
    { L".woff2",   NfIcon::FaFont                 },
    { L".eot",     NfIcon::FaFont                 },
    { L".ttc",     NfIcon::FaFont                 },

    // Archives
    { L".7z",      NfIcon::OctFileZip             },
    { L".arj",     NfIcon::OctFileZip             },
    { L".gz",      NfIcon::OctFileZip             },
    { L".rar",     NfIcon::OctFileZip             },
    { L".tar",     NfIcon::OctFileZip             },
    { L".zip",     NfIcon::OctFileZip             },
    { L".xz",      NfIcon::OctFileZip             },
    { L".bz2",     NfIcon::OctFileZip             },
    { L".tgz",     NfIcon::OctFileZip             },
    { L".cab",     NfIcon::OctFileZip             },
    { L".zst",     NfIcon::OctFileZip             },

    // Certificates / Keys (Terminal-Icons: nf-fa-certificate, nf-fa-key)
    { L".cer",     NfIcon::FaCertificate          },
    { L".cert",    NfIcon::FaCertificate          },
    { L".crt",     NfIcon::FaCertificate          },
    { L".pfx",     NfIcon::FaCertificate          },
    { L".pem",     NfIcon::FaKey                  },
    { L".pub",     NfIcon::FaKey                  },
    { L".key",     NfIcon::FaKey                  },
    { L".asc",     NfIcon::FaKey                  },
    { L".gpg",     NfIcon::FaKey                  },

    // Docker (Terminal-Icons: nf-dev-docker)
    { L".dockerfile", NfIcon::DevDocker           },
    { L".dockerignore", NfIcon::DevDocker         },

    // Terraform (using seti-terraform for NF v3 compat)
    { L".tf",      NfIcon::SetiTerraform          },
    { L".tfvars",  NfIcon::SetiTerraform          },
    { L".bicep",   NfIcon::SetiBicep              },

    // Lock files (Terminal-Icons: nf-fa-lock)
    { L".lock",    NfIcon::FaLock                 },

    // Resource (.rc)
    { L".rc",      NfIcon::SetiConfig             },
};

const size_t g_cDefaultExtensionIcons = _countof(g_rgDefaultExtensionIcons);





////////////////////////////////////////////////////////////////////////////////
//
//  g_rgDefaultWellKnownDirIcons
//
//  Default well-known directory name to icon mapping.
//
////////////////////////////////////////////////////////////////////////////////

const SIconMappingEntry g_rgDefaultWellKnownDirIcons[] =
{
    // Version control / IDEs (DEVIATIONS for legibility)
    { L".git",             NfIcon::SetiGit             },  // DEVIATION: nf-seti-git (not TI's nf-custom-folder_git)
    { L".github",          NfIcon::SetiGithub          },  // DEVIATION: nf-seti-github
    { L".vscode",          NfIcon::DevVscode           },  // DEVIATION: nf-dev-vscode
    { L".vscode-insiders", NfIcon::DevVscode           },
    { L"node_modules",     NfIcon::SetiNpm             },  // DEVIATION: nf-seti-npm (not TI's nf-custom-folder_npm)

    // Config / Cloud provider directories (Terminal-Icons)
    { L".config",          NfIcon::SetiConfig          },
    { L".cargo",           NfIcon::CustomFolderConfig  },
    { L".cache",           NfIcon::MdCached            },
    { L".docker",          NfIcon::DevDocker           },
    { L".aws",             NfIcon::DevAws              },
    { L".azure",           NfIcon::MdMicrosoftAzure    },
    { L".kube",            NfIcon::MdShipWheel         },

    // Source / Development (Terminal-Icons)
    { L"src",              NfIcon::OctTerminal         },
    { L"source",           NfIcon::OctTerminal         },
    { L"development",      NfIcon::OctTerminal         },
    { L"projects",         NfIcon::SetiProject         },

    // Documentation (Terminal-Icons)
    { L"docs",             NfIcon::OctRepo             },
    { L"doc",              NfIcon::OctRepo             },
    { L"documents",        NfIcon::OctRepo             },

    // Build outputs (Terminal-Icons)
    { L"bin",              NfIcon::OctFileBinary       },
    { L"build",            NfIcon::CodOutput           },
    { L"dist",             NfIcon::CodOutput           },
    { L"out",              NfIcon::CodOutput           },
    { L"output",           NfIcon::CodOutput           },
    { L"artifacts",        NfIcon::CodPackage          },

    // Testing (Terminal-Icons)
    { L"test",             NfIcon::MdTestTube          },
    { L"tests",            NfIcon::MdTestTube          },
    { L"__tests__",        NfIcon::MdTestTube          },
    { L"spec",             NfIcon::MdTestTube          },
    { L"specs",            NfIcon::MdTestTube          },
    { L"benchmark",        NfIcon::MdTimer             },

    // Libraries / Packages
    { L"lib",              NfIcon::CodFolderLibrary    },
    { L"libs",             NfIcon::CodFolderLibrary    },
    { L"packages",         NfIcon::SetiNpm             },

    // Scripts
    { L"scripts",          NfIcon::SetiShell           },

    // Media / Images (Terminal-Icons)
    { L"images",           NfIcon::MdFolderImage       },
    { L"img",              NfIcon::MdFolderImage       },
    { L"photos",           NfIcon::MdFolderImage       },
    { L"pictures",         NfIcon::MdFolderImage       },
    { L"assets",           NfIcon::MdFolderImage       },
    { L"videos",           NfIcon::MdMovie             },
    { L"movies",           NfIcon::MdMovie             },
    { L"media",            NfIcon::OctFileMedia        },
    { L"music",            NfIcon::MdMusicBoxMultiple  },
    { L"songs",            NfIcon::MdMusicBoxMultiple  },
    { L"fonts",            NfIcon::FaFont              },

    // User directories (Terminal-Icons)
    { L"downloads",        NfIcon::MdFolderDownload    },
    { L"desktop",          NfIcon::MdDesktopClassic    },
    { L"favorites",        NfIcon::MdFolderStar        },
    { L"contacts",         NfIcon::MdContacts          },
    { L"onedrive",         NfIcon::MdMicrosoftOnedrive },
    { L"users",            NfIcon::FaUsers             },
    { L"windows",          NfIcon::FaWindows           },

    // Other (Terminal-Icons)
    { L"apps",             NfIcon::MdApps              },
    { L"applications",     NfIcon::MdApps              },
    { L"demo",             NfIcon::CodPreview          },
    { L"samples",          NfIcon::CodPreview          },
    { L"shortcuts",        NfIcon::CodFileSymlinkDir   },
    { L"links",            NfIcon::CodFileSymlinkDir   },
    { L"github",           NfIcon::FaGithubAlt         },
};

const size_t g_cDefaultWellKnownDirIcons = _countof(g_rgDefaultWellKnownDirIcons);





////////////////////////////////////////////////////////////////////////////////
//
//  g_rgAttributePrecedenceOrder
//
//  Attribute precedence order for color+icon resolution. This is separate
//  from the display column order (k_rgFileAttributeMap in FileAttributeMap.h
//  retains the existing RHSATECP0 order).
//
//  Order: PSHERC0TA (reparse highest, archive lowest).
//
////////////////////////////////////////////////////////////////////////////////

const SFileAttributeMap g_rgAttributePrecedenceOrder[] =
{
    { FILE_ATTRIBUTE_REPARSE_POINT, L'P' },   // Priority 1 (highest)
    { FILE_ATTRIBUTE_SYSTEM,        L'S' },   // Priority 2
    { FILE_ATTRIBUTE_HIDDEN,        L'H' },   // Priority 3
    { FILE_ATTRIBUTE_ENCRYPTED,     L'E' },   // Priority 4
    { FILE_ATTRIBUTE_READONLY,      L'R' },   // Priority 5
    { FILE_ATTRIBUTE_COMPRESSED,    L'C' },   // Priority 6
    { FILE_ATTRIBUTE_SPARSE_FILE,   L'0' },   // Priority 7
    { FILE_ATTRIBUTE_TEMPORARY,     L'T' },   // Priority 8
    { FILE_ATTRIBUTE_ARCHIVE,       L'A' },   // Priority 9 (lowest)
};

const size_t g_cAttributePrecedenceOrder = _countof(g_rgAttributePrecedenceOrder);
