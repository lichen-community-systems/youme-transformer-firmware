#pragma once

#include "pio_usb.h"
#include "tusb.h"
#include "class/midi/midi_host.h"
#include "midi-port.h"

struct USBMidiHostPortCallbackState {
    uint8_t* readBuffer;
    size_t readBufferSize;
    struct sig_MidiParser* midiParser;
};

static USBMidiHostPortCallbackState* USBMidiHostPort_stateSingleton;

template<size_t messageBufferSize = 4,
    size_t sysexBufferSize = 32,
    size_t readBufferSize = 4>
class USBMidiHostPort: public MidiPort<
    messageBufferSize, sysexBufferSize, readBufferSize> {
public:
    USBMidiHostPortCallbackState callbackState;

    void init(uint8_t usbDPPin,
        MidiParserConfig parserConfig = MidiParserConfig()) {
        this->initParser(parserConfig);
        pio_usb_configuration_t pioUSBConfig = PIO_USB_DEFAULT_CONFIG;
        pioUSBConfig.pin_dp = usbDPPin;
        tuh_configure(1,
            TUH_CFGID_RPI_PIO_USB_CONFIGURATION, &pioUSBConfig);
        tuh_init(1);

        setupCallbackState();
    }

    void setupCallbackState() {
        callbackState.midiParser = &this->midiParser;
        callbackState.readBuffer = this->readBuffer;
        callbackState.readBufferSize = readBufferSize;

        USBMidiHostPort_stateSingleton = &this->callbackState;
    }

    uint8_t* getReadBuffer() {
        return this->readBuffer;
    }

    void tick() {
        tuh_task();
    }

    void write(uint8_t* buffer, uint32_t numBytes) {
        // TODO: Handle virtual cables correctly.
        // For now, just write MIDI data to the first device's
        // first virtual cable.
        uint32_t bytesWritten = tuh_midi_stream_write(0, 0, buffer, numBytes);

        if (bytesWritten < numBytes) {
            this->numTXBytesDropped += (numBytes - bytesWritten);
        }

        tuh_midi_write_flush(0);
    }
};

inline void readCable(uint8_t idx, uint8_t cableNum,
    USBMidiHostPortCallbackState* state) {
    size_t bytesRead = tuh_midi_stream_read(idx, &cableNum, state->readBuffer,
        state->readBufferSize);

    while (bytesRead > 0) {
        sig_MidiParser_feedBytes(state->midiParser, state->readBuffer,
            bytesRead);
        bytesRead = tuh_midi_stream_read(idx, &cableNum, state->readBuffer,
            state->readBufferSize);
    }
}

void tuh_midi_rx_cb(uint8_t idx, uint32_t xferredBytes) {
    (void) xferredBytes;

    // TODO: Handle virtual cables correctly.
    // For now, just read MIDI data from all virtual cables.
    uint8_t numCables = tuh_midi_get_rx_cable_count(idx);
    for (uint8_t i = 0; i < numCables; i++) {
        readCable(idx, i, USBMidiHostPort_stateSingleton);
    }
}
