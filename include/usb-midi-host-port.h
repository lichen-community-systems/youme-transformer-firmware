#pragma once

#include "pio_usb.h"
#include "tusb.h"
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
    static constexpr size_t USB_MIDI_PACKET_SIZE = 4;

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
        uint32_t bytesWritten = tuh_midi_stream_write(0, 0, buffer, numBytes);

        if (bytesWritten < numBytes) {
            this->numTXBytesDropped += (numBytes - bytesWritten);
        }

        tuh_midi_write_flush(0);
    }
};

static int32_t mounts = 0;


void tuh_midi_mount_cb(uint8_t idx,
    const tuh_midi_mount_cb_t* mount_cb_data) {
    (void) idx;
    (void) mount_cb_data;
    mounts++;
}

// Invoked when device with hid interface is un-mounted
void tuh_midi_umount_cb(uint8_t idx) {
    (void) idx;
    mounts--;
}

void tuh_midi_rx_cb(uint8_t idx, uint32_t xferredBytes) {
    (void) xferredBytes;
    uint8_t* readBuffer = USBMidiHostPort_stateSingleton->readBuffer;
    size_t readBufferSize = USBMidiHostPort_stateSingleton->readBufferSize;
    struct sig_MidiParser* midiParser = USBMidiHostPort_stateSingleton->midiParser;

    while(tuh_midi_packet_read(idx, readBuffer) > 0) {
        sig_MidiParser_feedBytes(midiParser, readBuffer, readBufferSize);
    }
}

void tuh_midi_tx_cb(uint8_t idx, uint32_t num_bytes) {
    (void)idx;
    (void)num_bytes;
}

