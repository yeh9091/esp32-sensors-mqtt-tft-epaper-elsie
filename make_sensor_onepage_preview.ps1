Add-Type -AssemblyName System.Drawing

$out = 'D:\shyeh 2026\ESP32\36_epaper\sensor_onepage_preview.png'
$w = 296
$h = 128
$bmp = [System.Drawing.Bitmap]::new($w, $h)
$g = [System.Drawing.Graphics]::FromImage($bmp)
$g.Clear([System.Drawing.Color]::White)
$g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias

$red = [System.Drawing.Color]::FromArgb(190, 30, 30)
$black = [System.Drawing.Color]::Black
$gray = [System.Drawing.Color]::FromArgb(120, 120, 120)
$brushRed = [System.Drawing.SolidBrush]::new($red)
$brushBlack = [System.Drawing.SolidBrush]::new($black)
$brushGray = [System.Drawing.SolidBrush]::new($gray)
$penRed = [System.Drawing.Pen]::new($red, 2)
$penGray = [System.Drawing.Pen]::new($gray, 1)
$fontLabel = [System.Drawing.Font]::new('Arial', 12, [System.Drawing.FontStyle]::Bold)
$fontValue = [System.Drawing.Font]::new('Arial', 16, [System.Drawing.FontStyle]::Bold)

$g.DrawRectangle($penGray, 1, 1, $w - 3, $h - 3)
$g.DrawLine($penGray, 98, 12, 98, 116)
$g.DrawLine($penGray, 197, 12, 197, 116)

# Thermometer: outlined tube, bulb, scale marks and red liquid.
$g.DrawString('TEMP', $fontLabel, $brushRed, 34, 12)
$g.DrawEllipse($penRed, 39, 77, 26, 26)
$g.FillEllipse($brushRed, 45, 83, 14, 14)
$g.DrawRectangle($penRed, 45, 38, 14, 48)
$g.FillRectangle($brushRed, 50, 55, 4, 31)
for ($y = 43; $y -le 73; $y += 10) {
  $g.DrawLine($penRed, 37, $y, 44, $y)
}
$g.DrawString('27°C', $fontValue, $brushBlack, 24, 100)

# Water drop: pointed top, rounded bottom and a small white highlight.
$g.DrawString('HUMI', $fontLabel, $brushRed, 131, 12)
$dropPoints = [System.Drawing.Point[]]@(
  [System.Drawing.Point]::new(149, 35),
  [System.Drawing.Point]::new(128, 67),
  [System.Drawing.Point]::new(130, 86),
  [System.Drawing.Point]::new(142, 98),
  [System.Drawing.Point]::new(156, 98),
  [System.Drawing.Point]::new(168, 86),
  [System.Drawing.Point]::new(170, 67)
)
$g.FillPolygon([System.Drawing.Brush]([System.Drawing.SolidBrush]::new([System.Drawing.Color]::White)), $dropPoints)
$g.DrawPolygon([System.Drawing.Pen]$penRed, $dropPoints)
$innerDrop = [System.Drawing.Point[]]@(
  [System.Drawing.Point]::new(149, 43),
  [System.Drawing.Point]::new(136, 68),
  [System.Drawing.Point]::new(137, 83),
  [System.Drawing.Point]::new(146, 91),
  [System.Drawing.Point]::new(153, 91),
  [System.Drawing.Point]::new(162, 83),
  [System.Drawing.Point]::new(163, 68)
)
$g.DrawPolygon([System.Drawing.Pen]$penRed, $innerDrop)
$g.DrawLine($penRed, 144, 70, 153, 70)
$g.DrawLine($penRed, 142, 76, 155, 76)
$g.FillEllipse([System.Drawing.Brush]$brushRed, 143, 55, 6, 10)
$g.DrawString('60%', $fontValue, $brushBlack, 128, 100)

# Sun
$g.DrawString('LIGHT', $fontLabel, $brushRed, 226, 12)
$g.FillEllipse($brushRed, 237, 57, 22, 22)
for ($i = 0; $i -lt 8; $i++) {
  $a = $i * [math]::PI / 4
  $x0 = 248 + [int]([math]::Cos($a) * 19)
  $y0 = 68 + [int]([math]::Sin($a) * 19)
  $x1 = 248 + [int]([math]::Cos($a) * 29)
  $y1 = 68 + [int]([math]::Sin($a) * 29)
  $g.DrawLine($penRed, $x0, $y0, $x1, $y1)
}
$g.DrawString('75%', $fontValue, $brushBlack, 226, 100)

$bmp.Save($out, [System.Drawing.Imaging.ImageFormat]::Png)
$g.Dispose()
$bmp.Dispose()
Write-Output $out
