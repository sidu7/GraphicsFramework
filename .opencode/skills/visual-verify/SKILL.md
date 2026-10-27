---
name: Visual Verification (GraphicsFramework)
description: Verify what GraphicsFramework actually renders on screen for Vulkan or OpenGL - build UnitTests, run one scene, capture the window with PrintWindow, read the pixel statistics and the Vulkan validation output, and compare backends. Use for rendering bugs, blank or black windows, crashes during texture creation, "nothing is drawn", screenshot requests, or checking that a rendering change did not regress a scene.
---

# Visual verification of GraphicsFramework rendering

Everything here is designed so that a model with **no vision and little context** can still
decide "did the scene render?" from numbers and exit codes. Reading the PNG is optional.

## 0. The three facts this workflow exists for

1. The sample apps and `UnitTests` are **GUI applications**. Success is a picture, not console
   output, so "it ran without throwing" proves nothing.
2. `CopyFromScreen` / normal screenshots return a **black image** for both the OpenGL and the
   Vulkan window on this machine. Only `PrintWindow(hwnd, hdc, PW_RENDERFULLCONTENT=2)` works.
3. The apps are console-subsystem executables, so a console window exists **in addition to** the
   SDL window (class `SDL_app`, title `Graphics Framework`). Capturing "the main window" by
   `Process.MainWindowHandle` grabs the console and yields a black rectangle. Always select the
   `SDL_app` window.

`scripts/verify-scene.ps1` (repo root) implements all three rules. **Use it; do not hand-roll
capture code unless it fails.**

## 0.1 Scope: what this covers, and what it does not

| Question | Answer |
|---|---|
| Which RHIs? | **All of them.** The capture is an OS-level `PrintWindow` of the composited window, so it does not know or care whether the pixels came from Vulkan, OpenGL or a future D3D12 backend. Verified on `Debug_Vulkan` and `Debug_OpenGL`. |
| Which apps? | **Any app built with `add_framework_app`** — every one creates its window through `Core/Window.cpp`, so the class is `SDL_app`. Currently `UnitTests` is the only one that builds (see below); pass `-BuildTarget <name>` for another. Non-SDL hosts: `-WindowClass`/`-WindowTitle`. |
| Any window mode? | Windowed and borderless are fine. **Exclusive fullscreen is not** — there is no window to capture; switch the app to windowed first. |
| Scene selection | `-scene=<0..4>` is implemented **only by `UnitTests`** (`UnitTestApplication.cpp`). Samples ignore it: pass `-NoScene`. |
| "Did anything draw?" for any clear colour | Yes: the background colour is detected per frame (most common colour), so black, white (WaveSimulation) and grey (FlockingParticles) clears all work. `-Background "r,g,b"` makes it exact. |
| "Is the picture *correct*?" | **No.** The statistics only prove pixels differ from the clear colour. Correctness needs the PNG (`read` tool) or a reference image comparison. |
| Full-screen effects | The metric is weak: if the whole window is a rendered image, there is no dominant background. Expect `bgCov` well below 50 %; read the PNG and use `-NoAssert`. |
| An existing screenshot from anywhere | Yes: `-AnalyzeOnly <path.png>` runs only the statistics. |
| Sample apps today | `WaveSimulation`, `FlockingParticles`, … **do not build** while `glslc` is on `PATH`: their GLSL is OpenGL-style, but `add_framework_spirv()` compiles it for Vulkan in every configuration. That is a pre-existing build-system limitation, not a capture problem. |

## 1. One-command verification

From the repo root (`E:\GraphicsFramework` on this machine):

```powershell
# Vulkan TextureTest, rebuilding first (scene 4 = TextureTest)
powershell -ExecutionPolicy Bypass -File scripts\verify-scene.ps1 -Preset Vulkan-Debug -Scene 4 -Build

# OpenGL reference for the same scene
powershell -ExecutionPolicy Bypass -File scripts\verify-scene.ps1 -Preset OpenGL-Debug -Scene 4 -Build
```

