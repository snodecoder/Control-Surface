#pragma once

#include "MIDI_Interface.hpp"
#include "USBMIDI/USBMIDI.hpp"
#include <AH/Error/Error.hpp>
#include <AH/Teensy/TeensyUSBTypes.hpp>
#include <MIDI_Parsers/USBMIDI_Parser.hpp>

AH_DIAGNOSTIC_WERROR()

#if defined(TEENSYDUINO) && !defined(TEENSY_MIDIUSB_ENABLED)
#warning                                                                       \
    "Teensy: USB MIDI not enabled. Enable it from the Tools > USB Type menu."
#endif

#ifndef ARDUINO
#include <gmock-wrapper.h>
#endif

// If the main MCU has a USB connection or is a Teensy with MIDI USB type
#if defined(USBCON) || defined(TEENSY_MIDIUSB_ENABLED) || !defined(ARDUINO)

BEGIN_CS_NAMESPACE

/**
 * @brief   A class for MIDI interfaces sending MIDI messages over a USB MIDI
 *          connection.
 * 
 * On boards that support it, this will create a native MIDI over USB interface
 * using the platform-specific libraries (e.g. MIDIUSB for Arduino Leonardo, or 
 * the Core usbMIDI library for Teensy).  
 * On boards without native USB support, it'll fall back to a serial MIDI 
 * interface at the default @ref MIDI_BAUD "MIDI baud rate" on the UART 
 * connected to the Serial to USB chip. This can be used with custom 
 * MIDI over USB firmware for the Serial to USB chip.
 * 
 * @note    See @ref md_pages_MIDI-over-USB for more information.
 * 
 * @ingroup MIDIInterfaces
 */
class USBMIDI_Interface : public MIDI_Interface {
  public:
    /**
     * @brief   Construct a new USBMIDI_Interface.
     */
    USBMIDI_Interface() = default;

    using MIDIUSBPacket_t = USBMIDI::MIDIUSBPacket_t;

#ifndef ARDUINO
  public:
    MOCK_METHOD(void, writeUSBPacket,
                (Cable, uint8_t, uint8_t, uint8_t, uint8_t));
    MOCK_METHOD(MIDIUSBPacket_t, readUSBPacket, ());
    void flushUSB() {}

#else
  private:
    void writeUSBPacket(Cable cn, uint8_t cin, uint8_t d0, uint8_t d1,
                        uint8_t d2) {
        USBMIDI::write(cn.getRaw(), cin, d0, d1, d2);
    }
    MIDIUSBPacket_t readUSBPacket() { return USBMIDI::read(); }
    void flushUSB() { USBMIDI::flush(); }
#endif

  private:
    // MIDI send implementations
    void sendChannelMessageImpl(ChannelMessage) override;
    void sendSysCommonImpl(SysCommonMessage) override { /* TODO */ }
    void sendSysExImpl(SysExMessage) override;
    void sendRealTimeImpl(RealTimeMessage) override;

    /// Send a single SysEx starts or continues USB packet. Exactly 3 bytes are
    /// sent. The `data` pointer is not incremented.
    void sendSysExStartCont1(const uint8_t *data, Cable cable);
    /// Send as many SysEx starts or continues USB packets, such that the 
    /// remaining length is 3, 2 or 1 byte. The `data` pointer is incremented,
    /// and the `length` is decremented.
    /// The reason for leaving 3, 2 or 1 bytes remaining is so the message can 
    /// be finished using a SysExEnd USB packet, which has to have 3, 2 or 1
    /// bytes.
    void sendSysExStartCont(const uint8_t *&data, uint16_t &length, Cable cable);
    /// Send a SysExEnd USB packet. The `length` should be either 3, 2 or 1
    /// bytes, and the last byte of `data` should be a SysExEnd byte.
    void sendSysExEnd(const uint8_t *data, uint16_t length, Cable cable);

  public:
    /**
     * @brief   Try reading and parsing a single incoming MIDI message.
     * @return  Returns the type of the read message, or 
     *          `MIDIReadEvent::NO_MESSAGE` if no MIDI message was available.
     */
    MIDIReadEvent read();

    void update() override;

  protected:
    bool dispatchMIDIEvent(MIDIReadEvent event);

  public:
    /// Return the received channel message.
    ChannelMessage getChannelMessage() const {
        return parser.getChannelMessage();
    }
    /// Return the received real-time message.
    RealTimeMessage getRealTimeMessage() const {
        return parser.getRealTimeMessage();
    }
    /// Return the received system exclusive message.
    SysExMessage getSysExMessage() const { return parser.getSysExMessage(); }

  private:
    void handleStall() override {
        auto stallername = MIDIStaller::getNameNull(getStaller());
        ERROR(F("Not implemented (stalled by ") << stallername << ')', 0x1349);
        (void)stallername;
    }

  private:
    USBMIDI_Parser parser;
    uint8_t storedSysExData[3];
    uint8_t storedSysExLength = 0;
};

END_CS_NAMESPACE

// If the main MCU doesn't have a USB connection:
// Fall back on Serial connection at the hardware MIDI baud rate.
// (Can be used with HIDUINO or USBMidiKliK.)
#else

#include "SerialMIDI_Interface.hpp"

BEGIN_CS_NAMESPACE

/**
 * @brief   A class for MIDI interfaces sending MIDI messages over a USB MIDI
 *          connection.
 * 
 * @note    See @ref md_pages_MIDI-over-USB for more information.
 * 
 * @ingroup MIDIInterfaces
 */
class USBMIDI_Interface : public USBSerialMIDI_Interface {
  public:
    /**
     * @brief   Construct a new USBMIDI_Interface.
     */
    USBMIDI_Interface() : USBSerialMIDI_Interface(MIDI_BAUD) {}
};

END_CS_NAMESPACE

#endif

AH_DIAGNOSTIC_POP()