# STM32H753ZI Ethernet UDP to MicroSD 실시간 고속 무손실 로거

[![Target MCU](https://img.shields.io/badge/MCU-STM32H753ZIT6%20(ARM%20Cortex--M7%20400MHz)-002B49.svg?style=flat-square)](https://www.st.com/en/microcontrollers-microprocessors/stm32h753zi.html)
[![Board](https://img.shields.io/badge/Board-NUCLEO--H753ZI-03234B.svg?style=flat-square)](https://www.st.com/en/evaluation-tools/nucleo-h753zi.html)
[![Network](https://img.shields.io/badge/Ethernet-LAN8742A%20RMII%20(UDP%20IPv4)-blue.svg?style=flat-square)]()
[![Storage](https://img.shields.io/badge/Storage-MicroSD%20(SDMMC1%201--bit%20FatFs)-orange.svg?style=flat-square)]()
[![Memory](https://img.shields.io/badge/Memory-100%25%20Static%20Allocation%20(Zero--malloc)-green.svg?style=flat-square)]()

PC(노트북)에서 발생하는 1초 주기 고속 센서 패킷 및 대화형 사용자 텍스트 메시지를 랜선(Ethernet UDP)을 통해 STM32H753ZI로 초고속 전송하고, 보드가 이를 하드웨어 DMA로 수신하여 MicroSD 카드의 물리 플래시 영역(FatFs FAT32)에 단 1패킷의 유실도 없이 실시간 영구 기록하는 **고신뢰성 임베디드 데이터 로깅 시스템**입니다.

---

## 📌 목차
1. [프로젝트 핵심 요구사항 및 구현 특징](#1-프로젝트-핵심-요구사항-및-구현-특징)
2. [시스템 아키텍처 및 3대 통신 채널 구조](#2-시스템-아키텍처-및-3대-통신-채널-구조)
3. [하드웨어 연결 및 핀 매핑](#3-하드웨어-연결-및-핀-매핑)
4. [임베디드 메모리 및 펌웨어 핵심 설계](#4-임베디드-메모리-및-펌웨어-핵심-설계)
5. [빌드 및 플래시 가이드 (Build & Flash)](#5-빌드-및-플래시-가이드-build--flash)
6. [PC 테스트 스크립트 및 전용 콘솔 뷰어](#6-pc-테스트-스크립트-및-전용-콘솔-뷰어)
7. [프로젝트 디렉터리 구조](#7-프로젝트-디렉터리-구조)

---

## 1. 프로젝트 핵심 요구사항 및 구현 특징

본 프로젝트는 개발 과정에서 정의된 다음과 같은 엄격한 엔지니어링 요구사항들을 100% 충족하도록 구현되었습니다.

* **100% 정적 메모리 할당 (Zero-malloc 원칙):**
  * OS가 없는 Bare-metal 환경에서 동적 할당(`malloc`/`free`)을 반복할 때 발생하는 **'힙 메모리 파편화(Heap Fragmentation)' 버그를 원천 차단**했습니다.
  * 이더넷 DMA 디스크립터, 수신 패킷 버퍼, LwIP 버퍼 풀, 파일 핸들 등 **모든 메모리 자원을 컴파일 타임에 물리 메모리(RAM_D1 및 RAM_D2 0x30000000)에 고정된 크기로 정적 할당**하여 24시간 연속 가동 시 메모리 고갈이나 다운 현상이 발생하지 않습니다.
* **No OS (Bare-metal) 환경에서의 LwIP 스택 운용:**
  * FreeRTOS 등의 RTOS 없이 순수 베어메탈 메인 루프에서 LwIP 2.1.2 스탠드얼론 UDP 서버를 구동합니다.
* **물리 점퍼선 한계 극복 (SDMMC1 1-bit 모드):**
  * 외장 MicroSD 슬롯 연결 시 긴 점퍼 와이어로 인한 라인 간 신호 왜곡(Skew)을 방지하기 위해 **1-bit 버스 모드(SDMMC_BUS_WIDE_1B)** 및 **보수적 클록 분주(ClockDiv=16, ~6.25MHz)**를 적용하여 무결점 저장을 달성했습니다.
* **부팅 시 세션 파일 자동 채번 (`LOG_0001.TXT` ~ `LOG_9999.TXT`):**
  * 보드가 켜질 때마다 기존 로그 파일을 덮어쓰지 않고, `f_stat()`으로 순차 검사하여 비어있는 가장 빠른 번호의 새 세션 파일을 자동 생성합니다.
* **실시간 무손실 파일 끝 덧붙임 (Append Mode):**
  * 패킷 도착 시 `FA_OPEN_ALWAYS | FA_WRITE` 및 `f_lseek`를 결합하여 파일의 끝에 즉시 데이터를 기록하고 플러시합니다.
* **불필요한 디버그 노이즈 로그 완전 제거:**
  * SD 카드가 섹터를 읽을 때마다 터미널을 도배하던 `sd_diskio.c` 내부의 `[diskio] disk_read: sector...` 콘솔 디버그 출력을 완전히 삭제하여 화면 가독성을 극대화했습니다.
* **PC 송신 vs 보드 실제 물리 저장의 명확한 검증:**
  * 터미널 화면에 뜨는 메시지는 단순 PC 메아리가 아니라, **"보드가 네트워크 패킷을 수신하여 실제 MicroSD 카드에 쓰기를 마쳤다는 보드 발(發) 공증 영수증"**을 USART3 시리얼로 회신받아 표시합니다.
* **콘솔 실시간 원격 제어 인터랙션:**
  * `sd_viewer` 실행 중 키보드 입력(`r`: 현재 세션 파일 덤프, `p`: 이전 세션 파일 덤프)을 통해 보드 내부 SD 카드 데이터를 즉시 화면으로 읽어올 수 있습니다.

---

## 2. 시스템 아키텍처 및 3대 통신 채널 구조

데이터 입력, 저장, 검증 통로가 물리적으로 완벽히 격리되어 있습니다.

```
+-----------------------------------------------------------------------------------+
|                                 PC (노트북)                                       |
|  - chat.ps1 (대화형 전송기)         - auto_sender.ps1 (1초 센서 자동 송신기)      |
|  - sd_viewer.ps1 / sd_viewer.bat (24-bit TrueColor ANSI 시리얼 모니터 & 덤프 제어) |
+-------------------+---------------------------------------^-----------------------+
                    | LAN선 (UDP 고속 입력)                 | USB (USART3 VCP 검증)
                    v                                       |
+-----------------------------------------------------------+-----------------------+
| NUCLEO-H753ZI Board                                                               |
|                                                                                   |
|  [Ethernet MAC/DMA] ---> RAM_D2 (0x30000000) ---> [Cortex-M7 CPU]                 |
|                                                          |                        |
|                                                          +---> [USART3 VCP COM3]  |
|                                                          |     (저장 확인 영수증) |
|                                                          v                        |
|                                                   [SDMMC1 1-bit]                  |
|                                                          |                        |
|                                                          v                        |
|                                                 [MicroSD Card (FatFs)]            |
|                                                 LOG_0001.TXT (세션 자동 채번)      |
+-----------------------------------------------------------------------------------+
```

| 통신 채널 | 물리적 연결 | 프로토콜 / 주소 | 역할 및 데이터 흐름 |
| :--- | :--- | :--- | :--- |
| **이더넷 채널 (입력)** | RJ45 UTP 랜선 | UDP IPv4<br/>PC: `192.168.0.100`<br/>STM32: `192.168.0.50:8080` | 고속 데이터 일방 입력 전용. auto_sender와 chat.ps1이 보드로 패킷 전송 |
| **SDMMC 채널 (저장)** | MicroSD 슬롯 (CN8 점퍼선) | SDMMC1 1-bit 버스<br/>FatFs (FAT32) | 데이터 영구 보존 전용. `LOG_0001.TXT` 세션 파일에 실시간 Append 기록 |
| **UART VCP 채널 (검증)** | Micro-USB 케이블 | USART3 (ST-LINK VCP)<br/>115200 bps, 8-N-1 (COM3) | 보드의 실제 물리 저장 확인 영수증 수신 및 원격 덤프(`r`/`p`) 콘솔 제어 |

---

## 3. 하드웨어 연결 및 핀 매핑

### 1) MicroSD 모듈 연결 (CN8 커넥터 ➔ MicroSD 슬롯)
초기에 SPI 모드로 오해하기 쉬우나, **STM32H753ZI의 네이티브 고속 호스트 컨트롤러인 SDMMC1 전용 핀**으로 연결되어 동작합니다.

| SD 카드 핀 | Nucleo CN8 핀 번호 | STM32 MCU 핀 | 신호명 및 역할 |
| :---: | :---: | :---: | :--- |
| **D0 (DAT0)** | **D43** | `PC8` | SDMMC1 데이터 라인 0 (1-bit 데이터 전송) |
| **CMD** | **D48** | `PD2` | SDMMC1 양방향 명령 및 응답선 |
| **CLK** | **D47** | `PC12` | SDMMC1 전송 클록 (~6.25MHz) |
| **VCC** | **3V3** | 3.3V 전원 | Micro-USB로부터 인가된 보드 3.3V 전원 |
| **GND** | **GND** | 접지 | 공통 접지(Ground) |
| *D1, D2, D3* | *D44, D45, D46* | `PC9, PC10, PC11` | 1-bit 모드 사용 시 미사용 (연결 유지 가능) |

### 2) 보드 전원 공급 및 ST-LINK/V3E 일체형 구조
* 별도의 외장 ST-LINK 동글이 필요하지 않습니다.
* **보드 상단의 ST-LINK/V3E Micro-USB 포트에 케이블 하나만 노트북에 연결**하면:
  1. **전원 공급:** USB 5V ➔ 보드 온보드 LDO를 거쳐 3.3V 전원 변환 공급.
  2. **SWD 디버거:** `flash.bat`을 통한 원클릭 펌웨어 다운로드.
  3. **가상 시리얼 포트(VCP):** USART3(PD8/PD9)가 USB를 통해 장치관리자 `COM3`로 자동 인식.

### 3) 온보드 상태 LED 안내
* **LD1 (녹색, PB0):** 메인 루프 정상 동작 시 점멸 (시스템 하트비트).
* **LD2 (주황/황색, PE1):** 네트워크 통신 LED (이더넷 패킷 수신 시 점등).
* **LD3 (적색, PB14):** 하드웨어 에러 경고 LED (시스템이 `Error_Handler`나 `HardFault` 진입 시 점등).
* **LD4 / LD5:** ST-LINK V3 전원 및 USB 통신 상태 표시등.

---

## 4. 임베디드 메모리 및 펌웨어 핵심 설계

### 1) STM32H7 메모리 구조와 D2 SRAM(0x30000000) 강제 배치
* **DTCM(0x20000000) DMA 접근 불가 원리:**
  * DTCM은 Cortex-M7 CPU 코어 전용 초고속 RAM입니다.
  * AXI 버스 매트릭스 상에서 이더넷 DMA가 DTCM으로 들어가는 물리 통로(Bridge)가 없으므로, 버퍼를 DTCM에 두면 하드웨어 **BusFault/HardFault**가 발생합니다.
* **RAM_D2 (0x30000000) 배치:**
  * 이더넷 DMA와 AHB 버스로 직결된 SRAM1/2/3(288KB) 영역에 링커 스크립트를 통해 디스크립터와 수신 버퍼를 배치했습니다.

### 2) 링커 스크립트 (`STM32H753ZITX_FLASH.ld`)의 NOLOAD 최적화
```ld
.lwip_sec (NOLOAD) :
{
    . = ALIGN(4);
    *(.RxDecripSection)   /* DMARxDscrTab[4] 수신 디스크립터 */
    *(.TxDecripSection)   /* DMATxDscrTab[4] 송신 디스크립터 */
    *(.RxArraySection)    /* Rx_Buff[4][1536] 수신 패킷 버퍼 */
} >RAM_D2 AT> FLASH
```
* `(NOLOAD)` 속성을 통해 수십 KB의 초기화 0 데이터를 바이너리(.bin)에 채우지 않아 빌드 용량과 플래시 다운로드 시간을 대폭 단축했습니다.

### 3) D2 SRAM 전원 클럭 활성화 (`Src/stm32h7xx_hal_msp.c`)
부팅 시 저전력 모드로 꺼져 있는 D2 SRAM 클럭을 켜주어야 AXI 버스 상에서 `OKAY (0b00)` 응답 신호가 정상 회신되어 하드폴트를 방지합니다:
```c
__HAL_RCC_D2SRAM1_CLK_ENABLE();
__HAL_RCC_D2SRAM2_CLK_ENABLE();
__HAL_RCC_D2SRAM3_CLK_ENABLE();
```

### 4) 이더넷 DMA 환형 링 버퍼 및 하드웨어 북마크 레지스터
* 4개의 디스크립터(`DMARxDscrTab[4]`)와 4개의 1536B 수신 버퍼(`Rx_Buff[4]`)가 1:1 매칭되어 원형 링(Circular Linked List)을 구성합니다.
* 하드웨어 내부 북마크 레지스터인 **`DMACRDLR`**가 다음 작업 위치를 기억하므로, 패킷이 도착할 때마다 0번부터 탐색하지 않고 즉시 다음 버퍼에 데이터를 기록합니다.
* CPU가 처리를 못 해 4개 버퍼가 꽉 찬 경우, 하드웨어가 새 패킷을 즉시 버리고(Hardware Drop) FIFO 오버플로우 플래그를 세워 메모리 오염을 원천 차단합니다.

### 5) UART 비동기 에러 플래그 클리어
고속 통신 중 터미널 창 리사이즈나 노이즈로 발생할 수 있는 `ORECF (Overrun)`, `FECF (Framing)`, `NECF (Noise)` 에러 플래그를 인터럽트 핸들러에서 즉시 클리어하여 24시간 365일 통신 연속성을 보장합니다.

---

## 5. 빌드 및 플래시 가이드 (Build & Flash)

터미널에서 무거운 GUI 없이 3초 만에 빌드 및 플래시가 완료됩니다.

### 1) 빌드 (Compile & Link)
터미널에서 아래 명령을 실행하면 `make -j4`가 구동되어 `build\h753zi_sd.bin`이 생성됩니다:
```cmd
.\build.bat
```

### 2) 펌웨어 플래시 (Flash via ST-LINK SWD)
보드를 USB로 연결한 상태에서 아래 스크립트를 실행하면 펌웨어가 플래시 롬(`0x08000000`)에 주입되고 보드가 자동 리셋 가동됩니다:
```cmd
.\flash.bat
```

---

## 6. PC 테스트 스크립트 및 전용 콘솔 뷰어

### 1) 실시간 SD 카드 뷰어 (`sd_viewer.bat` / `sd_viewer.ps1`)
24-bit TrueColor ANSI 시퀀스가 적용되어 있으며, 지저분한 화살표(`->`)를 모두 제거하여 정돈된 콘솔 UI를 제공합니다:
```cmd
.\sd_viewer.bat
```
또는
```powershell
.\sd_viewer.ps1
```

* **시각화 컬러 규격:**
  * `[ETH RX #...] Packet from 192.168.0.100`: **흰색 (White)**
  * `Message Preview: "안녕"`: **선명한 보라색 (Vivid Purple)** - *오토센서 데이터가 아닌 순수 사용자 메시지만 보라색 강조!*
  * `Temp: 24.5C | Humidity: 55.2%`: **시원한 하늘색 (Cyan)**
  * `[SD CARD] Appended...` 및 시스템 상태바: **차분한 회색 (Gray)**
* **실시간 키보드 콘솔 인터랙션:**
  * **`r` 키:** 현재 기록 중인 세션 파일 전체 내용을 SD 카드에서 실시간으로 읽어와 터미널에 덤프.
  * **`p` 키:** 현재 파일명의 번호를 역산하여 **직전 세션 파일(예: `LOG_0020.TXT`) 전체 내용**을 SD 카드에서 덤프.
  * **`q` 키:** 뷰어 안전 종료.

### 2) 대화형 UDP 채팅 전송기 (`chat.ps1`)
PC에서 원하는 텍스트를 입력하여 보드로 UDP 패킷을 전송합니다:
```powershell
.\chat.ps1
```
* 입력한 메시지는 보드의 SD 카드에 실시간 기록되며, `sd_viewer` 창에 보라색으로 프리뷰가 표시됩니다.

### 3) 1초 주기 센서 패킷 자동 송신기 (`auto_sender.ps1`)
실제 IoT 센서 장비처럼 1초마다 보드로 온도/습도 더미 데이터를 연속 전송합니다:
```powershell
.\auto_sender.ps1
```
* 중지하려면 키보드의 `[Ctrl + C]`를 누르면 됩니다.

> 💡 **PowerShell 권한 에러 해결 팁 (최초 1회):**  
> 스크립트 실행 시 정책 에러가 발생하면 PowerShell 창에서 다음 명령을 한 번만 실행해 두면 긴 옵션 없이 실행할 수 있습니다:  
> `Set-ExecutionPolicy -Scope CurrentUser -ExecutionPolicy Bypass -Force`

---

## 7. 프로젝트 디렉터리 구조

```text
h753zi_sd/
├── Inc/                      # 헤더 파일 디렉터리
│   ├── app_ethernet.h        # UDP 로거 및 파일 세션 관리 헤더
│   ├── main.h                # 핀 매핑 및 전역 설정
│   └── sd_diskio.h           # SDMMC1 FatFs 디스크 드라이버 헤더
├── Src/                      # C 소스 파일 디렉터리
│   ├── app_ethernet.c        # UDP 수신, FatFs 세션 파일 생성 및 Append 로직
│   ├── ethernetif.c          # 이더넷 DMA 인터페이스 및 디스크립터 연동
│   ├── main.c                # 시스템 초기화, USART3 printf 리타게팅, 메인 루프
│   ├── sd_diskio.c           # FatFs 하위 SDMMC 드라이버 (디버그 로그 제거됨)
│   └── stm32h7xx_hal_msp.c   # D2 SRAM 클럭 인가 및 주변장치 하드웨어 초기화
├── Drivers/                  # STM32H7 HAL 및 CMSIS 드라이버
├── Middlewares/              # FatFs 및 LwIP 네트워크 스택
├── STM32H753ZITX_FLASH.ld    # D2 SRAM 및 NOLOAD 메모리 배치 링커 스크립트
├── build.bat                 # make 기반 펌웨어 초고속 빌드 스크립트
├── flash.bat                 # ST-LINK SWD 펌웨어 주입 스크립트
├── sd_viewer.ps1             # 24-bit TrueColor ANSI 시리얼 모니터 & 덤프 뷰어
├── sd_viewer.bat             # 뷰어 원클릭 실행 배치 파일
├── chat.ps1                  # 대화형 UDP 메시지 전송기
├── auto_sender.ps1           # 1초 주기 센서 데이터 자동 전송기
└── README.md                 # 본 프로젝트 메인 기술 문서
```
