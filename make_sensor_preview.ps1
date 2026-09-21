Add-Type -AssemblyName System.Drawing

$out = 'D:\shyeh 2026\ESP32\36_epaper\sensor_pages_preview.png'
$pageW = [int]296
$pageH = [int]128
$canvas = [System.Drawing.Bitmap]::new([int]($pageW * 3), $pageH)
$g = [System.Drawing.Graphics]::FromImage($canvas)
$g.Clear([System.Drawing.Color]::White)
$g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias

$red = [System.Drawing.Color]::FromArgb(190, 30, 30)
$black = [System.Drawing.Color]::Black
$gray = [System.Drawing.Color]::FromArgb(120, 120, 120)
$fontTitle = New-Object System.Drawing.Font('Arial', 21, [System.Drawing.FontStyle]::Bold)
$fontValue = New-Object System.Drawing.Font('Arial', 27, [System.Drawing.FontStyle]::Bold)
$fontSmall = New-Object System.Drawing.Font('Arial', 9, [System.Drawing.FontStyle]::Regular)
$penBorder = New-Object System.Drawing.Pen($gray, 1)
$penRed = New-Object System.Drawing.Pen($red, 3)
$brushRed = New-Object System.Drawing.SolidBrush($red)
$brushBlack = New-Object System.Drawing.SolidBrush($black)
$brushGray = New-Object System.Drawing.SolidBrush($gray)

function Draw-Thermometer($x) {
  $g.DrawEllipse($penRed, $x + 35, 68, 20, 20)
  $g.FillEllipse($brushRed, $x + 39, 72, 12, 12)
  $g.DrawRectangle($penRed, $x + 41, 28, 8, 46)
  $g.FillRectangle($brushRed, $x + 43, 45, 4, 30)
}

function Draw-Drop($x) {
  $g.DrawEllipse($penRed, $x + 27, 45, 36, 45)
  $g.FillEllipse($brushRed, $x + 30, 48, 30, 39)
  $g.DrawLine($penRed, $x + 45, 25, $x + 27, 57)
  $g.DrawLine($penRed, $x + 45, 25, $x + 63, 57)
}

function Draw-Sun($x) {
  $g.FillEllipse($brushRed, $x + 32, 42, 27, 27)
  for ($i = 0; $i -lt 8; $i++) {
    $a = $i * [math]::PI / 4
    $x0 = $x + 45 + [int]([math]::Cos($a) * 22)
    $y0 = 55 + [int]([math]::Sin($a) * 22)
    $x1 = $x + 45 + [int]([math]::Cos($a) * 32)
    $y1 = 55 + [int]([math]::Sin($a) * 32)
    $g.DrawLine($penRed, $x0, $y0, $x1, $y1)
  }
}

$pages = @(
  @{ title = 'TEMP'; value = '27°C'; draw = ${function:Draw-Thermometer} },
  @{ title = 'HUMI'; value = '60%'; draw = ${function:Draw-Drop} },
  @{ title = 'LIGHT'; value = '75%'; draw = ${function:Draw-Sun} }
)

for ($i = 0; $i -lt 3; $i++) {
  $x = [int]($i * $pageW)
  $g.DrawRectangle($penBorder, $x + 1, 1, $pageW - 3, $pageH - 3)
  $g.DrawString($pages[$i].title, $fontTitle, $brushRed, $x + 104, 15)
  if ($i -eq 0) {
    Draw-Thermometer $x
  } elseif ($i -eq 1) {
    Draw-Drop $x
  } else {
    Draw-Sun $x
  }
  $g.DrawString($pages[$i].value, $fontValue, $brushBlack, $x + 102, 73)
  $g.DrawString("ESP32 e-Paper sensor page", $fontSmall, $brushGray, $x + 80, 112)
}

$canvas.Save($out, [System.Drawing.Imaging.ImageFormat]::Png)
$g.Dispose()
$canvas.Dispose()
Write-Output $out