Presets: `Vulkan-Debug`, `Vulkan-Release`, `OpenGL-Debug`, `OpenGL-Release`.
`-Build` is optional and runs `cmake --build --preset <Preset> --target UnitTests --parallel`
inside `vcvars64.bat` (the build **must** be x64 MSVC; an x86 shell fails to link with unresolved
`operator new`).

Scene numbers (`-scene=<n>`):

| `-scene` | Scene | What must appear |
|---|---|---|
| 0 | `ShaderTest` | one large gradient triangle |
| 1 | `VertexBufferTest` | one gradient triangle (animated) |
| 2 | `IndexBufferTest` | one large gradient quad |
| 3 | `UniformBufferTest` | smaller gradient quad rotating about Z |
| 4 | `TextureTest` | JPEG (night city) quad rotating about Z |

## 2. Read the output block

```
EXE        : ...\Debug_Vulkan\UnitTests.exe
ARGS       : -scene=4
PROCESS    : running, window 1296x759
PNG        : E:\GraphicsFramework\build\verify-scenes\Debug_Vulkan_scene4.png
STDOUT     : ...\Debug_Vulkan_scene4.stdout.txt
STDERR     : ...\Debug_Vulkan_scene4.stderr.txt
STATS      : content=1.8% bg=(2,2,2)(auto) bgCov=96.5% colored=0.48% centerMean=(3,2,3) bbox=(480,279,347,198)
VALIDATION : no errors in stdout/stderr
RESULT     : PASS
```

| Field | Meaning |
|---|---|
| `PROCESS` | `running, window WxH` = the app survived long enough to render. `exited early, exit code …` = crash (see §5). |
| `PNG` | The captured image. Read it with the `read` tool to look at it; it is safe to ignore. |
| `STDOUT` | Redirected `std::cout`. Vulkan validation messages are printed there as `Vulkan Error …` / `Vulkan Warning …` (`DeviceVK.cpp` debug messenger). **An empty file means no validation messages**, which is what you want. |
| `STATS content=` | % of pixels right of the ImGui panels and below the title bar that differ from the background colour. `0%` = nothing but the clear colour was drawn. |
| `STATS bg=` | Background (clear) colour used for the comparison, plus `(auto)` or `(manual)` for how it was chosen; `(auto)` = most common colour in the frame. |
| `STATS bgCov=` | Share of the checked region covered by that colour. `>99%` with `content≈0` = empty frame; `<50%` = full-screen image, where `content` is not meaningful. |
| `STATS colored=` | % of pixels with a large channel spread (saturated geometry). Useful for gradient scenes, useless for dark textures. |
| `STATS centerMean=` | Mean RGB of the middle 50% of the window. For a black screen this is `(0,0,0)`. |
| `STATS bbox=` | Bounding box of the drawn content, `(x,y,w,h)`. `none` = nothing drawn. |
| `VALIDATION` | `no errors` or the first 10 matching lines (`Vulkan ERROR`, `VUID-`, `Failed to load`, …). |

Exit codes: `0` rendered, `1` setup problem (missing exe/window), `2` crash **or** nothing drawn.

## 3. Decide PASS / FAIL without looking at the picture

A run passes when **all three** hold:

1. exit code is `0` and `RESULT : PASS` (or `PASS with warnings` if the warnings are unrelated);
2. `STATS content=` is above the threshold (default 0.5 %, see the baseline table below);
3. `VALIDATION : no errors in stdout/stderr`.

Known-good values measured on this machine (window 1296x759, `Debug` configs, 2026-10-27):

| Scene | Vulkan `content` | OpenGL `content` | Vulkan `centerMean` |
|---|---|---|---|
| 0 `ShaderTest` | 16.51 % | 16.52 % | (40,39,39) |
| 1 `VertexBufferTest` | 16.51 % | 16.52 % | (30,57,78) |
| 2 `IndexBufferTest` | 32.92 % | 32.92 % | (119,79,118) |
| 3 `UniformBufferTest` | 5.38 % | 5.41 % | (21,13,18) |
| 4 `TextureTest` | 1.8–3.5 % | 1.8–3.5 % | (3,2,3) |

Tolerance: ±30 % relative between runs is normal, because scenes 3 and 4 rotate and the capture
happens at an arbitrary angle. A **tenfold** drop, `content=0%`, or `bbox=none` is a failure.

