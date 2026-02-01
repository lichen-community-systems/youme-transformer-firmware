#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "midi_uart_lib_config.h"
#include "led.h"
#include "midi-port.h"
#include "uart-midi-port.h"
#include "usb-midi-device-port.h"
#include "midi-logger.h"

#define CPU_CLOCK_SPEED_KHZ 240000

#define MIDI_UART_NUM 0
#define MIDI_UART_TX_GPIO 0
#define MIDI_UART_RX_GPIO 1

#define LOG_BUFFER_SIZE 1024 * 100

LED mainLED;
LED noteLED;
UARTMidiPort uartMidiPort;
USBMidiDevicePort usbMidiDevicePort;
MIDILogger<LOG_BUFFER_SIZE> midiLogger;

void handleLEDStateForMIDIMessage(uint8_t* message) {
    // Light up the LED when notes are on.
    if (sig_MIDI_MESSAGE_TYPE(message[0]) == sig_MIDI_STATUS_NOTE_ON &&
        message[2] > 0) {
        noteLED.on();
    } else if (sig_MidiParser_isNoteOff(message)) {
        noteLED.off();
    }
}

void writeMessageFromUART(uint8_t* message, size_t size) {
    (void) uartMidiPort.write(message, size);
    (void) usbMidiDevicePort.write(message, size);
}

void writeMessageFromUSBDevice(uint8_t* message, size_t size) {
    // Only write to the UART; don't echo the message back to USB.
    (void) uartMidiPort.write(message, size);
}

void onMIDIMessage(uint8_t* message, size_t size, void* userData) {
    handleLEDStateForMIDIMessage(message);

    if (userData == &uartMidiPort) {
        writeMessageFromUART(message, size);
    } else if (userData == &usbMidiDevicePort) {
        writeMessageFromUSBDevice(message, size);
    }

    midiLogger.write(message, size);
}

void onSysexChunk(uint8_t* sysexData, size_t size, bool isFinal,
    void* userData) {
    (void)userData;
    (void)isFinal;

    (void) uartMidiPort.write(sysexData, size);
    (void) usbMidiDevicePort.write(sysexData, size);
}

int main() {
    set_sys_clock_khz(CPU_CLOCK_SPEED_KHZ, true);

    mainLED.init(25);
    noteLED.init(24);

    UARTConfig uartConfig = {
        .uartNum = MIDI_UART_NUM,
        .txGPIO = MIDI_UART_TX_GPIO,
        .rxGPIO = MIDI_UART_RX_GPIO
    };

    MidiParserConfig uartParserConfig = {
        .onMIDIMessage = onMIDIMessage,
        .onSysexChunk = onSysexChunk,
        .userData = &uartMidiPort
    };

    uartMidiPort.init(uartConfig, uartParserConfig);
    MidiParserConfig usbParserConfig = {
        .onMIDIMessage = onMIDIMessage,
        .onSysexChunk = onSysexChunk,
        .userData = &usbMidiDevicePort
    };
    usbMidiDevicePort.init(usbParserConfig);

    midiLogger.init();

    mainLED.on();

    while (true) {
        uartMidiPort.tick();
        usbMidiDevicePort.tick();
    }

    noteLED.off();
    mainLED.off();

    return 0;
}
