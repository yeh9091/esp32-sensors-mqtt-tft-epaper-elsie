Add-Type -AssemblyName System.Drawing

$src = 'C:\Users\user\AppData\Local\Temp\codex-clipboard-68ef7d70-1aa4-4e6f-bde3-1d66525fd8a9.png'
$outPng = 'D:\shyeh 2026\ESP32\36_epaper\moon_rabbit_3color_preview.png'
$outH = 'D:\shyeh 2026\ESP32\36_epaper\moon_rabbit_3color.h'

$srcBmp = [System.Drawing.Bitmap]::FromFile($src)
$w = 180
$h = 104
$byteCount = [int](($w * $h + 3) / 4)

$bmp = New-Object System.Drawing.Bitmap($w, $h)
$g = [System.Drawing.Graphics]::FromImage($bmp)
$g.Clear([System.Drawing.Color]::White)
$g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
$g.DrawImage($srcBmp, 0, 0, $w, $h)
$g.Dispose()

$bytes = New-Object byte[] $byteCount
$codes = New-Object byte[] ($w * $h)
$preview = New-Object System.Drawing.Bitmap($w, $h)

function ColorCode([System.Drawing.Color]$c) {
  $r = $c.R
  $green = $c.G
  $b = $c.B
  $lum = 0.299 * $r + 0.587 * $green + 0.114 * $b
  if ($lum -lt 150) { return 1 } # stronger black outlines and eyes
  # Keep only the darker, saturated parts as red.  The bright golden
  # cake surface becomes white so the three-colour result stays legible.
  if ($lum -lt 190 -and (($r -gt ($green * 1.10)) -or ($r -gt ($b * 1.10)) -or ($b -gt ($green * 1.08)))) {
    return 2 # red
  }
  return 0 # white
}

for ($y = 0; $y -lt $h; $y++) {
  for ($x = 0; $x -lt $w; $x++) {
    $v = [int](ColorCode $bmp.GetPixel($x, $y))
    $codes[$y * $w + $x] = [byte]$v
  }
}

# Thicken black/red strokes by one pixel so they survive the e-paper
# panel's native resolution and remain readable after a refresh.
$thick = New-Object byte[] ($w * $h)
for ($y = 0; $y -lt $h; $y++) {
  for ($x = 0; $x -lt $w; $x++) {
    $v = [int]$codes[$y * $w + $x]
    if ($v -ne 0) {
      $thick[$y * $w + $x] = [byte]$v
    }
    for ($dy = -1; $dy -le 1; $dy++) {
      for ($dx = -1; $dx -le 1; $dx++) {
        $nx = $x + $dx
        $ny = $y + $dy
        if ($nx -ge 0 -and $nx -lt $w -and $ny -ge 0 -and $ny -lt $h) {
          $near = [int]$codes[$ny * $w + $nx]
          if ($near -eq 1 -or ($near -eq 2 -and $v -eq 0)) {
            if ($near -eq 1 -or $thick[$y * $w + $x] -eq 0) {
              $thick[$y * $w + $x] = [byte]$near
            }
          }
        }
      }
    }
  }
}

for ($y = 0; $y -lt $h; $y++) {
  for ($x = 0; $x -lt $w; $x++) {
    $v = [int]$thick[$y * $w + $x]
    $idx = [int]($y * $w + $x)
    $bi = [int][math]::Floor($idx / 4)
    $shift = [int]((3 - ($idx % 4)) * 2)
    $bytes[$bi] = [byte]($bytes[$bi] -bor [byte]($v -shl $shift))
    if ($v -eq 1) {
      $preview.SetPixel($x, $y, [Drawing.Color]::Black)
    } elseif ($v -eq 2) {
      $preview.SetPixel($x, $y, [Drawing.Color]::Red)
    } else {
      $preview.SetPixel($x, $y, [Drawing.Color]::White)
    }
  }
}

$preview.Save($outPng, [System.Drawing.Imaging.ImageFormat]::Png)

$lines = New-Object System.Collections.Generic.List[string]
[void]$lines.Add('#pragma once')
[void]$lines.Add('#include <Arduino.h>')
[void]$lines.Add("constexpr uint16_t MOON_RABBIT_W = $w;")
[void]$lines.Add("constexpr uint16_t MOON_RABBIT_H = $h;")
[void]$lines.Add('const uint8_t MOON_RABBIT_3COLOR[] PROGMEM = {')
for ($i = 0; $i -lt $bytes.Length; $i += 16) {
  $end = [math]::Min($i + 16, $bytes.Length) - 1
  $vals = for ($j = $i; $j -le $end; $j++) { ('0x{0:X2}' -f $bytes[$j]) }
  [void]$lines.Add('  ' + ($vals -join ', ') + ',')
}
[void]$lines.Add('};')
[IO.File]::WriteAllLines($outH, $lines)

$srcBmp.Dispose()
$bmp.Dispose()
$preview.Dispose()
Write-Output "created $byteCount bytes"
