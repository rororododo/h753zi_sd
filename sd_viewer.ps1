# STM32 MicroSD Packet Real-time Viewer
$portName = "COM3"
$baudRate = 115200

$port = New-Object System.IO.Ports.SerialPort($portName, $baudRate, [System.IO.Ports.Parity]::None, 8, [System.IO.Ports.StopBits]::One)
$port.DtrEnable = $true
$port.RtsEnable = $true
$port.ReadTimeout = 50
$port.WriteTimeout = 500

try {
    $port.Open()
} catch {
    Write-Host ""
    Write-Host "[ERROR] Failed to open $portName. Please check if another program is using it." -ForegroundColor Red
    exit 1
}

# 24-bit RGB TrueColor ANSI 코드 (테마 설정과 상관없이 정확한 진짜 보라색 강제 출력)
$ESC = [char]27
$cPurple = "$ESC[38;2;220;100;255m"   # 선명한 보라색 (Vivid Purple)
$cWhite  = "$ESC[38;2;255;255;255m"   # 흰색 (White)
$cCyan   = "$ESC[38;2;0;220;255m"     # 하늘색 (Cyan)
$cGray   = "$ESC[38;2;160;160;160m"   # 차분한 회색 (Gray)
$cReset  = "$ESC[0m"

[Console]::WriteLine("==========================================================")
[Console]::WriteLine(" [STM32 MicroSD Packet Viewer] Port: $portName @ $baudRate")
[Console]::WriteLine("  * [LIVE STREAM] : Real-time SD Card writes appear here!")
[Console]::WriteLine("  * [r] key       : Dump CURRENT session file from SD Card")
[Console]::WriteLine("  * [p] key       : Dump PREVIOUS session file from SD Card")
[Console]::WriteLine("  * [q] key       : Quit / Exit")
[Console]::WriteLine("==========================================================")

$buffer = ""

try {
    while ($true) {
        # 1. 시리얼 수신 및 라인 단위 색상/화살표 가공 출력
        try {
            if ($port.BytesToRead -gt 0) {
                $incoming = $port.ReadExisting()
                $buffer += $incoming

                while ($buffer.Contains("`n")) {
                    $splitPos = $buffer.IndexOf("`n")
                    $rawLine = $buffer.Substring(0, $splitPos).Trim("`r")
                    $buffer = $buffer.Substring($splitPos + 1)

                    # 화살표 '->' 제거 및 Payload -> Message 교체
                    $cleanLine = $rawLine -replace '^\s*->\s*', '    ' -replace '\s*->\s*', ' '
                    $cleanLine = $cleanLine -replace 'Payload Preview:', 'Message Preview:'

                    # 1) [ETH RX ...] 라인은 흰색(White)
                    if ($cleanLine -match '\[ETH RX') {
                        [Console]::WriteLine("$cWhite$cleanLine$cReset")
                    }
                    # 2) 사용자가 직접 보낸 채팅 메시지("23", "456" 등)만 보라색(Purple)!
                    elseif ($cleanLine -match 'Message Preview' -and $cleanLine -notmatch 'Temp:' -and $cleanLine -notmatch '\[DATA') {
                        [Console]::WriteLine("$cPurple$cleanLine$cReset")
                    }
                    # 3) 오토센서 데이터(Temp:, Humidity 등)는 이전 색상(하늘색 Cyan)으로 되돌림!
                    elseif ($cleanLine -match 'Temp:' -or $cleanLine -match '\[DATA') {
                        [Console]::WriteLine("$cCyan$cleanLine$cReset")
                    }
                    # 4) 세션 구분선 배너는 하늘색(Cyan)
                    elseif ($cleanLine -match '===|====') {
                        [Console]::WriteLine("$cCyan$cleanLine$cReset")
                    }
                    # 5) 기타 내용(SD CARD Appended, Heartbeat 등)은 차분한 회색(Gray)
                    else {
                        [Console]::WriteLine("$cGray$cleanLine$cReset")
                    }
                }
            }
        } catch { }

        # 2. 키보드 입력 감지 (p: 이전 파일, r: 현재 파일, q: 종료)
        $hasKey = $false
        try {
            if ([Console]::KeyAvailable) { $hasKey = $true }
        } catch {
            try {
                if ($Host.UI.RawUI.KeyAvailable) { $hasKey = $true }
            } catch { }
        }

        if ($hasKey) {
            $key = $Host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown")
            $ch = $key.Character

            if ($ch -eq 'p' -or $ch -eq 'P') {
                $port.Write("p")
                [Console]::WriteLine("`n>>> [KEY 'p'] Requesting PREVIOUS file dump from SD Card...")
            }
            elseif ($ch -eq 'r' -or $ch -eq 'R') {
                $port.Write("r")
                [Console]::WriteLine("`n>>> [KEY 'r'] Requesting CURRENT file dump from SD Card...")
            }
            elseif ($ch -eq 'q' -or $ch -eq 'Q') {
                [Console]::WriteLine("`n[EXIT] Exiting SD Card Viewer.")
                break
            }
        }

        Start-Sleep -Milliseconds 10
    }
} finally {
    if ($port.IsOpen) {
        $port.Close()
    }
    [Console]::WriteLine("`n[CLOSED] Serial port $portName closed cleanly.`n")
}
