<#
.SYNOPSIS
    Runs one GraphicsFramework sample/UnitTests scene, captures its window to a PNG,
    collects the process stdout/stderr (Vulkan validation messages, shader errors) and
    prints simple pixel statistics that prove whether the scene actually drew anything.

.DESCRIPTION
    This is the supported way to "look at the screen" on this project, because the
    applications are GUI apps: there is no console to read success from and
    CopyFromScreen returns black for both the OpenGL and the Vulkan window.
    PrintWindow(PW_RENDERFULLCONTENT) is the only reliable capture path.

    For every run the script prints a block like:

        EXE        : ...\Debug_Vulkan\UnitTests.exe
        ARGS       : -scene=4
        PROCESS    : running after 6000 ms (window 1296x759)
        PNG        : ...\Debug_Vulkan_scene4.png
        STDOUT     : ...\Debug_Vulkan_scene4.stdout.txt
        STDERR     : ...\Debug_Vulkan_scene4.stderr.txt
        STATS      : content=8.42% bg=(0,0,0)(auto) bgCov=83.5% colored=8.10% centerMean=(97,88,103) bbox=(476,289,315,190)
        VALIDATION : no errors in stdout/stderr
        RESULT     : PASS

    Exit codes:
        0  = scene rendered (content share >= -MinContentPercent)
        1  = script/setup problem (missing exe, no window, capture failed)
        2  = process crashed, or scene rendered nothing / only ImGui panels

.PARAMETER Preset
    CMake preset whose output folder is used. One of
    OpenGL-Debug, OpenGL-Release, Vulkan-Debug, Vulkan-Release.
    Maps to build\ninja-multi\GraphicsFramework_Binary\<Debug|Release>_<OpenGL|Vulkan>.

.PARAMETER Exe
    Full path to an executable. Overrides -Preset/-Configuration (used for samples that are
    not UnitTests, or for a custom build folder).

.PARAMETER Scene
    Value passed as -scene=<n>. UnitTests: 0=ShaderTest, 1=VertexBufferTest,
    2=IndexBufferTest, 3=UniformBufferTest, 4=TextureTest. Omit with -NoScene.
    Samples ignore this argument, so pass -NoScene for them.

.PARAMETER OutDir
    Folder for the PNG and the captured logs. Default: build\verify-scenes.

.PARAMETER WaitMs
    How long to wait for a main window before giving up. Default 8000.

.PARAMETER SettleMs
    Extra wait after the window appears, so a few frames are rendered before capturing.
    Default 1500. Increase this for scenes that load models/textures slowly.

.PARAMETER MinContentPercent
    Minimum percentage of non-background pixels for the run to count as a successful render.
    Default 0.5: a scene that only cleared the screen reports exactly 0%, while the darkest
    real scene (`TextureTest`) reports 1.6-3.5%.

.PARAMETER LeftMargin
    Pixels on the left of the image that are ignored by the statistics, so the ImGui
    "Projects"/"FPS" panels (which have blue title bars and white text) do not count as
    rendered geometry. Default 320.

.PARAMETER TopMargin
    Pixels at the top of the image that are ignored, so the window title bar and border do
    not count as rendered geometry. Default 40.

.PARAMETER Background
    The colour the scene clears to, as "r,g,b" (0-255), e.g. '255,255,255' for WaveSimulation.
    When omitted the most common colour of the frame is used, which works for any clear colour
    as long as the scene is not a full-screen effect.

.PARAMETER ColorTolerance
    How far a pixel must differ from the background (per channel, 0-255) to count as content.
    Default 24. Raise it for noisy or dithered renders, lower it for subtle gradients.

.PARAMETER BuildTarget
    CMake target built by -Build. Default UnitTests. Use the sample name for samples, e.g.
    `-BuildTarget WaveSimulation`.

.PARAMETER WindowClass
    Window class to capture. Default `SDL_app`, which every window created by
    `Core/Window.cpp` uses. Only change it for a non-SDL host.

.PARAMETER WindowTitle
    Optional substring the window title must contain (e.g. 'Flocking'). Useful when a process
    owns several visible windows.

