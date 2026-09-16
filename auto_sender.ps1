# 1초에 1번씩 자동으로 STM32(192.168.0.50:8080)로 센서 패킷을 전송하는 스크립트
# 중지하고 싶을 때는 키보드의 [Ctrl + C] 를 누르시면 됩니다.

$targetIP = "192.168.0.50"
$targetPort = 8080

Write-Host "==========================================================" -ForegroundColor Cyan
Write-Host " [AUTO SENDER] 1 sec interval -> STM32 (${targetIP}:${targetPort})" -ForegroundColor Cyan
Write-Host " Stop sending: Press [Ctrl + C]" -ForegroundColor Yellow
Write-Host "==========================================================" -ForegroundColor Cyan

$udpClient = New-Object System.Net.Sockets.UdpClient
$count = 0

try {
    while ($true) {
        $count++
        $timestamp = (Get-Date).ToString("yyyy-MM-dd HH:mm:ss")
        $message = "[$timestamp] [DATA #$count] Temp: 24.5C | Humidity: 55.2% | Status: OK"
        
        $bytes = [System.Text.Encoding]::UTF8.GetBytes($message)
        [void]$udpClient.Send($bytes, $bytes.Length, $targetIP, $targetPort)
        
        Write-Host "[$timestamp] Sent (#$count) -> STM32 (${targetIP}:${targetPort})" -ForegroundColor Green
        Start-Sleep -Seconds 1
    }
} finally {
    $udpClient.Close()
    Write-Host "`n[STOPPED] Auto sender stopped." -ForegroundColor Yellow
}
