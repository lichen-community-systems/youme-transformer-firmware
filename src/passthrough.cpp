#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "midi_uart_lib_config.h"
#include "midi_uart_lib.h"
#include "tusb.h"
#include "class/midi/midi_device.h"
#include "led.h"

#define CPU_CLOCK_SPEED_KHZ 240000
#define LED_PIN 25
#define LED_DELAY_MS 250

#define MIDI_UART_NUM 0
#define MIDI_UART_TX_GPIO 0
#define MIDI_UART_RX_GPIO 1

#define READ_BUFFER_SIZE 128

LED led;
void* midi_uart;
uint8_t uartReadBuffer[READ_BUFFER_SIZE] = {0};
constexpr size_t uartReadBufferSize = sizeof(uartReadBuffer);
bool isMountedAsUSBMIDIDevice = false;
size_t numUARTTXDroppedBytes = 0;
size_t numUSBTXDroppedBytes = 0;

void writeToUART(uint8_t* buffer, uint32_t numBytes) {
    uint8_t bytesWritten = midi_uart_write_tx_buffer(
            midi_uart, buffer, numBytes);

    if (bytesWritten < numBytes) {
        numUARTTXDroppedBytes += (numBytes - bytesWritten);
    }

    midi_uart_drain_tx_buffer(midi_uart);
}

void writeToUSBMIDI(uint8_t* buffer, uint32_t numBytes) {
    if (!tud_midi_mounted()) {
        // Don't treat bytes not written due to an
        // unmounted device as dropped.
        return;
    }

    uint32_t bytesWritten = tud_midi_stream_write(
        0, buffer, numBytes);

    if (bytesWritten < numBytes) {
        numUSBTXDroppedBytes += (numBytes - bytesWritten);
    }
}

void midiDataPassthroughLoop() {
    uint8_t numBytesRead = midi_uart_poll_rx_buffer(
        midi_uart, uartReadBuffer, uartReadBufferSize);

    if (numBytesRead == 0) {
        return;
    }

    writeToUART(uartReadBuffer, numBytesRead);
    writeToUSBMIDI(uartReadBuffer, numBytesRead);
}

int main() {
    set_sys_clock_khz(CPU_CLOCK_SPEED_KHZ, true);

    // Turn on the LED.
    // TODO: Flash this when MIDI messages are received.
    int rc = led.init(LED_PIN);
    hard_assert(rc == PICO_OK);
    led.on();

    // Set MIDI TRS.
    midi_uart = midi_uart_configure(
        MIDI_UART_NUM, MIDI_UART_TX_GPIO, MIDI_UART_RX_GPIO);

    // Set up as a USB MIDI device on the Host/PWR port.
    tusb_init();

    while (true) {
        tud_task();
        midiDataPassthroughLoop();
    }

    led.off();
    return 0;
}
