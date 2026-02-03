#include "midi-parser.h"

bool sig_MidiParser_isNoteOff(uint8_t* message) {
    uint8_t status = message[0];
    if (sig_MIDI_MESSAGE_TYPE(status) == sig_MIDI_STATUS_NOTE_OFF) {
        return true;
    }

    if (sig_MIDI_MESSAGE_TYPE(status) == sig_MIDI_STATUS_NOTE_ON &&
        message[2] == 0) {
        return true;
    }

    return false;
}

void sig_MidiParser_noOpMessageCallback(
    uint8_t* message, size_t size, void* userData) {
    (void)message;
    (void)size;
    (void)userData;
}

void sig_MidiParser_noOpSysexCallback(
    uint8_t* sysexData, size_t size, void* userData, bool isFinal) {
    (void)sysexData;
    (void)size;
    (void)userData;
    (void)isFinal;
}

static void sig_MidiParser_clearBuffer(uint8_t* buffer,
    size_t size) {
    for (size_t i = 0; i < size; ++i) {
        buffer[i] = 0;
    }
}

void sig_MidiParser_reset(struct sig_MidiParser* self) {
    self->runningStatusByte = 0;
    self->msgLen = 0;
    self->expectedDataBytes = 0;
    self->isParsingSysex = 0;
    self->sysexWriteIdx = 0;

    sig_MidiParser_clearBuffer(self->messageBuffer,
        self->messageBufferSize);
    sig_MidiParser_clearBuffer(self->sysexBuffer,
        self->sysexBufferSize);
}

void sig_MidiParser_init(struct sig_MidiParser* self,
    uint8_t* messageBuffer, size_t messageBufferSize,
    uint8_t* sysexBuffer, size_t sysexBufferSize,
    sig_MidiParser_MessageCallback callback,
    sig_MidiParser_SysexChunkCallback sysexCallback,
    void* userData) {
    self->messageBuffer = messageBuffer;
    self->messageBufferSize = messageBufferSize;
    self->sysexBuffer = sysexBuffer;
    self->sysexBufferSize = sysexBufferSize;

    self->callback = callback ? callback :
        sig_MidiParser_noOpMessageCallback;
    self->sysexCallback = sysexCallback ? sysexCallback :
        sig_MidiParser_noOpSysexCallback;
    self->userData = userData;

    sig_MidiParser_reset(self);
}

uint8_t sig_MidiParser_messageDataSize(uint8_t status) {
    // Not a status byte.
    if (status < 0x80) {
        return 0;
    }

    // Note on, Note off, Poly key pressure, Control change
    if (0x80 <= status && status <= 0xBF) {
        return 2;
    }

    // Program Change, Channel Pressure
    if (0xC0 <= status && status <= 0xDF) {
        return 1;
    }

    // Pitch Bend
    if (0xE0 <= status && status <= 0xEF) {
        return 2;
    }

    // System Messages
    switch (status) {
        // System Exclusive (variable length, handled specially)
        case 0xF0:
            return 0;
        // Time Code Quarter Frame
        case 0xF1:
            return 2;
        // Song Position Pointer
        case 0xF2:
            return 3;
        // Song Select
        case 0xF3:
            return 2;
        // Reserved
        case 0xF4:
        case 0xF5:
            return 0;
        // Tune Request
        case 0xF6:
            return 1;
        // System Exclusive End
        case 0xF7:
            return 0;
        // Timing Clock
        case 0xF8:
        // Reserved
        case 0xF9:
        // Start
        case 0xFA:
        // Continue
        case 0xFB:
        // Stop
        case 0xFC:
        // Reserved
        case 0xFD:
        // Active Sensing
        case 0xFE:
        // System Reset
        case 0xFF:
            return 0;
        default:
            return 0;
    }
}

