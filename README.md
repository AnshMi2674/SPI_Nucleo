# STM32F103RB Bare-Metal SPI Communication with Arduino UNO

> Full-duplex SPI communication between NUCLEO-F103RB (master) and Arduino UNO (slave). No HAL, no libraries on the STM32 side. Every register configured directly from RM0008. Completed in one day — the fastest of three serial protocol projects.

---

## Project Overview

This project establishes SPI communication between a STM32F103RBT6 acting as SPI master and an Arduino UNO acting as SPI slave. A button on PC13 starts and stops the communication. While active, the Nucleo sends 0xA5 every 200ms and receives 0xBB back from the Arduino. All transactions are logged over USART2 to a serial terminal.

---

## Hardware

| Component | Details |
|---|---|
| Master | STMicroelectronics NUCLEO-F103RB (STM32F103RBT6) |
| Slave | Arduino UNO (ATmega328P) |
| SPI Interface | SPI2 on STM32, hardware SPI on Arduino |
| Serial Monitor | USART2 via ST-LINK Virtual COM Port |
| Button | B1 (USER) on PC13 — active LOW |

---

## Wiring

| Nucleo Pin | Arduino Pin | Signal |
|---|---|---|
| PB12 | Pin 10 (SS) | NSS (Chip Select) |
| PB13 | Pin 13 (SCK) | Clock |
| PB14 | Pin 12 (MISO) | Master In Slave Out |
| PB15 | Pin 11 (MOSI) | Master Out Slave In |
| GND | GND | Common Ground ← critical |

### Voltage Level Note

```
STM32 outputs 3.3V logic
Arduino inputs accept 3.3V as HIGH (threshold ~2V) ✅
Arduino MISO outputs 5V
PB14 is FT (5V tolerant) pin — confirmed in DS5319 ✅
No level shifting needed
```

---

## SPI Configuration

### STM32 (Master)

```
Peripheral  : SPI2
Clock       : APB1 → 36MHz
Baud rate   : fPCLK/32 = 1.125MHz  (BR = 100 in CR1 bits 5:3)
Mode        : Mode 0 (CPOL=0, CPHA=0)
Data frame  : 8-bit (DFF = 0)
Bit order   : MSB first (LSBFIRST = 0)
NSS         : Software (SSM=1, SSI=1) — controlled via GPIO ODR
Direction   : Full duplex (BIDIMODE=0, RXONLY=0)
```

### Arduino UNO (Slave)

```
Mode        : SPI slave (MSTR bit not set in SPCR)
SPI Mode    : Mode 0 (CPOL=0, CPHA=0)
Interrupt   : SPI_STC_vect — fires on each complete byte transfer
Pre-loaded  : 0xBB in SPDR before first transaction
```

---

## GPIO Configuration

| Pin | Function | MODE | CNF | 4-bit value | CRH bits |
|---|---|---|---|---|---|
| PB12 | NSS (GP output) | 01 (10MHz) | 00 (GP push-pull) | 0001 = 0x1 | 19:16 |
| PB13 | SCK (AF output) | 01 (10MHz) | 10 (AF push-pull) | 1001 = 0x9 | 23:20 |
| PB14 | MISO (input) | 00 (input) | 01 (floating) | 0100 = 0x4 | 27:24 |
| PB15 | MOSI (AF output) | 01 (10MHz) | 10 (AF push-pull) | 1001 = 0x9 | 31:28 |

### Why NSS is GP push-pull not AF push-pull

NSS is controlled manually via GPIO ODR in software NSS mode (SSM=1). The SPI peripheral has no control over the pin — it is a regular GPIO output, not alternate function.

---

## Register Map

### SPI2 (Base: 0x40003800)

| Register | Offset | Purpose |
|---|---|---|
| SPI2_CR1 | 0x00 | Mode, clock, baud rate, frame format, enable |
| SPI2_CR2 | 0x04 | Interrupt enable, DMA, NSS output |
| SPI2_SR | 0x08 | Status flags (TXE, RXNE, BSY) |
| SPI2_DR | 0x0C | Data register (read/write) |

