#include <stdint.h>

// ─── Base Addresses ───────────────────────────────────────────
#define RCC_BASE     0x40021000
#define GPIOA_BASE   0x40010800
#define GPIOB_BASE   0x40010C00
#define GPIOC_BASE   0x40011000
#define AFIO_BASE    0x40010000
#define EXTI_BASE    0x40010400
#define FLASH_BASE   0x40022000
#define USART2_BASE  0x40004400
#define SPI2_BASE    0x40003800

// ─── RCC Registers ────────────────────────────────────────────
#define RCC_APB1ENR  (*(volatile uint32_t *)(RCC_BASE + 0x1C))
#define RCC_APB2ENR  (*(volatile uint32_t *)(RCC_BASE + 0x18))
#define RCC_CR       (*(volatile uint32_t *)(RCC_BASE + 0x00))
#define RCC_CFGR     (*(volatile uint32_t *)(RCC_BASE + 0x04))

// ─── GPIOA Registers ──────────────────────────────────────────
#define GPIOA_CRL    (*(volatile uint32_t *)(GPIOA_BASE + 0x00))

// ─── GPIOB Registers ──────────────────────────────────────────
#define GPIOB_CRH    (*(volatile uint32_t *)(GPIOB_BASE + 0x04))
#define GPIOB_ODR    (*(volatile uint32_t *)(GPIOB_BASE + 0x0C))

// ─── GPIOC Registers ──────────────────────────────────────────
#define GPIOC_CRH    (*(volatile uint32_t *)(GPIOC_BASE + 0x04))
#define GPIOC_ODR    (*(volatile uint32_t *)(GPIOC_BASE + 0x0C))
#define GPIOC_IDR    (*(volatile uint32_t *)(GPIOC_BASE + 0x08))

// ─── AFIO Registers ───────────────────────────────────────────
#define AFIO_EXTICR4 (*(volatile uint32_t *)(AFIO_BASE + 0x14))

// ─── EXTI Registers ───────────────────────────────────────────
#define EXTI_IMR     (*(volatile uint32_t *)(EXTI_BASE + 0x00))
#define EXTI_FTSR    (*(volatile uint32_t *)(EXTI_BASE + 0x0C))
#define EXTI_PR      (*(volatile uint32_t *)(EXTI_BASE + 0x14))

// ─── Flash Register ───────────────────────────────────────────
#define FLASH_ACR    (*(volatile uint32_t *)(FLASH_BASE + 0x00))

// ─── SysTick Registers ────────────────────────────────────────
#define SYST_CSR     (*(volatile uint32_t *)0xE000E010)
#define SYST_RVR     (*(volatile uint32_t *)0xE000E014)
#define SYST_CVR     (*(volatile uint32_t *)0xE000E018)

// ─── NVIC ─────────────────────────────────────────────────────
#define NVIC_ISER1   (*(volatile uint32_t *)0xE000E104)

// ─── USART2 Registers ─────────────────────────────────────────
#define USART2_SR    (*(volatile uint32_t *)(USART2_BASE + 0x00))
#define USART2_DR    (*(volatile uint32_t *)(USART2_BASE + 0x04))
#define USART2_BRR   (*(volatile uint32_t *)(USART2_BASE + 0x08))
#define USART2_CR1   (*(volatile uint32_t *)(USART2_BASE + 0x0C))
#define USART2_CR2   (*(volatile uint32_t *)(USART2_BASE + 0x10))

// ─── SPI2 Registers ───────────────────────────────────────────
#define SPI2_CR1     (*(volatile uint32_t *)(SPI2_BASE + 0x00))
#define SPI2_CR2     (*(volatile uint32_t *)(SPI2_BASE + 0x04))
#define SPI2_SR      (*(volatile uint32_t *)(SPI2_BASE + 0x08))
#define SPI2_DR      (*(volatile uint32_t *)(SPI2_BASE + 0x0C))

// ─── Pin Definitions ──────────────────────────────────────────
#define BTN_PIN      13
#define NSS_PIN      12    // PB12

// ─── State Machine ────────────────────────────────────────────
typedef enum {
    STATE_SPI_OFF,
    STATE_SPI_ON
} State;

// ─── Global Variables ─────────────────────────────────────────
volatile uint32_t tick_count = 0;
volatile uint8_t  btn_event  = 0;
volatile uint8_t  spi_rx     = 0;

// ─── SystemInit ───────────────────────────────────────────────
void SystemInit(void) {
}

// ─── SysTick Handler ──────────────────────────────────────────
void SysTick_Handler(void) {
    tick_count++;
}

// ─── Button Interrupt ─────────────────────────────────────────
void EXTI15_10_IRQHandler(void) {
    if(EXTI_PR & (1 << BTN_PIN)) {
        btn_event = 1;
        EXTI_PR = (1 << BTN_PIN);
    }
}

