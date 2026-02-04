#pragma once

#include "tusb.h"
#include "midi-port.h"

template<size_t messageBufferSize = 4,
    size_t sysexBufferSize = 32,
    size_t readBufferSize = 4>
class USBMidiDevicePort: public MidiPort<
    messageBufferSize, sysexBufferSize, readBufferSize> {
public:
    void init(MidiParserConfig parserConfig = MidiParserConfig()) {
        tud_init(0);
        this->initParser(parserConfig);
    }

    void tick() {
        tud_task();
        read();
    }

    void read() {
        size_t bytesRead = tud_midi_stream_read(this->readBuffer,
            readBufferSize);

        while (bytesRead > 0) {
            sig_MidiParser_feedBytes(&this->midiParser,
                            this->readBuffer, bytesRead);
            bytesRead = tud_midi_stream_read(this->readBuffer,
                readBufferSize);
        }
    }

    void write(uint8_t* buffer, size_t numBytes) {
        if (!tud_midi_mounted()) {
            // Bytes that can't be written because the USB port
            // isn't mounted don't count as dropped.
            return;
        }

        // TODO: Handle virtual cables correctly.
        // For now, just write MIDI data to the first virtual cable.
        uint32_t bytesWritten = tud_midi_stream_write(
            0, buffer, numBytes);

        if (bytesWritten < numBytes) {
            this->numTXBytesDropped += (numBytes - bytesWritten);
        }
    }
};
