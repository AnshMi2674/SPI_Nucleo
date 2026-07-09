#include <SPI.h>

volatile uint8_t received = 0;
volatile bool    data_ready = false;

ISR(SPI_STC_vect) {
    received   = SPDR;
    SPDR       = 0xBB;
    data_ready = true;
}

void setup() {
    Serial.begin(115200);

    // Set MISO as output — mandatory for slave
    pinMode(MISO, OUTPUT);
    pinMode(MOSI, INPUT);
    pinMode(SCK,  INPUT);
    pinMode(SS,   INPUT);    // SS must be INPUT for hardware slave

    // Wait for SS to go HIGH before enabling SPI
    // This prevents capturing garbage during Nucleo startup
    while(digitalRead(SS) == LOW);

    SPCR = (1 << SPE) | (1 << SPIE);   // enable SPI + interrupt, slave, Mode 0
    SPDR = 0xBB;    // pre-load response

    sei();
    Serial.println("Arduino SPI Slave Ready");
}

void loop() {
    if(data_ready) {
        data_ready = false;
        Serial.print("Received: 0x");
        Serial.println(received, HEX);
        delay(500);
    }
}