void sig_MidiParser_startSysexMessage(
    struct sig_MidiParser* self,
    uint8_t byte) {
    self->isParsingSysex = true;
    self->sysexWriteIdx = 0;
    self->runningStatusByte = 0;

    if (self->sysexWriteIdx < self->sysexBufferSize) {
        self->sysexBuffer[self->sysexWriteIdx] = byte;
        self->sysexWriteIdx++;
    }
}

void sig_MidiParser_endSysexMessage(
    struct sig_MidiParser* self) {
    self->isParsingSysex = 0;
    self->runningStatusByte = 0;
    self->sysexCallback(self->sysexBuffer, self->sysexWriteIdx,
        self->userData, true);
    self->sysexWriteIdx = 0;
}

void sig_MidiParser_handleSysexChunk(
    struct sig_MidiParser* self, uint8_t byte) {
    self->sysexCallback(self->sysexBuffer, self->sysexWriteIdx,
        false, self->userData);
    self->sysexWriteIdx = 0;
    if (self->sysexWriteIdx < self->sysexBufferSize) {
        self->sysexBuffer[self->sysexWriteIdx] = byte;
        self->sysexWriteIdx++;
    }
}

void sig_MidiParser_handleSysexByte(
    struct sig_MidiParser* self, uint8_t byte) {
    if (self->sysexWriteIdx < self->sysexBufferSize) {
        self->sysexBuffer[self->sysexWriteIdx] = byte;
        self->sysexWriteIdx++;
    } else {
        sig_MidiParser_handleSysexChunk(self, byte);
    }

    if (byte == 0xF7) {
        sig_MidiParser_endSysexMessage(self);
    }
}

void sig_MidiParser_handleNonRealtimeStatusByte(struct sig_MidiParser* self,
    uint8_t byte) {
    self->runningStatusByte = byte;
    self->msgLen = 1;
    self->messageBuffer[0] = byte;
    self->expectedDataBytes = sig_MidiParser_messageDataSize(byte);

    if (self->expectedDataBytes == 0) {
        self->callback(self->messageBuffer, self->msgLen, self->userData);
        self->msgLen = 0;
        self->runningStatusByte = 0;
    }
}

void sig_MidiParser_handleCompleteMIDIMessage(struct sig_MidiParser* self) {
    self->callback(self->messageBuffer, self->msgLen, self->userData);

    // Set up in case of running status.
    self->msgLen = 1;
    self->messageBuffer[0] = self->runningStatusByte;
}

inline void sig_MidiParser_feedByte(struct sig_MidiParser* self,
    uint8_t byte) {
    // Real-time messages (0xF8-0xFF) can occur at any time,
    // and are only single byte messages, so can be dispatched immediately.
    if (byte >= 0xF8) {
        self->callback(&byte, 1, self->userData);
        return;
    }

    // We're already in the midst of receiving a SysEx message.
    if (self->isParsingSysex) {
        sig_MidiParser_handleSysexByte(self, byte);
        return;
    }

    // Start of new SysEx message.
    if (byte == 0xF0) {
        sig_MidiParser_startSysexMessage(self, byte);
        return;
    }

    // Status byte for any non-realtime message.
    if (byte & 0x80) {
        sig_MidiParser_handleNonRealtimeStatusByte(self, byte);
        return;
    }

    // This is a data byte
    if (self->runningStatusByte == 0) {
        // No running status, so we're in the midst of a message we
        // missed the start of. Ignore this byte.
        return;
    }

    // Add data byte
    if (self->msgLen < sizeof(self->messageBuffer)) {
        self->messageBuffer[self->msgLen] = byte;
        self->msgLen++;
    }

    if (self->msgLen == self->expectedDataBytes + 1) {
        sig_MidiParser_handleCompleteMIDIMessage(self);
    }
}

void sig_MidiParser_feedBytes(struct sig_MidiParser* self,
    uint8_t* buffer, size_t len) {
    for (size_t i = 0; i < len; ++i) {
        sig_MidiParser_feedByte(self, buffer[i]);
    }
}
