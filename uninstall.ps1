$output = (Invoke-Expression '.\devcon.exe dp_enum') | out-string

$filter = $output | select-string -Pattern "(?m)^(oem\d+\.inf)\r\n +Provider: Fabrikam Inc\.\r\n +Class: Extensions" -AllMatches | select -ExpandProperty Matches | % {($_.Groups)[1].value}
$midi = $output | select-string -Pattern "(?m)^(oem\d+\.inf)\r\n +Provider: Fabrikam Inc\.\r\n +Class: Sound, video and game controllers" -AllMatches | select -ExpandProperty Matches | % {($_.Groups)[1].value}

if ($filter -or $midi) {

  if ((Invoke-Expression ".\devcon.exe stack ''USB\ROOT_HUB30" | out-string) -match "Mixman_Filter") {
    Invoke-Expression ".\devcon.exe remove ''USB\ROOT_HUB30"
    $rescan = $true
  }

  $filter + $midi | ForEach-Object { Invoke-Expression ".\devcon.exe -f dp_delete $_" }

  if ($rescan) {
    Invoke-Expression ".\devcon.exe rescan" 
  }
}
else {
  Write-Error "No driver found"
}