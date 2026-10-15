param(
    [Parameter(Mandatory=$true)][string]$TargetPath,
    [Parameter(Mandatory=$true)][string]$LinkName,
    [Parameter(Mandatory=$true)][string]$WorkingDirectory,
    [string]$DesktopPath,
    [string]$IconPath
)
$ErrorActionPreference = 'Stop'
[Console]::OutputEncoding = New-Object System.Text.UTF8Encoding($false)
$temporaryLink = $null
$shell = $null
$shortcut = $null
try {
    if (-not [System.IO.File]::Exists($TargetPath)) { throw 'The committed launcher executable is missing.' }
    if ([string]::IsNullOrWhiteSpace($LinkName) -or $LinkName.IndexOfAny([System.IO.Path]::GetInvalidFileNameChars()) -ge 0 -or $LinkName.EndsWith('.') -or $LinkName.EndsWith(' ')) { throw 'Invalid shortcut name.' }
    if ([string]::IsNullOrEmpty($DesktopPath)) { $DesktopPath = [Environment]::GetFolderPath('Desktop') }
    if (-not [System.IO.Directory]::Exists($DesktopPath)) { throw 'The Desktop folder does not exist.' }
    $finalLink = [System.IO.Path]::Combine($DesktopPath, $LinkName + '.lnk')
    if ([System.IO.File]::Exists($finalLink) -or [System.IO.Directory]::Exists($finalLink)) { throw 'The Desktop shortcut already exists; it will not be overwritten.' }
    $temporaryLink = [System.IO.Path]::Combine($DesktopPath, '.fm5-' + [Guid]::NewGuid().ToString('N') + '.lnk')
    $shell = New-Object -ComObject WScript.Shell
    $shortcut = $shell.CreateShortcut($temporaryLink)
    $shortcut.TargetPath = $TargetPath
    $shortcut.WorkingDirectory = $WorkingDirectory
    $shortcut.Description = 'RaceOne5'
    if (-not [string]::IsNullOrEmpty($IconPath)) { $shortcut.IconLocation = $IconPath + ',0' }
    $shortcut.Save()
    # This overload never replaces an existing destination, even if one was
    # created since the earlier check. The new link is published only at commit.
    [System.IO.File]::Move($temporaryLink, $finalLink)
    $temporaryLink = $null
    [Console]::WriteLine($finalLink)
} catch {
    [Console]::Error.WriteLine($_.Exception.Message)
    exit 1
} finally {
    if ($null -ne $shortcut) { [void][Runtime.InteropServices.Marshal]::FinalReleaseComObject($shortcut) }
    if ($null -ne $shell) { [void][Runtime.InteropServices.Marshal]::FinalReleaseComObject($shell) }
    if ($null -ne $temporaryLink -and [System.IO.File]::Exists($temporaryLink)) { [System.IO.File]::Delete($temporaryLink) }
}
