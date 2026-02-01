#ifndef SIG_MIDI_PARSER_H
#define SIG_MIDI_PARSER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

/**
 * MIDI Status Byte Constants
 *
 * These constants define commonly-used MIDI status bytes.
 * Channel messages use the format: MESSAGE_TYPE | channel (0-15)
 */

#define sig_MIDI_STATUS_NOTE_OFF 0x80
#define sig_MIDI_STATUS_NOTE_ON 0x90
#define sig_MIDI_STATUS_POLY_AFTERTOUCH 0xA0
#define sig_MIDI_STATUS_CONTROL_CHANGE 0xB0
#define sig_MIDI_STATUS_PROGRAM_CHANGE 0xC0
#define sig_MIDI_STATUS_CHANNEL_AFTERTOUCH 0xD0
#define sig_MIDI_STATUS_PITCH_BEND 0xE0
#define sig_MIDI_STATUS_SYSEX_START 0xF0
#define sig_MIDI_STATUS_SYSEX_END 0xF7
#define sig_MIDI_STATUS_MTC_QUARTER_FRAME 0xF1
#define sig_MIDI_STATUS_SONG_POSITION 0xF2
#define sig_MIDI_STATUS_SONG_SELECT 0xF3
#define sig_MIDI_STATUS_TUNE_REQUEST 0xF6
#define sig_MIDI_STATUS_TIMING_CLOCK 0xF8
#define sig_MIDI_STATUS_START 0xFA
#define sig_MIDI_STATUS_CONTINUE 0xFB
#define sig_MIDI_STATUS_STOP 0xFC
#define sig_MIDI_STATUS_ACTIVE_SENSING 0xFE
#define sig_MIDI_STATUS_SYSTEM_RESET 0xFF

/**
 * Helper macro to create a channel message status byte
 *
 * @param msgType One of the sig_MIDI_STATUS_* message type constants
 * @param channel Channel number (0-15)
 * @return Complete status byte for the specified message type and channel
 */
#define sig_MIDI_CHANNEL_MESSAGE(msgType, channel) ((msgType) | ((channel) & 0x0F))

/**
 * Helper macro to extract the message type from a status byte
 *
 * @param statusByte A MIDI status byte
 * @return The message type (high nibble)
 */
#define sig_MIDI_MESSAGE_TYPE(statusByte) ((statusByte) & 0xF0)

/**
 * Helper macro to extract the channel from a channel message status byte
 *
 * @param statusByte A channel message status byte (0x80-0xEF)
 * @return The channel number (0-15)
 */
#define sig_MIDI_CHANNEL(statusByte) ((statusByte) & 0x0F)

/**
 * @brief Checks a MIDI message to determine if it is a Note Off message.
 *
 * @param message the MIDI message buffer
 * @return true if the message is a Note Off message
 * @return false otherwise
 */
bool sig_MidiParser_isNoteOff(uint8_t* message);

/**
 * @brief Message callback which will be invoked when a complete,
 * non-SysEx MIDI message is parsed.
 */
typedef void (*sig_MidiParser_MessageCallback)(
    uint8_t* message, size_t size, void* userData);

/**
 * @brief A message callback that does nothing.
 * This can be used if you don't need to read
 * regular MIDI messages.
 */
void sig_MidiParser_noOpMessageCallback(
    uint8_t* message, size_t size, void* userData);

/**
 * @brief Sysex streaming callback.
 *
 * This callback will be invoked as chunks of SysEx data are received,
 * as well as when the final byte of the SysEx message is received.
 *
 * @param sysexData pointer to the SysEx data chunk
 * @param size size of the SysEx data chunk
 * @param isFinal true if this chunk is the final chunk of the SysEx message
 * @param userData user context pointer passed during parser initialization
 */
typedef void (*sig_MidiParser_SysexChunkCallback)(
    uint8_t* sysexData, size_t size, bool isFinal, void* userData);

