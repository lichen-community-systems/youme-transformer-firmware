#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "midi_uart_lib_config.h"
#include "led.h"
#include "midi-port.h"
#include "uart-midi-port.h"
#include "usb-midi-device-port.h"
#include "usb-midi-host-port.h"
#include "midi-logger.h"

#define CPU_CLOCK_SPEED_KHZ 240000

#define MIDI_UART_NUM 0
#define MIDI_UART_TX_GPIO 0
#define MIDI_UART_RX_GPIO 1
#define USB_HOST_DP_GPIO 12
#define LOG_BUFFER_SIZE 1024 * 100

LED mainLED;
LED noteLED;
UARTMidiPort uartMidiPort;
USBMidiDevicePort usbDevice;
USBMidiHostPort usbHost;

void handleLEDStateForMIDIMessage(uint8_t* message) {
    // Light up the LED when notes are on.
    if (sig_MIDI_MESSAGE_TYPE(message[0]) == sig_MIDI_STATUS_NOTE_ON &&
        message[2] > 0) {
        noteLED.on();
    } else if (sig_MidiParser_isNoteOff(message)) {
        noteLED.off();
    }
}

void writeMessageFromUART(uint8_t* message, size_t size,
    void* userData) {
    (void) userData;
    // Write to all output ports.
    uartMidiPort.write(message, size);
    usbDevice.write(message, size);
    usbHost.write(message, size);
}

void writeMessageFromUSBDevice(uint8_t* message, size_t size,
    void* userData) {
    (void) userData;
    // Only write to the UART and USB host port;
    // don't echo the message back to the USB device port.
    uartMidiPort.write(message, size);
    usbHost.write(message, size);
}

void writeMessageFromUSBHost(uint8_t* message, size_t size,
    void* userData) {
    (void) userData;

    // Only write to the UART and USB device port;
    // don't echo the message back to the USB host port.
    uartMidiPort.write(message, size);
    usbDevice.write(message, size);
}

void onSysexChunk(uint8_t* sysexData, size_t size, void* userData,
    bool isFinal) {
    (void) userData;
    (void) isFinal;

    uartMidiPort.write(sysexData, size);
    usbDevice.write(sysexData, size);
    usbHost.write(sysexData, size);
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
        .onMIDIMessage = writeMessageFromUART,
        .onSysexChunk = onSysexChunk,
        .userData = &uartMidiPort
    };
    uartMidiPort.init(uartConfig, uartParserConfig);

    MidiParserConfig usbDeviceParserConfig = {
        .onMIDIMessage = writeMessageFromUSBDevice,
        .onSysexChunk = onSysexChunk,
        .userData = &usbDevice
    };
    usbDevice.init(usbDeviceParserConfig);

    MidiParserConfig usbHostParserConfig = {
        .onMIDIMessage = writeMessageFromUSBHost,
        .onSysexChunk = onSysexChunk,
        .userData = &usbHost
    };
    usbHost.init(USB_HOST_DP_GPIO, usbHostParserConfig);

    mainLED.on();

    while (true) {
        tuh_task();

        uartMidiPort.tick();
        usbDevice.tick();
        usbHost.tick();
    }

    noteLED.off();
    mainLED.off();

    return 0;
}
