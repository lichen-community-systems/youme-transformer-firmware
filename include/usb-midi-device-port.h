#pragma once

#include <algorithm>
#include "tusb.h"
#include "class/midi/midi_device.h"
#include "midi-port.h"

template<size_t messageBufferSize = 4,
        size_t sysexBufferSize = 32,
        size_t readBufferSize = 128>
class USBMidiDevicePort: MidiPort<
    messageBufferSize, sysexBufferSize, readBufferSize> {
public:

    void init(MidiParserConfig parserConfig = MidiParserConfig()) {
        tusb_init();
        this->initParser(parserConfig);
    }

    void tick() {
        tud_task();
        read();
    }

    size_t read() {
        if (!tud_midi_mounted()) {
            return 0;
        }

        size_t numBytesRead = tud_midi_stream_read(
            this->readBuffer, readBufferSize);

        if (numBytesRead == 0) {
            return 0;
        }

        size_t bytesToFeed = std::min(numBytesRead,
            readBufferSize);

        sig_MidiParser_feedBytes(&this->midiParser,
            this->readBuffer, bytesToFeed);

        return numBytesRead;
    }

    size_t write(uint8_t* buffer, uint32_t numBytes) {
        if (!tud_midi_mounted()) {
            // Bytes that can't be written because the USB port
            // isn't mounted don't count as dropped.
            return 0;
        }

        uint32_t bytesWritten = tud_midi_stream_write(
            0, buffer, numBytes);

        if (bytesWritten < numBytes) {
            this->numTXBytesDropped += (numBytes - bytesWritten);
        }

        return bytesWritten;
    }
};
