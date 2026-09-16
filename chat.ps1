# STM32 UDP Interactive Messenger (채팅형 송신기)
$targetIP = "192.168.0.50"
$targetPort = 8080

Write-Host "==========================================================" -ForegroundColor Cyan
Write-Host " [STM32 UDP Chat] Destination: ${targetIP}:${targetPort}" -ForegroundColor Cyan
Write-Host " Type your message and press [Enter] to send." -ForegroundColor Yellow
Write-Host " Type 'exit' or 'q' to quit." -ForegroundColor Gray
Write-Host "==========================================================" -ForegroundColor Cyan

$udpClient = New-Object System.Net.Sockets.UdpClient
$count = 0

try {
    while ($true) {
        $msg = Read-Host "Message to STM32"
        
        if ([string]::IsNullOrWhiteSpace($msg)) {
            continue
        }
        
        if ($msg -eq "exit" -or $msg -eq "q") {
            break
        }
        
        $count++
        $bytes = [System.Text.Encoding]::UTF8.GetBytes($msg)
        [void]$udpClient.Send($bytes, $bytes.Length, $targetIP, $targetPort)
        
        $timestamp = (Get-Date).ToString("HH:mm:ss")
        Write-Host " -> [SENT #$count at $timestamp] `"$msg`" -> Saved to MicroSD!" -ForegroundColor Green
    }
} finally {
    $udpClient.Close()
    Write-Host "`n[CLOSED] Chat messenger closed." -ForegroundColor Yellow
}
