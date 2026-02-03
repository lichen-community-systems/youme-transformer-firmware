#pragma once

#include "tusb.h"
#include "midi-port.h"

template<size_t messageBufferSize = 4,
    size_t sysexBufferSize = 32,
    size_t readBufferSize = 4>
class USBMidiDevicePort: public MidiPort<
    messageBufferSize, sysexBufferSize, readBufferSize> {
public:
    static constexpr size_t USB_MIDI_PACKET_SIZE = 4;

    void init(MidiParserConfig parserConfig = MidiParserConfig()) {
        tud_init(0);
        this->initParser(parserConfig);
    }

    void tick() {
        tud_task();
        read();
    }

    void read() {
        while (tud_midi_available()) {
            (void) tud_midi_packet_read(this->readBuffer);
            sig_MidiParser_feedBytes(&this->midiParser,
                this->readBuffer, USB_MIDI_PACKET_SIZE);
        }
    }

    void write(uint8_t* buffer, uint32_t numBytes) {
        if (!tud_midi_mounted()) {
            // Bytes that can't be written because the USB port
            // isn't mounted don't count as dropped.
            return;
        }

        uint32_t bytesWritten = tud_midi_stream_write(
            0, buffer, numBytes);

        if (bytesWritten < numBytes) {
            this->numTXBytesDropped += (numBytes - bytesWritten);
        }
    }
};