### CR1 Bits Used

```
Bit 15 → BIDIMODE = 0  (2-line unidirectional, full duplex)
Bit 11 → DFF      = 0  (8-bit data frame)
Bit 9  → SSM      = 1  (software NSS management)
Bit 8  → SSI      = 1  (internal NSS HIGH, prevents MODF fault)
Bit 7  → LSBFIRST = 0  (MSB transmitted first)
Bits 5:3 → BR    = 100 (fPCLK/32 = 1.125MHz)
Bit 2  → MSTR    = 1   (master mode)
Bit 1  → CPOL    = 0   (clock idle LOW)
Bit 0  → CPHA    = 0   (sample on first edge)
Bit 6  → SPE     = 1   (SPI enable — set last)
```

---

## How Full Duplex SPI Works

In full duplex mode, every clock cycle simultaneously shifts one bit out on MOSI and one bit in on MISO. There is no separate send and receive — they happen on the same 8 clock pulses:

```
Master sends 0xA5:    1 0 1 0 0 1 0 1  (on MOSI)
Slave sends  0xBB:    1 0 1 1 1 0 1 1  (on MISO)
                      ↑               ↑
                   same 8 clock pulses
```

This is why `spi_transfer()` both sends and returns a value — you cannot send without simultaneously receiving.

---

## Transaction Sequence

```
1. NSS pulled LOW  (PB12 → 0) → slave selected
2. Write 0xA5 to SPI2_DR → enters TX buffer
3. Poll TXE = 1 (TX buffer empty → byte in shift register)
4. 8 clock pulses → 0xA5 shifted out, 0xBB shifted in
5. Poll RXNE = 1 (RX buffer has received byte)
6. Read SPI2_DR → returns 0xBB, clears RXNE
7. Poll BSY = 0 (transfer complete)
8. NSS pulled HIGH (PB12 → 1) → slave deselected
```

---

## Documents Used

| Document | Purpose |
|---|---|
| **RM0008** | SPI2 registers, GPIO, RCC, EXTI, USART |
| **DS5319** | Pin alternate functions (PB12-PB15 SPI2 mapping), FT pin confirmation |
| **UM1724** | USART2 virtual COM port, button wiring |
| **ATmega328P datasheet** | SPCR register, SPI_STC interrupt |

---

## Problems Encountered and Solutions

---

### Problem 1 — Initial Corrupted Data on Both Sides

**What happened:**
First test showed Nucleo RX alternating between 0xFF and 0x00. Arduino received random values like 0xE5, 0xA1, 0xAF instead of 0xA5.

**Investigation:**
0xFF and 0x00 on MISO are classic signs of SPI mode mismatch. When CPOL/CPHA differs between master and slave, bits are sampled on the wrong clock edge. The master reads the bus either during setup time (before valid data) or hold time (after data changed) giving all 1s or all 0s.

The Arduino values (0xE5, 0xA1 etc.) showed correct lower bits but corrupted upper bits — consistent with a bit-shift caused by incorrect clock phase alignment.

**Solution — Part 1:** Slowed SPI clock from 4.5MHz (fPCLK/8) to 1.125MHz (fPCLK/32). Arduino UNO running Serial alongside SPI slave cannot reliably handle fast clocks.

**Solution — Part 2:** Replaced Arduino polling sketch with interrupt-driven approach using `SPI_STC_vect`. Polling `SPIF` in `loop()` misses bytes at higher speeds because the flag can be cleared before the loop checks it.

**Solution — Part 3:** Added `while(digitalRead(SS) == LOW)` before enabling SPI on Arduino. This prevents Arduino from capturing partial bytes during STM32 initialization when NSS transitions.

**Lesson:**
SPI mode (CPOL/CPHA) must match exactly between master and slave. When debugging SPI corruption, always verify mode first. Also — Arduino as SPI slave requires interrupt-driven byte capture, not polling, for reliable operation.

