#pragma once

#include "midi-parser.h"

struct MidiParserConfig {
    sig_MidiParser_MessageCallback onMIDIMessage = sig_MidiParser_noOpMessageCallback;
    sig_MidiParser_SysexChunkCallback onSysexChunk = sig_MidiParser_noOpSysexCallback;
    void* userData = NULL;
};

template<size_t messageBufferSize,
         size_t sysexBufferSize,
         size_t readBufferSize>
class MidiPort {
public:
    uint8_t messageBuffer[messageBufferSize] = {0};
    uint8_t sysexBuffer[sysexBufferSize] = {0};
    uint8_t readBuffer[readBufferSize] = {0};
    size_t numTXBytesDropped = 0;
    struct sig_MidiParser midiParser;

    void initParser(MidiParserConfig config) {
        sig_MidiParser_init(
            &this->midiParser,
            this->messageBuffer,
            messageBufferSize,
            this->sysexBuffer,
            sysexBufferSize,
            config.onMIDIMessage,
            config.onSysexChunk,
            config.userData
        );
    }
};