Scenes 3 and 4 are small quads; `content≈5 %` and `content≈2 %` are *correct*, not a partial
render. Do not "fix" a low percentage by moving the threshold.

## 4. Look at the PNG (when you can)

```
read E:\GraphicsFramework\build\verify-scenes\Debug_Vulkan_scene4.png
```

Expected, and **not** bugs:

* the window title bar plus two ImGui panels (`Projects`, `FPS`) are part of the capture;
* Vulkan geometry is **vertically mirrored** relative to OpenGL (the framework flips the Y axis
  in `Camera` via `Renderer::GetProjectionYFlip()`), so a triangle points down on Vulkan;
* `TextureTest` looks very dark on **both** backends (sRGB texture sampled into a UNORM
  swapchain). Compare Vulkan against the OpenGL PNG, never against the original JPEG;
* scenes 3 and 4 show the quad at a random rotation angle.

Bug signatures visible in the image: only the clear colour and the ImGui panels (nothing drawn),
garbage/rainbow colours (wrong vertex layout), a black or white quad (texture not bound), a quad
with zero size or clipped at the window edge (matrix/UBO wrong).

## 5. When it fails

### 5.1 `PROCESS : exited early, exit code …`

| Code | Meaning |
|---|---|
| `-1073741676` / `0xC0000094` | integer divide by zero — historically a texture-creation crash |
| `-1073741819` / `0xC0000005` | access violation |
| `-1073740791` / `0xC0000409` | stack buffer overrun / `__fastfail` (often an assertion) |

Then read the captured stdout/stderr tail that the script prints, and the full files named in
`STDOUT` / `STDERR`. Reproduce under a debugger if needed:

```powershell
& "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\Common7\IDE\devenv.exe" /debugexe `
  "build\ninja-multi\GraphicsFramework_Binary\Debug_Vulkan\UnitTests.exe" -scene=4
