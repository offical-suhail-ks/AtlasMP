# Building AtlasMP

## Prerequisites

Install these before building:

### Required
| Tool | Version | Download |
|---|---|---|
| Visual Studio 2022 | Any edition | https://visualstudio.microsoft.com/ |
| CMake | 3.25+ | https://cmake.org/download/ |
| Git | Latest | https://git-scm.com/ |

**Visual Studio workloads required:**
- Desktop development with C++
- Windows 10/11 SDK (latest)

### Optional
| Tool | Purpose |
|---|---|
| Node.js 18+ | Build dashboard UI |
| Python 3.10+ | Helper scripts |
| Ninja | Faster builds than MSBuild |
| clang-format | Code formatting |

---

## Clone and Setup

```bash
git clone https://github.com/offical-suhail-ks/atlasmp.git
cd atlasmp

# Pull all submodules (ENet, MinHook, LuaJIT, etc.)
git submodule update --init --recursive
```

---

## Building (Windows)

### Option 1: Visual Studio GUI

```bash
cmake -B build -G "Visual Studio 17 2022" -A x64
```
Then open `build/AtlasMP.sln` in Visual Studio and build.

### Option 2: Command Line (MSBuild)

```bash
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --parallel
```

### Option 3: Ninja (Fastest)

```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

### Option 4: Helper Script

```bash
scripts\build.bat Release
```

---

## Build Options

Pass these to CMake with `-D`:

```bash
cmake -B build \
  -DATLAS_BUILD_CLIENT=ON \
  -DATLAS_BUILD_SERVER=ON \
  -DATLAS_BUILD_RUNTIME_LUA=ON \
  -DATLAS_BUILD_RUNTIME_JS=ON \
  -DATLAS_BUILD_RUNTIME_DOTNET=OFF \
  -DATLAS_BUILD_RPFLIB=ON \
  -DATLAS_BUILD_TESTS=ON
```

---

## Build Outputs

After a successful Release build:

```
build/bin/Release/
├── AtlasMP-Client.dll       ← Inject into GTA5.exe
├── AtlasMP-Server.exe       ← Run on your server
├── AtlasMP-Launcher.exe     ← Player-facing launcher
└── runtimes/
    ├── AtlasMP-Lua.dll
    ├── AtlasMP-JS.dll
    └── AtlasMP-DotNet.dll
```

---

## Running the Server

```bash
cd build/bin/Release

# First run: generates default server.toml
AtlasMP-Server.exe

# Edit server.toml, then restart
AtlasMP-Server.exe --config server.toml
```

---

## Running as Client (Development)

1. Build the project
2. Copy `AtlasMP-Client.dll` to your GTA V folder (or use the launcher)
3. Start GTA V
4. The DLL will automatically attempt to connect to `localhost:7788`

For custom server address, edit `atlasmp-client.toml` in your GTA V folder.

### Using AtlasMP Launcher

The launcher now automates the client flow:
- writes `atlasmp-client.toml` in your GTA V folder
- starts `GTA5.exe`
- injects `AtlasMP-Client.dll`

Example:

```bash
cd build/bin/Release
AtlasMP-Launcher.exe --gta "D:\Games\GTA V\GTA5.exe" --host 192.168.1.25 --port 7788 --name Suhail
```

Optional flags:
- `--password mypass` for protected servers
- `--dll <path>` to use a custom client DLL location
- `--no-inject` to only launch GTA V without AtlasMP injection
- `--inject-delay-ms <ms>` to tune injection timing on slower systems

---

## Building the Dashboard

```bash
cd dashboard
npm install
npm run dev     # Development with hot reload
npm run build   # Production build
```

The dashboard connects to the server's REST API at `http://localhost:7789/api/v1/`.

---

## Running Tests

```bash
cmake --build build --target AtlasMP-Tests
ctest --test-dir build --output-on-failure
```

---

## Common Build Errors

### `Cannot open include file: 'lua.h'`
LuaJIT submodule not initialized. Run:
```bash
git submodule update --init --recursive
```

### `MinHook.h not found`
Same — submodule issue. Run the submodule update command above.

### `LNK2019: unresolved external symbol`
Usually means a `.cpp` file is missing from a CMakeLists.txt target. Check that all source files are listed.

### Build fails on `DK22Pac/plugin-sdk`
plugin-sdk requires Windows SDK headers. Make sure you have "Windows 10 SDK" or "Windows 11 SDK" installed via Visual Studio Installer.

---

## Directory Layout After Build

```
build/
├── bin/
│   ├── Debug/
│   └── Release/
│       ├── AtlasMP-Client.dll
│       ├── AtlasMP-Server.exe
│       └── ...
├── lib/          ← Static libs
└── AtlasMP.sln   ← VS solution (if using VS generator)
```