/**
 * @brief A sysex chunk callback that does nothing.
 * This can be used if you don't need sysex support.
 */
void sig_MidiParser_noOpSysexCallback(
    uint8_t* sysexData, size_t size, bool isFinal,
    void* userData);

/**
 * Signaletic MIDI Parser
 *
 * Parses incoming MIDI bytes and invokes callbacks
 * when complete messages are ready. Includes streaming support
 * for System Exclusive messages.
 */
struct sig_MidiParser {
    sig_MidiParser_MessageCallback callback;
    sig_MidiParser_SysexChunkCallback sysexCallback;
    void* userData;

    uint8_t runningStatusByte;
    uint8_t* messageBuffer;
    size_t messageBufferSize;
    uint32_t msgLen;
    uint32_t expectedDataBytes;

    bool isParsingSysex;
    uint8_t* sysexBuffer;
    size_t sysexBufferSize;
    uint32_t sysexWriteIdx;
};

/**
 * @brief Resets the parser's state and clears all message buffers.
 */
void sig_MidiParser_reset(struct sig_MidiParser* self);

/**
 * @brief Initializes the MIDI parser.
 *
 * @param self the parser instance.
 * @param callback a callback function that will be called when non-SysEx MIDI messages are complete
 * @param sysexCallback a callback function that will be called when SysEx data chunks are received
 * @param userData user context that will be passed to all callbacks
 */
void sig_MidiParser_init(struct sig_MidiParser* self,
    uint8_t* messageBuffer, size_t messageBufferSize,
    uint8_t* sysexBuffer, size_t sysexBufferSize,
    sig_MidiParser_MessageCallback callback,
    sig_MidiParser_SysexChunkCallback sysexCallback,
    void* userData);

/**
 * @brief Determines the number of data bytes that will follow
 * the specified status byte.
 *
 * This function will return 0 for any messages that do not have
 * data bytes as well as system exclusive messages, which have
 * an indeterminate length.
 */
uint8_t sig_MidiParser_messageDataSize(uint8_t status);

/**
 * @brief Unsupported, non-API function.
 */
void sig_MidiParser_startSysexMessage(
    struct sig_MidiParser* self,
    uint8_t byte);

/**
 * @brief Unsupported, non-API function.
 */
void sig_MidiParser_endSysexMessage(
    struct sig_MidiParser* self);

/**
 * @brief unsupported, non-API function.
 */
void sig_MidiParser_handleSysexChunk(
    struct sig_MidiParser* self, uint8_t byte);

/**
 * @brief unsupported, non-API function.
 */
void sig_MidiParser_handleSysexByte(
    struct sig_MidiParser* self, uint8_t byte);

/**
 * @brief unsupported, non-API function.
 */
void sig_MidiParser_handleNonRealtimeStatusByte(struct sig_MidiParser* self,
    uint8_t byte);

/**
 * @brief unsupported, non-API function.
 */
void sig_MidiParser_handleCompleteMIDIMessage(struct sig_MidiParser* self);

/**
 * @brief Feeds a midi byte to the parser.
 *
 * @param self the parser instance
 * @param byte the MIDI byte
 */
void sig_MidiParser_feedByte(struct sig_MidiParser* self, uint8_t byte);

/**
 * @brief Feeds a buffer of MIDI bytes to the parser.
 *
 * @param self the parser instance
 * @param buffer pointer to a buffer of MIDI bytes to parse
 * @param len length of the buffer in bytes
 */
void sig_MidiParser_feedBytes(struct sig_MidiParser* self,
    uint8_t* buffer, size_t len);

/**
 * Reset the parser state (e.g., on disconnection or error recovery).
 *
 * @param parser the MIDI parser instance.
 */
void sig_MidiParser_reset(struct sig_MidiParser* parser);

#ifdef __cplusplus
}
#endif

#endif // SIG_MIDI_PARSER_H