.PARAMETER AnalyzeOnly
    Path to an existing PNG. Skips building and launching and only runs the statistics on that
    image. Use it for screenshots produced elsewhere (another app, another RHI, a CI artifact,
    a colleague's machine).

.PARAMETER Build
    Run the cmake build for -Preset first. Requires -Preset (not -Exe).

.PARAMETER NoScene
    Do not append -scene=<n> to the command line.

.PARAMETER NoAssert
    Report the statistics but always exit 0 unless the process crashed.

.EXAMPLE
    # Vulkan TextureTest, the case that used to crash during texture creation
    powershell -ExecutionPolicy Bypass -File scripts\verify-scene.ps1 -Preset Vulkan-Debug -Scene 4 -Build

.EXAMPLE
    # Every UnitTests scene on both backends, one PNG per run
    foreach ($p in 'Vulkan-Debug','OpenGL-Debug') {
        foreach ($s in 0..4) {
            powershell -ExecutionPolicy Bypass -File scripts\verify-scene.ps1 -Preset $p -Scene $s
        }
    }
#>

[CmdletBinding()]
param(
    [ValidateSet('OpenGL-Debug', 'OpenGL-Release', 'Vulkan-Debug', 'Vulkan-Release')]
    [string]$Preset = 'Vulkan-Debug',

    [string]$Exe,

    [int]$Scene = 4,

    [string]$OutDir,

    [int]$WaitMs = 8000,

    [int]$SettleMs = 1500,

    [double]$MinContentPercent = 0.5,

    [int]$LeftMargin = 320,

    [int]$TopMargin = 40,

    [string]$Background,

    [int]$ColorTolerance = 24,

    [string]$BuildTarget = 'UnitTests',

    [string]$WindowClass = 'SDL_app',

    [string]$WindowTitle,

    [string]$AnalyzeOnly,

    [switch]$Build,

    [switch]$NoScene,

    [switch]$NoAssert
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$repoRoot = Split-Path -Parent $PSScriptRoot
if (-not $OutDir) { $OutDir = Join-Path $repoRoot 'build\verify-scenes' }

# ---------------------------------------------------------------- helpers ----

function Get-VcVarsPath {
    # Preferred: vswhere knows every installed MSVC toolset. The path is
    # <installationPath>\VC\Auxiliary\Build\vcvars64.bat, e.g.
    # C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat
    $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (Test-Path $vswhere) {
        $installs = & $vswhere -latest -products * `
            -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath 2>$null
        foreach ($install in $installs) {
            if (-not $install) { continue }
            $candidate = Join-Path $install 'VC\Auxiliary\Build\vcvars64.bat'
            if (Test-Path $candidate) { return $candidate }
        }
    }

    foreach ($base in @("${env:ProgramFiles(x86)}\Microsoft Visual Studio", "${env:ProgramFiles}\Microsoft Visual Studio")) {
        if (-not $base -or -not (Test-Path $base)) { continue }
        $hit = Get-ChildItem -Path $base -Filter 'vcvars64.bat' -Recurse -Depth 4 -File -ErrorAction SilentlyContinue |
            Select-Object -First 1
        if ($hit) { return $hit.FullName }
    }

    throw 'vcvars64.bat not found. Install the MSVC x64 build tools (Visual Studio Build Tools with the C++ workload).'
}

function Invoke-PresetBuild([string]$cmakePreset, [string]$target) {
    $vcvars = Get-VcVarsPath
    Write-Host "BUILD      : cmake --build --preset $cmakePreset --target $target --parallel"
    # cmake/ninja must run in an x64 MSVC environment, otherwise linking fails with
    # unresolved 'operator new' / CRT symbols.
    $cmd = '"{0}" >nul && cmake --build --preset {1} --target {2} --parallel' -f $vcvars, $cmakePreset, $target
    # Pipe through Write-Host: a bare `& cmd` would add the build log to the caller's return
    # value, and this function's caller returns the process exit code.
    & cmd.exe /c $cmd 2>&1 | ForEach-Object { Write-Host $_ }
    if ($LASTEXITCODE -ne 0) { throw "Build failed (exit $LASTEXITCODE)." }
}

function Resolve-ExePath([string]$cmakePreset, [string]$explicit, [string]$target) {
    if ($explicit) {
        if (-not (Test-Path $explicit)) { throw "Executable not found: $explicit" }
        return (Resolve-Path $explicit).Path
    }
    $parts = $cmakePreset -split '-'
    $config = '{0}_{1}' -f $parts[1], $parts[0]          # Vulkan-Debug -> Debug_Vulkan
    $path = Join-Path $repoRoot "build\ninja-multi\GraphicsFramework_Binary\$config\$target.exe"
    if (-not (Test-Path $path)) {
        throw "Executable not found: $path`nBuild it first: -Build (target $target), or run cmake --build --preset $cmakePreset --target $target --parallel"
    }
    return $path
}

function Parse-Color([string]$text) {
    $parts = $text -split '[,;\s]+' | Where-Object { $_ -ne '' }
    if ($parts.Count -ne 3) { throw "-Background must be 'r,g,b' with values 0-255, got '$text'." }
    $values = @()
    foreach ($p in $parts) {
        $v = 0
        if (-not [int]::TryParse($p, [ref]$v)) { throw "-Background component '$p' is not an integer." }
        if ($v -lt 0 -or $v -gt 255) { throw "-Background component '$p' must be 0-255." }
        $values += $v
    }
    return $values
}

# Native calls needed for a reliable capture. CopyFromScreen gives a black image for both
# backends, so PrintWindow with PW_RENDERFULLCONTENT (2) is used instead.
$win32 = @"
using System;
using System.Runtime.InteropServices;
using System.Text;
public class GfWin32 {
    [DllImport("user32.dll", SetLastError=true)]
    public static extern bool PrintWindow(IntPtr hwnd, IntPtr hdcBlt, uint nFlags);
    [DllImport("user32.dll")]
    public static extern bool GetWindowRect(IntPtr hWnd, out RECT lpRect);
    [DllImport("user32.dll")]
    public static extern bool SetForegroundWindow(IntPtr hWnd);
    [DllImport("user32.dll")]
    public static extern bool IsWindowVisible(IntPtr hWnd);
    [DllImport("user32.dll")]
    public static extern bool EnumWindows(EnumWindowsProc lpEnumFunc, IntPtr lParam);
    [DllImport("user32.dll")]
    public static extern uint GetWindowThreadProcessId(IntPtr hWnd, out uint lpdwProcessId);
    [DllImport("user32.dll", CharSet = CharSet.Unicode)]
    public static extern int GetClassName(IntPtr hWnd, StringBuilder lpClassName, int nMaxCount);
    public delegate bool EnumWindowsProc(IntPtr hWnd, IntPtr lParam);

    [StructLayout(LayoutKind.Sequential)]
    public struct RECT { public int Left, Top, Right, Bottom; }

    // The process is a console-subsystem exe, so launching it (especially with redirected
    // stdout/stderr) also creates a console window. Process.MainWindowHandle can return that
    // console instead of the real SDL window, so the SDL window is looked up by class name.
    public static IntPtr FindByClass(uint pid, string className) {
        return FindByClassAndTitle(pid, className, null);
    }

    [DllImport("user32.dll", CharSet = CharSet.Unicode)]
    public static extern int GetWindowTextW(IntPtr hWnd, StringBuilder lpString, int nMaxCount);

    public static IntPtr FindByClassAndTitle(uint pid, string className, string title) {
        IntPtr found = IntPtr.Zero;
        EnumWindows(delegate(IntPtr hWnd, IntPtr lParam) {
            uint owner;
            GetWindowThreadProcessId(hWnd, out owner);
            if (owner != pid) return true;
            if (!IsWindowVisible(hWnd)) return true;
            var sb = new StringBuilder(256);
            GetClassName(hWnd, sb, sb.Capacity);
            if (className != null && sb.ToString() != className) return true;
            if (title != null) {
                var tb = new StringBuilder(256);
                GetWindowTextW(hWnd, tb, tb.Capacity);
                if (!tb.ToString().Contains(title)) return true;
            }
            found = hWnd; return false;
        }, IntPtr.Zero);
        return found;
    }

    public static IntPtr FindLargest(uint pid) {
        IntPtr best = IntPtr.Zero;
        long bestArea = -1;
        EnumWindows(delegate(IntPtr hWnd, IntPtr lParam) {
            uint owner;
            GetWindowThreadProcessId(hWnd, out owner);
            if (owner != pid) return true;
            if (!IsWindowVisible(hWnd)) return true;
            RECT r;
            GetWindowRect(hWnd, out r);
            long area = (long)(r.Right - r.Left) * (r.Bottom - r.Top);
            if (area > bestArea) { bestArea = area; best = hWnd; }
            return true;
        }, IntPtr.Zero);
        return best;
    }
}
"@
if (-not ('GfWin32' -as [type])) { Add-Type -AssemblyName System.Drawing; Add-Type -TypeDefinition $win32 }

function Save-WindowPng([IntPtr]$hwnd, [string]$outFile) {
    $rect = New-Object GfWin32+RECT
    if (-not [GfWin32]::GetWindowRect($hwnd, [ref]$rect)) { throw 'GetWindowRect failed.' }
    $w = $rect.Right - $rect.Left
    $h = $rect.Bottom - $rect.Top
    if ($w -le 0 -or $h -le 0) { throw "Invalid window size ${w}x${h}." }

    $bmp = New-Object System.Drawing.Bitmap($w, $h)
    $gfx = [System.Drawing.Graphics]::FromImage($bmp)
    $hdc = $gfx.GetHdc()
    try { $ok = [GfWin32]::PrintWindow($hwnd, $hdc, 2) }
    finally { $gfx.ReleaseHdc($hdc); $gfx.Dispose() }
    $bmp.Save($outFile, [System.Drawing.Imaging.ImageFormat]::Png)
    $bmp.Dispose()

    if (-not $ok) { throw 'PrintWindow returned false.' }
    return @{ Width = $w; Height = $h }
}

# Pixel statistics so a human *or* a small model can decide "did it render?" without
# interpreting the picture.
#
# "Background" is the colour the scene cleared to. It is detected automatically as the most
# common colour in the checked region (right of the ImGui panels, below the title bar), or
# supplied explicitly with -Background, which is the accurate option when you know the clear
# colour (e.g. WaveSimulation clears to white). content% is the share of pixels that differ
# from that background, so it means the same thing for a black, white or grey clear colour.
#
# Reported counts:
#   * content - pixels that differ from the background colour (any RHI, any clear colour)
#   * colored - subset with a large channel spread (saturated geometry); useless for dark textures
# A scene that only cleared the screen reports content=0%.
function Get-ImageStats([string]$path, [int]$leftMargin, [int]$topMargin, $background, [int]$tolerance) {
    $src = [System.Drawing.Bitmap]::FromFile($path)
    try {
        $bmp = New-Object System.Drawing.Bitmap($src.Width, $src.Height, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
        $g = [System.Drawing.Graphics]::FromImage($bmp)
        $g.DrawImage($src, 0, 0, $src.Width, $src.Height)
        $g.Dispose()

        $w = $bmp.Width; $h = $bmp.Height
        $rectAll = New-Object System.Drawing.Rectangle(0, 0, $w, $h)
        $data = $bmp.LockBits($rectAll, [System.Drawing.Imaging.ImageLockMode]::ReadOnly, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
        $stride = $data.Stride
        $bytes = New-Object byte[] ($stride * $h)
        [System.Runtime.InteropServices.Marshal]::Copy($data.Scan0, $bytes, 0, $bytes.Length)
        $bmp.UnlockBits($data)
        $bmp.Dispose()

        # Pass 1: histogram of 6-bit-per-channel colours inside the checked region. The most
        # frequent bucket is the clear colour of a normally composed frame.
        $bgSource = 'auto'
        $considered = 0
        $bgR = 0; $bgG = 0; $bgB = 0
        if ($null -eq $background) {
            $bins = New-Object int[] 262144
            for ($y = $topMargin; $y -lt $h; $y++) {
                $row = $y * $stride
                for ($x = $leftMargin; $x -lt $w; $x++) {
                    $i = $row + ($x * 4)
                    # Cast to [int]: PowerShell's -shl keeps the left operand's type, and a
                    # [byte] 63 -shl 12 silently truncates to 0, which would make the detected
                    # background black for every image.
                    $key = (([int]$bytes[$i + 2] -shr 2) -shl 12) -bor (([int]$bytes[$i + 1] -shr 2) -shl 6) -bor ([int]$bytes[$i] -shr 2)
                    $bins[$key]++
                }
            }
            $best = 0; $bestKey = 0
            for ($k = 0; $k -lt $bins.Length; $k++) {
                if ($bins[$k] -gt $best) { $best = $bins[$k]; $bestKey = $k }
            }
            $bgR = ((($bestKey -shr 12) -band 0x3F) -shl 2) + 2
            $bgG = ((($bestKey -shr 6) -band 0x3F) -shl 2) + 2
            $bgB = (($bestKey -band 0x3F) -shl 2) + 2
            $bgCoverage = if ($best -gt 0) { 100.0 * $best / ($w - $leftMargin) / ($h - $topMargin) } else { 0 }
        }
        else {
            $bgSource = 'manual'
            $bgR = $background[0]; $bgG = $background[1]; $bgB = $background[2]
            $bgCoverage = -1
        }

        # Pass 2: content bounding box, centre brightness and the "colored" count.
        $minX = [int]::MaxValue; $minY = [int]::MaxValue; $maxX = -1; $maxY = -1
        $content = 0; $colored = 0
        $sumR = 0; $sumG = 0; $sumB = 0; $centerCount = 0
        $cx0 = [int]($w * 0.25); $cx1 = [int]($w * 0.75)
        $cy0 = [int]($h * 0.25); $cy1 = [int]($h * 0.75)

        for ($y = 0; $y -lt $h; $y++) {
            $row = $y * $stride
            for ($x = 0; $x -lt $w; $x++) {
                $i = $row + ($x * 4)
                $b = $bytes[$i]; $g2 = $bytes[$i + 1]; $r = $bytes[$i + 2]

                if ($x -ge $cx0 -and $x -lt $cx1 -and $y -ge $cy0 -and $y -lt $cy1) {
                    $sumR += $r; $sumG += $g2; $sumB += $b; $centerCount++
                }

                if ($x -lt $leftMargin -or $y -lt $topMargin) { continue }
                $considered++

                $max = [Math]::Max($r, [Math]::Max($g2, $b))
                $min = [Math]::Min($r, [Math]::Min($g2, $b))
                if (($max - $min) -gt 48) { $colored++ }

                $diff = [Math]::Max([Math]::Abs($r - $bgR), [Math]::Max([Math]::Abs($g2 - $bgG), [Math]::Abs($b - $bgB)))
                if ($diff -gt $tolerance) {
                    $content++
                    if ($x -lt $minX) { $minX = $x }
                    if ($x -gt $maxX) { $maxX = $x }
                    if ($y -lt $minY) { $minY = $y }
                    if ($y -gt $maxY) { $maxY = $y }
                }
            }
        }

        $contentPercent = if ($considered -gt 0) { 100.0 * $content / $considered } else { 0 }
        $coloredPercent = if ($considered -gt 0) { 100.0 * $colored / $considered } else { 0 }
        $bbox = if ($maxX -ge 0) { '({0},{1},{2},{3})' -f $minX, $minY, ($maxX - $minX + 1), ($maxY - $minY + 1) } else { 'none' }
        $mean = if ($centerCount -gt 0) {
            '({0},{1},{2})' -f [int]($sumR / $centerCount), [int]($sumG / $centerCount), [int]($sumB / $centerCount)
        } else { 'n/a' }
        $bgText = '({0},{1},{2})' -f $bgR, $bgG, $bgB

        return [pscustomobject]@{
            Width = $w; Height = $h
            ContentPercent = [Math]::Round($contentPercent, 2)
            ColoredPercent = [Math]::Round($coloredPercent, 2)
            CenterMean = $mean
            BoundingBox = $bbox
            Background = $bgText
            BackgroundSource = $bgSource
            BackgroundCoverage = [Math]::Round($bgCoverage, 1)
        }
    }
    finally { $src.Dispose() }
}

# ------------------------------------------------------------------- main ----

# The body returns an exit code instead of calling `exit` directly: `exit` inside a try
# block is swallowed by the catch block in Windows PowerShell, which would report the
# wrong code (1 instead of 2).
function Invoke-Verification {
    $bgColor = if ($Background) { Parse-Color $Background } else { $null }

    if ($AnalyzeOnly) {
        if (-not (Test-Path $AnalyzeOnly)) { throw "Image not found: $AnalyzeOnly" }
        $imagePath = (Resolve-Path $AnalyzeOnly).Path
        $stats = Get-ImageStats $imagePath $LeftMargin $TopMargin $bgColor $ColorTolerance

        Write-Host ("IMAGE      : {0}" -f $imagePath)
        Write-Host ("STATS      : content={0}% bg={1}({2}) bgCov={3}% colored={4}% centerMean={5} bbox={6}" -f `
                $stats.ContentPercent, $stats.Background, $stats.BackgroundSource, $stats.BackgroundCoverage, `
                $stats.ColoredPercent, $stats.CenterMean, $stats.BoundingBox)
        if ($stats.ContentPercent -ge $MinContentPercent -or $NoAssert) {
            Write-Host 'RESULT     : PASS'
            return 0
        }
        Write-Host ("RESULT     : FAIL (content {0}% < threshold {1}%)" -f $stats.ContentPercent, $MinContentPercent)
        return 2
    }

    if ($Build) { Invoke-PresetBuild $Preset $BuildTarget }

    $exePath = Resolve-ExePath $Preset $Exe $BuildTarget
    $workDir = Split-Path -Parent $exePath
    if (-not (Test-Path $OutDir)) { New-Item -ItemType Directory -Path $OutDir -Force | Out-Null }

    $tag = if ($NoScene) { Split-Path -Leaf $workDir } else { '{0}_scene{1}' -f (Split-Path -Leaf $workDir), $Scene }
    $pngPath = Join-Path $OutDir "$tag.png"
    $outPath = Join-Path $OutDir "$tag.stdout.txt"
    $errPath = Join-Path $OutDir "$tag.stderr.txt"
    foreach ($f in @($pngPath, $outPath, $errPath)) { if (Test-Path $f) { Remove-Item $f -Force } }

    $argList = @()
    if (-not $NoScene) { $argList += ('-scene={0}' -f $Scene) }

    Write-Host ("EXE        : {0}" -f $exePath)
    Write-Host ("ARGS       : {0}" -f (($argList -join ' ') -replace '^$', '<none>'))

    # The app resolves res/ relative to its working directory, so it MUST start in the
    # folder that contains UnitTests.exe.
    $startArgs = @{
        FilePath               = $exePath
        WorkingDirectory       = $workDir
        RedirectStandardOutput = $outPath
        RedirectStandardError  = $errPath
        PassThru               = $true
    }
    if ($argList.Count -gt 0) { $startArgs['ArgumentList'] = $argList }
    $proc = Start-Process @startArgs

    try {
        $hwnd = [IntPtr]::Zero
        $deadline = (Get-Date).AddMilliseconds($WaitMs)
        while ((Get-Date) -lt $deadline) {
            if ($proc.HasExited) { break }
            $hwnd = if ($WindowTitle) {
                [GfWin32]::FindByClassAndTitle([uint32]$proc.Id, $WindowClass, $WindowTitle)
            }
            else {
                [GfWin32]::FindByClass([uint32]$proc.Id, $WindowClass)
            }
            if ($hwnd -ne [IntPtr]::Zero) { break }
            Start-Sleep -Milliseconds 200
        }

        if ($proc.HasExited) {
            $code = $proc.ExitCode
            $hex = '0x{0:X8}' -f ($code -band 0xFFFFFFFF)
            Write-Host ("PROCESS    : exited early, exit code {0} ({1})" -f $code, $hex)
            if (Test-Path $outPath) { Write-Host '---- last stdout lines ----'; Get-Content $outPath -Tail 25 }
            if (Test-Path $errPath) { Write-Host '---- last stderr lines ----'; Get-Content $errPath -Tail 25 }
            Write-Host ('RESULT     : FAIL (process died; 0xC0000094/3221225596 = integer divide by zero,' +
                ' 0xC0000005/3221225477 = access violation)')
            return 2
        }

        if ($hwnd -eq [IntPtr]::Zero) {
            # A backend that never creates an SDL window would land here; fall back to the
            # largest visible window so the failure is still captured for inspection.
            $hwnd = [GfWin32]::FindLargest([uint32]$proc.Id)
        }
        if ($hwnd -eq [IntPtr]::Zero) {
            throw "No window of class '$WindowClass' within $WaitMs ms. Check that the app created its window and did not block during init."
        }

        Start-Sleep -Milliseconds $SettleMs
        [void][GfWin32]::SetForegroundWindow($hwnd)
        Start-Sleep -Milliseconds 400

        $size = Save-WindowPng $hwnd $pngPath
        $stats = Get-ImageStats $pngPath $LeftMargin $TopMargin $bgColor $ColorTolerance

        Write-Host ("PROCESS    : running, window {0}x{1}" -f $size.Width, $size.Height)
        Write-Host ("PNG        : {0}" -f $pngPath)
        Write-Host ("STDOUT     : {0}" -f $outPath)
        Write-Host ("STDERR     : {0}" -f $errPath)
        Write-Host ("STATS      : content={0}% bg={1}({2}) bgCov={3}% colored={4}% centerMean={5} bbox={6}" -f `
                $stats.ContentPercent, $stats.Background, $stats.BackgroundSource, $stats.BackgroundCoverage, `
                $stats.ColoredPercent, $stats.CenterMean, $stats.BoundingBox)

        # Vulkan validation messages are written to std::cout by DeviceVK.cpp's debug
        # messenger, so they land in the redirected stdout file.
        $problems = @()
        foreach ($log in @($outPath, $errPath)) {
            if (Test-Path $log) {
                $problems += Select-String -Path $log -Pattern 'Vulkan ERROR|Vulkan WARNING|VUID-|Validation Error|Failed to load|Shader compilation failed' -ErrorAction SilentlyContinue
            }
        }
        $problems = @($problems | Where-Object { $_ })
        if ($problems.Count -gt 0) {
            Write-Host ("VALIDATION : {0} suspicious line(s), first 10:" -f $problems.Count)
            $problems | Select-Object -First 10 | ForEach-Object { Write-Host ("             {0}" -f $_.Line.Trim()) }
        }
        else {
            Write-Host 'VALIDATION : no errors in stdout/stderr'
        }

        $drew = $stats.ContentPercent -ge $MinContentPercent
        if (-not $drew -and -not $NoAssert) {
            Write-Host ("RESULT     : FAIL (content {0}% < threshold {1}%; only the clear colour / ImGui panels were captured)" -f $stats.ContentPercent, $MinContentPercent)
            return 2
        }
        if ($problems.Count -gt 0) {
            Write-Host 'RESULT     : PASS with warnings (image has content; check the validation lines above)'
        }
        elseif (-not $drew) {
            Write-Host 'RESULT     : NO CONTENT (assertion disabled with -NoAssert)'
        }
        else {
            Write-Host 'RESULT     : PASS'
        }
        return 0
    }
    finally {
        if ($proc -and -not $proc.HasExited) {
            $proc.Kill()
            $proc.WaitForExit(5000) | Out-Null
        }
    }
}

$exitCode = 1
try {
    $exitCode = Invoke-Verification
}
catch {
    Write-Host ("ERROR      : {0}" -f $_.Exception.Message)
    $exitCode = 1
}
exit $exitCode
