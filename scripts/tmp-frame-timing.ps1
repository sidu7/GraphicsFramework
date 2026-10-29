param(
    [Parameter(Mandatory=$true)][string]$Exe,
    [int]$Seconds = 6
)

Add-Type @"
using System;
using System.Text;
using System.Runtime.InteropServices;
public static class Win32Run {
    [DllImport("user32.dll")] public static extern bool EnumWindows(EnumWindowsProc lpEnumFunc, IntPtr lParam);
    public delegate bool EnumWindowsProc(IntPtr hWnd, IntPtr lParam);
    [DllImport("user32.dll")] public static extern int GetClassName(IntPtr hWnd, StringBuilder lpClassName, int nMaxCount);
    [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr hWnd, uint Msg, IntPtr wParam, IntPtr lParam);
    [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr hWnd, out uint pid);
}
"@

$psi = New-Object System.Diagnostics.ProcessStartInfo
$psi.FileName = $Exe
$psi.WorkingDirectory = (Split-Path -Parent $Exe)
$psi.UseShellExecute = $false
$psi.RedirectStandardOutput = $true
$psi.RedirectStandardError = $true
$p = [System.Diagnostics.Process]::Start($psi)
$script:targetPid = [uint32]$p.Id
$script:found = [IntPtr]::Zero

$stdoutTask = $p.StandardOutput.ReadToEndAsync()
$stderrTask = $p.StandardError.ReadToEndAsync()

Start-Sleep -Seconds $Seconds

$cb = [Win32Run+EnumWindowsProc]{
    param($h, $l)
    $sb = New-Object System.Text.StringBuilder 256
    [void][Win32Run]::GetClassName($h, $sb, 256)
    if ($sb.ToString() -eq 'SDL_app') {
        [uint32]$wpid = 0
        [void][Win32Run]::GetWindowThreadProcessId($h, [ref]$wpid)
        if ($wpid -eq $script:targetPid) { $script:found = $h; return $false }
    }
    return $true
}
[void][Win32Run]::EnumWindows($cb, [IntPtr]::Zero)

if ($script:found -ne [IntPtr]::Zero) {
    [void][Win32Run]::PostMessage($script:found, 0x0010, [IntPtr]::Zero, [IntPtr]::Zero)
}

if (-not $p.WaitForExit(15000)) { $p.Kill() }

$out = $stdoutTask.GetAwaiter().GetResult()
$err = $stderrTask.GetAwaiter().GetResult()
if ($out) { $out -split "`n" | Where-Object { $_.Trim() } | ForEach-Object { "OUT: $_" } }
if ($err) { $err -split "`n" | Where-Object { $_.Trim() } | ForEach-Object { "ERR: $_" } }
Write-Host ("EXIT: {0}" -f $p.ExitCode)