---

### Problem 2 — Why NSS is Not Alternate Function

**What happened:**
Initially configured PB12 as AF push-pull (0x9) like other SPI pins. NSS behavior was unpredictable.

**Investigation:**
When SSM=1 (software NSS management), the SPI peripheral completely ignores the physical NSS pin. NSS is controlled manually via GPIO ODR. If the pin is in AF mode, the SPI peripheral's NSS output logic (which is disabled by SSM=1) still has some influence on the pin state.

**Solution:**
Configure PB12 as regular GP push-pull output (0x1), not AF push-pull. Manual control via ODR:
```c
GPIOB_ODR &= ~(1 << 12);   // NSS LOW  → select slave
GPIOB_ODR |=  (1 << 12);   // NSS HIGH → deselect slave
```

**Lesson:**
GPIO alternate function mode gives pin control to the peripheral. If the peripheral's feature (hardware NSS) is disabled via SSM=1, the pin should be configured as regular GPIO — not AF — to ensure clean manual control.

---

### Problem 3 — SPI Mode vs I2C Mode Naming Confusion

**What happened:**
SPI "Mode 0" (CPOL=0, CPHA=0) vs I2C open-drain mode caused confusion during GPIO configuration.

**Clarification:**

```
I2C open-drain → physical electrical characteristic
                  pin can only pull LOW or float
                  required because I2C is a shared bus (wired-AND)
                  multiple devices share SDA/SCL

SPI Mode 0     → clock polarity and phase timing
                  CPOL=0: clock idle LOW
                  CPHA=0: data sampled on rising edge
                  has nothing to do with electrical output type

SPI uses push-pull, not open-drain
SPI has dedicated lines, no bus sharing
```

---

## Comparison: SPI vs I2C vs UART

| Feature | UART | I2C | SPI |
|---|---|---|---|
| Wires | 2 (TX, RX) | 2 (SDA, SCL) | 4 (MOSI, MISO, SCK, NSS) |
| Speed | Medium | Slow-Medium | Fast |
| Multi-device | No | Yes (addressing) | Yes (multiple NSS) |
| Full duplex | No | No | Yes |
| Complexity | Low | Medium | Low-Medium |
| Clock | Async (baud rate) | Sync (master) | Sync (master) |
| Error detection | Parity (optional) | ACK/NACK | None (add CRC) |

---

## Why This Was the Fastest Project

By the third serial protocol project, the RM reading process had become familiar. The pattern repeats across every peripheral:

```
1. Enable peripheral clock via RCC
2. Configure GPIO pins (check DS5319 for alternate functions)
3. Configure peripheral registers (mode, speed, format)
4. Enable peripheral
5. Poll status flags for each operation
```

Having already done UART and I2C, SPI felt like reading a familiar story with different characters. The debugging skills from previous projects (oscilloscope, logic analyzer via Arduino, systematic flag checking) also made this faster to diagnose and fix.

---

## Project Structure

```
SPI_Nucleo/
├── Src/
│   └── main.c           ← STM32 bare-metal SPI master
├── Arduino/
│   └── spi_slave.ino    ← Arduino UNO SPI slave
├── Startup/
│   └── startup_stm32f103rbtx.s
├── STM32F103RBTX_FLASH.ld
└── README.md
```

---

## Building and Flashing

**STM32:**
```
Build  : Ctrl+B in STM32CubeIDE
Flash  : F11 → F8 to run
Serial : 115200 baud (any terminal)
```

**Arduino:**
```
Open spi_slave.ino in Arduino IDE
Upload to UNO
Open Serial Monitor at 115200 baud
```

---

## Author

**Ansh**
EXTC First Year, SPIT Mumbai
Learning bare-metal embedded systems from scratch — RM first, code second.

Third serial protocol project completed. UART → I2C → SPI.

---

## License

Educational use. Free to use and learn from.
