# Manual capture and interaction

Use this when you want to inspect a window that is **already running** (for example after
launching the app yourself, or after pausing it in a debugger). For normal verification prefer
`scripts/verify-scene.ps1`, which does all of this plus statistics and log capture.

## 1. Launch the app by hand

```powershell
$exe = "E:\GraphicsFramework\build\ninja-multi\GraphicsFramework_Binary\Debug_Vulkan\UnitTests.exe"
Start-Process -FilePath $exe -WorkingDirectory (Split-Path $exe) -ArgumentList '-scene=4'
```

* `-WorkingDirectory` is mandatory: `res/` (settings, shaders, `spirv/`, textures) is resolved
  relative to the current working directory.
* Omit `-scene=<n>` to start on `ShaderTest` and use the **Projects → Select Unit Test** combo.
* Launched this way from a PowerShell session, `std::cout` (including Vulkan validation messages)
  appears in that session. From a double-click it goes to a console window that disappears with
  the app, which is why the script redirects it to a file instead.

## 2. Capture the running window

```powershell
Add-Type -AssemblyName System.Drawing
$sig = @"
using System;
using System.Runtime.InteropServices;
public class Capture {
    [DllImport("user32.dll", CharSet = CharSet.Unicode)]
    public static extern IntPtr FindWindow(string cls, string title);
    [DllImport("user32.dll")]
    public static extern bool PrintWindow(IntPtr hwnd, IntPtr hdc, uint flags);
    [DllImport("user32.dll")]
    public static extern bool GetWindowRect(IntPtr hwnd, out RECT r);
    [StructLayout(LayoutKind.Sequential)]
    public struct RECT { public int L, T, R, B; }
}
"@
Add-Type -TypeDefinition $sig

$hwnd = [Capture]::FindWindow('SDL_app', 'Graphics Framework')
if ($hwnd -eq [IntPtr]::Zero) { throw 'window not found' }

$r = New-Object Capture+RECT
[void][Capture]::GetWindowRect($hwnd, [ref]$r)
$w = $r.R - $r.L; $h = $r.B - $r.T

$bmp = New-Object System.Drawing.Bitmap($w, $h)
$g = [System.Drawing.Graphics]::FromImage($bmp)
$hdc = $g.GetHdc()
# flags = 2 is PW_RENDERFULLCONTENT: required, otherwise the GPU-rendered client area is black
[void][Capture]::PrintWindow($hwnd, $hdc, 2)
$g.ReleaseHdc($hdc); $g.Dispose()
$bmp.Save("$env:TEMP\gf_manual.png", [System.Drawing.Imaging.ImageFormat]::Png)
$bmp.Dispose()
```

Then read `$env:TEMP\gf_manual.png` with the `read` tool.

Why each part matters:

| Detail | Reason |
|---|---|
| class `SDL_app` + title `Graphics Framework` | the process also owns a console window; `MainWindowHandle` may return that one |
| `PrintWindow(..., 2)` | `PW_RENDERFULLCONTENT`; flag `0` captures only the non-client area of a GPU-rendered window |
| `GetWindowRect` first | the bitmap must match the window size, otherwise the image is cropped |
| `Graphics.FromImage` + `GetHdc` | `PrintWindow` needs a device context, not a `Bitmap` |

## 3. Capture a screenshot of a window that is mid-frame

`PrintWindow` asks the window to repaint into the supplied DC. Vulkan presents asynchronously, so
a capture taken immediately after a state change can show the previous frame. Wait ~500–1500 ms
after any interaction before capturing; `verify-scene.ps1` uses `-SettleMs` for exactly this.

## 4. Drive the ImGui UI without a mouse

The scenes are also selectable at startup, which is the supported way to script them:

```powershell
Start-Process -FilePath $exe -WorkingDirectory (Split-Path $exe) -ArgumentList '-scene=3'
```

If you must click (e.g. to test the combo), use `SetForegroundWindow` and send mouse input with
`SendInput`/`SetCursorPos`; do not use `SendKeys`, ImGui has no keyboard focus model for the
combo. Prefer `-scene`.

## 5. Tear down

```powershell
Get-Process UnitTests -ErrorAction SilentlyContinue | Stop-Process -Force
```

The verification script kills the process it started, but a manual run stays open until closed.
Leaving stale instances running makes the next `FindWindow` pick the wrong window.

## 6. If the capture is black even with `PrintWindow`

1. Confirm the window is the SDL one: it must be 1296x759-ish and titled `Graphics Framework`,
   not `…\UnitTests.exe`.
2. Confirm the process is still alive (`Get-Process UnitTests`) — a crashed app leaves a
   ghost window.
3. Confirm the scene actually renders: run `scripts/verify-scene.ps1` for the same scene; if its
   `content` is 0 %, the bug is in the renderer, not in the capture.
4. On a remote/RDP session, DWM composition can be unavailable. Log in to the physical console or
   run the app and the capture in the same session.