// ─── get_tick ─────────────────────────────────────────────────
uint32_t get_tick(void) {
    return tick_count;
}

// ─── PLL Init ─────────────────────────────────────────────────
void pll_init(void) {
    RCC_CR      |=  (1 << 16);
    while(!(RCC_CR & (1 << 17)));

    FLASH_ACR   &= ~(0x7);
    FLASH_ACR   |=  (0x2);

    RCC_CFGR    &= ~(0xF << 4);
    RCC_CFGR    |=  (0x4 << 8);
    RCC_CFGR    &= ~(0x7 << 11);

    RCC_CFGR    |=  (1 << 16);
    RCC_CFGR    &= ~(1 << 17);
    RCC_CFGR    &= ~(0xF << 18);
    RCC_CFGR    |=  (0x7 << 18);

    RCC_CR      |=  (1 << 24);
    while(!(RCC_CR & (1 << 25)));

    RCC_CFGR    &= ~(0x3);
    RCC_CFGR    |=  (0x2);
    while((RCC_CFGR & (0x3 << 2)) != (0x2 << 2));
}

// ─── SysTick Init ─────────────────────────────────────────────
void systick_init(void) {
    SYST_RVR = 71999;
    SYST_CVR = 0;
    SYST_CSR = 0x7;
}

// ─── USART2 Init ──────────────────────────────────────────────
void usart2_init(void) {
    RCC_APB1ENR |= (1 << 17);      // USART2EN
    RCC_APB2ENR |= (1 << 2);       // IOPAEN

    GPIOA_CRL   &= ~(0xF << 8);
    GPIOA_CRL   |=  (0x9 << 8);    // PA2 TX → AF push-pull 10MHz

    GPIOA_CRL   &= ~(0xF << 12);
    GPIOA_CRL   |=  (0x4 << 12);   // PA3 RX → floating input

    USART2_CR1  |=  (1 << 13);     // UE
    USART2_CR1  &= ~(1 << 12);     // 8 data bits
    USART2_CR2  &= ~(0x3 << 12);   // 1 stop bit
    USART2_BRR   =   0x139;        // 115200 baud
    USART2_CR1  |=  (1 << 3);      // TE
    USART2_CR1  |=  (1 << 2);      // RE
    USART2_CR1  |=  (1 << 5);      // RXNEIE
    NVIC_ISER1  |=  (1 << 6);      // IRQ38
}

// ─── GPIO Init ────────────────────────────────────────────────
void gpio_init(void) {
    RCC_APB2ENR |= (1 << 0);       // AFIOEN
    RCC_APB2ENR |= (1 << 3);       // IOPBEN
    RCC_APB2ENR |= (1 << 4);       // IOPCEN

    // PC13 → input pull-up (button)
    // bits 23:20 of CRH → CNF=10 MODE=00 → 0x8
    GPIOC_CRH   &= ~(0xF << 20);
    GPIOC_CRH   |=  (0x8 << 20);
    GPIOC_ODR   |=  (1 << BTN_PIN);

    // PB12 NSS  → GP push-pull output 10MHz (software controlled)
    // bits 19:16 of CRH → CNF=00 MODE=01 → 0x1
    GPIOB_CRH   &= ~(0xF << 16);
    GPIOB_CRH   |=  (0x1 << 16);
    GPIOB_ODR   |=  (1 << NSS_PIN);    // NSS HIGH initially (idle)

    // PB13 SCK  → AF push-pull 10MHz
    // bits 23:20 of CRH → CNF=10 MODE=01 → 0x9
    GPIOB_CRH   &= ~(0xF << 20);
    GPIOB_CRH   |=  (0x9 << 20);

    // PB14 MISO → floating input
    // bits 27:24 of CRH → CNF=01 MODE=00 → 0x4
    GPIOB_CRH   &= ~(0xF << 24);
    GPIOB_CRH   |=  (0x4 << 24);

    // PB15 MOSI → AF push-pull 10MHz
    // bits 31:28 of CRH → CNF=10 MODE=01 → 0x9
    GPIOB_CRH   &= ~(0xF << 28);
    GPIOB_CRH   |=  (0x9 << 28);
}

// ─── EXTI Init ────────────────────────────────────────────────
void exti_init(void) {
    AFIO_EXTICR4 &= ~(0xF << 4);
    AFIO_EXTICR4 |=  (0x2 << 4);   // PC13 → EXTI13

    EXTI_IMR  |= (1 << BTN_PIN);
    EXTI_FTSR |= (1 << BTN_PIN);

    NVIC_ISER1 |= (1 << 8);        // IRQ40
}

