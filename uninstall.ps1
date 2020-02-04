$output = (Invoke-Expression '.\devcon.exe dp_enum') | out-string

if ($output -match "(?m)^(?:(oem\d+\.inf)\r\n +Provider: Fabrikam Inc\.\r\n +Class: Extensions\r\n)?(oem\d+\.inf)\r\n +Provider: Fabrikam Inc\.\r\n +Class: Sound, video and game controllers") {

  $drivers = @()

  $matches.Keys | ForEach-Object { if ($_ -ne '0') {
      $drivers += $matches[$_]
    } }

  if ((Invoke-Expression ".\devcon.exe stack ''USB\ROOT_HUB30" | out-string) -match "Mixman_Filter") {
    Invoke-Expression ".\devcon.exe remove ''USB\ROOT_HUB30"
    $rescan = $true
  }

  $drivers | ForEach-Object { Invoke-Expression ".\devcon.exe -f dp_delete $_" }

  if ($rescan) {
    Invoke-Expression ".\devcon.exe rescan" 
  }
}
else {
  Write-Error "No driver found"
}