```

### 5.2 `RESULT : FAIL (content 0% …)` but the process is alive

The scene cleared the window and drew no geometry. Work through, in this order:

1. `STATS bbox=none` + `centerMean=(0,0,0)` → nothing reached the framebuffer.
2. Read `STDOUT`: shader load/compile failures (`Failed to load SPIR-V shader source for: …`,
   `Shader compilation failed …`) mean the `res/spirv/*.spv` files are missing — the app must run
   with its working directory set to the folder that contains `UnitTests.exe`, and SPIR-V is
   generated by `add_framework_spirv()` at build time.
3. `VALIDATION` lines with `VUID-…` usually name the exact call (descriptor set not bound, wrong
   image layout, invalid pipeline layout).
4. Suspect the descriptor path first for "alive but blank": matrices arrive through a UBO at
   `binding = 2`, textures at `binding = 0`; if the UBO is zero every vertex collapses.
5. Only then add temporary `std::cout` instrumentation (it is captured automatically by the
   script). Do **not** rely on `OutputDebugString`/`printf` alone — see §7.

### 5.3 `ERROR : No window within … ms`

The app blocked during init (device/shader/asset load) or crashed before creating the window.
Raise `-WaitMs` (e.g. `-WaitMs 20000`) for slow model/texture loads, then inspect stdout.

### 5.4 `ERROR : Executable not found`

The target is `EXCLUDE_FROM_ALL`, so building `GraphicsFramework` does **not** produce sample
binaries. Build the target explicitly (`-Build`, or
`cmake --build --preset Vulkan-Debug --target UnitTests --parallel`). For another app, add
`-BuildTarget <name>`; the script then looks for `<Config>\<name>.exe`.

## 6. Backend parity checks

To prove a change did not regress either backend, run the whole matrix (≈1 min per run):

```powershell
foreach ($p in 'Vulkan-Debug','OpenGL-Debug') {
    foreach ($s in 0..4) {
        Write-Host ("=== {0} scene {1} ===" -f $p, $s)
        powershell -ExecutionPolicy Bypass -File scripts\verify-scene.ps1 -Preset $p -Scene $s |
            Select-String -Pattern 'STATS|RESULT|VALIDATION'
        Write-Host ("exit={0}" -f $LASTEXITCODE)
    }
}
```

Then compare: both backends must exit `0` for every scene, `content` percentages must be within a
factor of ~1.5 of each other, and neither may print validation errors. PNGs land in
`build\verify-scenes\<Config>_scene<N>.png`, so Vulkan and OpenGL images sit side by side for a
visual diff.

## 6.1 Other apps and other RHIs

The same command works for any app and any RHI; only the scene argument differs:

```powershell
# a sample app: it does not parse -scene, so -NoScene; clear colour is white
powershell -ExecutionPolicy Bypass -File scripts\verify-scene.ps1 -Preset OpenGL-Debug `
    -BuildTarget WaveSimulation -NoScene -Background "255,255,255"

# a window with a custom class or several windows: disambiguate
... -WindowClass MyAppWindow -WindowTitle "My App"

# analyse a PNG produced somewhere else (another machine, a CI artifact, a bug report)
powershell -ExecutionPolicy Bypass -File scripts\verify-scene.ps1 -AnalyzeOnly C:\shots\frame.png
```

`-Background` is worth passing whenever you can read the clear colour in the source
(`SetClearColor(...)` / `glClearColor(...)`), because a full-screen effect has no dominant
background colour for the auto-detection to find.

## 7. Rules that save time

* **Never** use `CopyFromScreen`, `Graphics.CopyFromScreen`, or `SnippingTool` for these windows:
  the result is black. Use `PrintWindow` with flag `2`.
* **Never** capture `Process.MainWindowHandle`: it can be the console window. Match class
  `SDL_app` (the script does).
* Always launch the exe with `-WorkingDirectory` = the folder containing it; `res/` is resolved
  relative to the CWD.
* Prefer `std::cout` for temporary diagnostics: the script redirects stdout/stderr to files and
  greps them. `printf`/`std::cout` written to a hidden console is lost.
* The window is 1280x720 content (≈1296x759 including chrome). If a run reports a very different
  size, check `res/Settings.json` (`WindowSize`) and the DPI scaling; the `bbox` numbers shift
  with it, the percentages do not.
* Killing the process at the end of the capture is expected; the script does it.
* Do not add `#if defined(RENDERER_*)` outside the RHI seam while debugging — see `AGENTS.md`.
* Vulkan validation messages only exist in builds where the layer is enabled (Debug configs);
  `Release_Vulkan` runs may be silent even when they misbehave.
* Exclusive-fullscreen windows cannot be captured (there is no window). Run the app windowed.
* A full-screen effect (nothing but rendered pixels) has no dominant background: pass
  `-Background` from the source's `SetClearColor`, or read the PNG and use `-NoAssert`.
* If you modify the statistics code: PowerShell's `-shl`/`-shr` keep the **left operand's type**,
  so `[byte]63 -shl 12` is `0`, not `258048`. Cast to `[int]` first (this exact bug once made the
  auto-detected background black for every image).

## 8. Supporting files

* `scripts/verify-scene.ps1` — the implementation (build, launch, capture, stats, validation
  scan). Read it if you need to change thresholds or add a check.
* `references/baselines.md` — full baseline table, per-scene descriptions, tolerance guidance.
* `references/manual-capture.md` — how to capture an **already running** window, and how to drive
  the scene combo by hand.

Self-check of the statistics code (no app required — proves the background detection works for
black *and* white clears):

```powershell
# generate a white 800x600 image with a 200x200 red square, then analyse it
Add-Type -AssemblyName System.Drawing
$bmp = New-Object System.Drawing.Bitmap(800,600)
$g = [System.Drawing.Graphics]::FromImage($bmp)
$g.Clear([System.Drawing.Color]::White)
$g.FillRectangle([System.Drawing.Brushes]::Red, 400, 300, 200, 200)
$g.Dispose(); $bmp.Save("$env:TEMP\synth.png", [System.Drawing.Imaging.ImageFormat]::Png); $bmp.Dispose()

powershell -ExecutionPolicy Bypass -File scripts\verify-scene.ps1 -AnalyzeOnly "$env:TEMP\synth.png"
# expected: content=14.88% bg=(254,254,254)(auto) bgCov=85.1% bbox=(400,300,200,200)
```