// ─── SPI2 Init ────────────────────────────────────────────────
void spi2_init(void) {

    // 1. Enable SPI2 clock (APB1 bit 14)
    RCC_APB1ENR |= (1 << 14);      // SPI2EN

    // 2. Configure CR1 — do this before SPE = 1
    SPI2_CR1 = 0;                   // clear register first

    // BR[2:0] = 100 → fPCLK/8 = 36MHz/8 = 4.5MHz
    // bits 5:3
    SPI2_CR1 |= (0x4 << 3);

    // CPOL = 0 → clock idle LOW    (bit 1)
    // CPHA = 0 → sample on first edge (bit 0)
    // Mode 0 — both bits 0, already cleared above

    // DFF = 0 → 8-bit data frame   (bit 11)
    // already 0 after clear

    // LSBFIRST = 0 → MSB first     (bit 7)
    // already 0 after clear

    // SSM = 1 → software NSS management (bit 9)
    SPI2_CR1 |= (1 << 9);

    // SSI = 1 → internal NSS HIGH  (bit 8)
    // prevents master mode fault
    SPI2_CR1 |= (1 << 8);

    // MSTR = 1 → master mode       (bit 2)
    SPI2_CR1 |= (1 << 2);

    // 3. SPE = 1 → enable SPI      (bit 6)
    SPI2_CR1 |= (1 << 6);
}

// ─── SPI NSS Control ──────────────────────────────────────────
void spi_select(void) {
    GPIOB_ODR &= ~(1 << NSS_PIN);  // NSS LOW → select slave
}

void spi_deselect(void) {
    GPIOB_ODR |=  (1 << NSS_PIN);  // NSS HIGH → deselect slave
}

// ─── SPI Transmit and Receive (full duplex) ───────────────────
// In full duplex, every transmit produces a received byte
// Returns the byte received from slave during same clock cycle
uint8_t spi_transfer(uint8_t data) {
    while(!(SPI2_SR & (1 << 1)));  // wait TXE = 1 (TX buffer empty)
    SPI2_DR = data;                 // write to TX buffer
    while(!(SPI2_SR & (1 << 0)));  // wait RXNE = 1 (RX buffer full)
    return (uint8_t)SPI2_DR;       // read received byte (clears RXNE)
}

// ─── SPI Stop ─────────────────────────────────────────────────
void spi_stop(void) {
    while(SPI2_SR & (1 << 7));     // wait BSY = 0
    spi_deselect();                 // NSS HIGH
}

// ─── USART2 Transmit ──────────────────────────────────────────
void usart2_transmit(uint8_t data) {
    while(!(USART2_SR & (1 << 7)));
    USART2_DR = data;
}

// ─── USART2 Send String ───────────────────────────────────────
void usart2_send_string(const char *str) {
    while(*str) {
        usart2_transmit((uint8_t)*str++);
    }
}

// ─── USART2 Send Number ───────────────────────────────────────
void usart2_send_number(uint32_t num) {
    if(num == 0) { usart2_transmit('0'); return; }
    char buf[10];
    int i = 0;
    while(num > 0) { buf[i++] = '0' + (num % 10); num /= 10; }
    while(i > 0)   { usart2_transmit(buf[--i]); }
}

// ─── Main ─────────────────────────────────────────────────────
int main(void) {

    pll_init();
    systick_init();
    usart2_init();
    gpio_init();
    exti_init();
    spi2_init();

    usart2_send_string("SPI Master Ready\r\n");
    usart2_send_string("Press button to start\r\n");

    State current_state = STATE_SPI_OFF;

    while(1) {

        // ── Button debounce ───────────────────────────────────
        uint8_t event = 0;
        if(btn_event) {
            uint32_t press_time = get_tick();
            while(!((GPIOC_IDR >> BTN_PIN) & 1)) {
                if(get_tick() - press_time > 20) {
                    event = 1;
                    break;
                }
            }
            btn_event = 0;
        }

        // ── State Machine ─────────────────────────────────────
        switch(current_state) {

            case STATE_SPI_OFF:
                if(event) {
                    current_state = STATE_SPI_ON;
                    usart2_send_string("State: SPI ON\r\n");
                }
                break;

            case STATE_SPI_ON:
                if(event) {
                    current_state = STATE_SPI_OFF;
                    usart2_send_string("State: SPI OFF\r\n");
                } else {
                    // Select slave
                    spi_select();

                    // Send a test byte and receive response
                    // 0xA5 = 1010 0101 — easy to identify on logic analyzer
                    uint8_t tx_byte = 0xA5;		//165 in decimal
                    uint8_t rx_byte = spi_transfer(tx_byte);

                    // Stop transaction
                    spi_stop();

                    // Log over USART
                    usart2_send_string("TX: 0x");
                    usart2_send_number(tx_byte);
                    usart2_send_string(" RX: 0x");
                    usart2_send_number(rx_byte);
                    usart2_send_string("\r\n");

                    // 200ms between transfers
                    uint32_t t = get_tick();
                    while(get_tick() - t < 200);
                }
                break;
        }
    }
}
