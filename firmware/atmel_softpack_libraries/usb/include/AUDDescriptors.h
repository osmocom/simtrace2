/* ----------------------------------------------------------------------------
 *         ATMEL Microcontroller Software Support 
 * ----------------------------------------------------------------------------
 * Copyright (c) 2010, Atmel Corporation
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the disclaimer below.
 *
 * Atmel's name may not be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * DISCLAIMER: THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * ----------------------------------------------------------------------------
 */

/** \file
 *
 *  Definitions and classes for USB Audio class descriptors.
 *
 *  - For a USB %device:
 *     -# When declaring an Audio-specific descriptor, use the descriptor types
 *        and subtypes defined in this unit (see "USB Audio descriptor types"
 *        and "USB Audio descriptor subtypes").
 *     -# When declaring the %device descriptor of a USB %audio %device, use
 *        "USB Audio device descriptor values" defined here.
 *     -# Declare an AUDStreamingInterfaceDescriptor instance as part of the
 *        configuration descriptors returned by a USB %audio %device.
 *     -# Declare a AUDHeaderDescriptor as part of the configuration
 *        descriptors of the %device.
 *     -# Declare an AUDFeatureUnitDescriptor instance as part of the
 *        configuration descriptors returned by a USB %audio %device.
 *     -# Declare an instance of AUDFormatTypeOneDescriptor as part of the
 *        configuration descriptors of an audio device.
 */

#ifndef _AUDDESCRIPTORS_H_
#define _AUDDESCRIPTORS_H_
/** \addtogroup usb_audio
 *@{
 */

/*----------------------------------------------------------------------------
 *         Includes
 *----------------------------------------------------------------------------*/
 
#include <stdint.h>
 
/*----------------------------------------------------------------------------
 *         Definitions
 *----------------------------------------------------------------------------*/

/** \addtogroup usb_audio_desc_type USB Audio descriptor types
 *      @{
 * This section lists the available types for USB audio-specific descriptors.
 * - \ref AUDGenericDescriptor_DEVICE
 * - \ref AUDGenericDescriptor_CONFIGURATION
 * - \ref AUDGenericDescriptor_STRING
 * - \ref AUDGenericDescriptor_INTERFACE
 * - \ref AUDGenericDescriptor_ENDPOINT
 */

/** Descriptor gives detail about the whole device.*/
#define AUDGenericDescriptor_DEVICE                     0x21
/** Descriptor gives detail about a configuration.*/
#define AUDGenericDescriptor_CONFIGURATION              0x22
/** Descriptor gives detail about a string.*/
#define AUDGenericDescriptor_STRING                     0x23
/** Descriptor gives detail about an interface.*/
#define AUDGenericDescriptor_INTERFACE                  0x24
/** Descriptor gives detail about an endpoint.  */
#define AUDGenericDescriptor_ENDPOINT                   0x25
/**     @}*/

/** \addtogroup usb_audio_desc_subtype USB Audio descriptor subtypes
 *      @{
 * This section lists the possible subtypes for USB audio-specific descriptors.
 * - \ref AUDGenericDescriptor_HEADER
 * - \ref AUDGenericDescriptor_INPUTTERMINAL
 * - \ref AUDGenericDescriptor_OUTPUTTERMINAL
 * - \ref AUDGenericDescriptor_MIXERUNIT
 * - \ref AUDGenericDescriptor_SELECTORUNIT
 * - \ref AUDGenericDescriptor_FEATUREUNIT
 * - \ref AUDGenericDescriptor_PROCESSINGUNIT
 * - \ref AUDGenericDescriptor_EXTENSIONUNIT
 */
/** Header descriptor subtype.*/
#define AUDGenericDescriptor_HEADER                     0x01
/** Input terminal descriptor subtype.*/
#define AUDGenericDescriptor_INPUTTERMINAL              0x02
/** Output terminal descriptor subtype.*/
#define AUDGenericDescriptor_OUTPUTTERMINAL             0x03
/** Mixer unit descriptor subtype.*/
#define AUDGenericDescriptor_MIXERUNIT                  0x04
/** Selector unit descriptor subtype.*/
#define AUDGenericDescriptor_SELECTORUNIT               0x05
/** Feature unit descriptor subtype.*/
#define AUDGenericDescriptor_FEATUREUNIT                0x06
/** Processing unit descriptor subtype.*/
#define AUDGenericDescriptor_PROCESSINGUNIT             0x07
/** Extension unit descriptor subtype.*/
#define AUDGenericDescriptor_EXTENSIONUNIT              0x08
/**     @}*/


/** \addtogroup usb_audio_desc USB Audio Descriptors
 *      @{
 * USB Audio device descriptor values
 *
 * This section lists the class, subclass & protocol codes that a USB audio
 * device should display in its device descriptor.
 * - \ref AUDDeviceDescriptor_CLASS
 * - \ref AUDDeviceDescriptor_SUBCLASS
 * - \ref AUDDeviceDescriptor_PROTOCOL
 */
/** Class code for a USB audio device. */
#define AUDDeviceDescriptor_CLASS                       0x00
/** Subclass code for a USB audio device. */
#define AUDDeviceDescriptor_SUBCLASS                    0x00
/** Protocol code for a USB audio device. */
#define AUDDeviceDescriptor_PROTOCOL                    0x00
/**     @}*/


/**
 *  \addtogroup usb_audio_stream_if USB Audio streaming interface codes
 *      @{
 * This section lists the class, subclass and protocol codes that an Audio
 * Streaming interface should display in its descriptor.
 * - \ref AUDStreamingInterfaceDescriptor_CLASS
 * - \ref AUDStreamingInterfaceDescriptor_SUBCLASS
 * - \ref AUDStreamingInterfaceDescriptor_PROTOCOL
 */
/** Class code for an USB audio streaming interface.*/
#define AUDStreamingInterfaceDescriptor_CLASS               0x01

/** Subclass code for an audio streaming interface.*/
#define AUDStreamingInterfaceDescriptor_SUBCLASS            0x02

/** Protocol code for an audio streaming interface.*/
#define AUDStreamingInterfaceDescriptor_PROTOCOL            0x00
/**     @}*/

/**
 * \addtogroup usb_audio_stream_subtype USB Audio streaming descriptor subtypes
 *      @{
 * This section lists the possible subtypes for audio-specific descriptor
 * appended to an Audio Streaming interface.
 * - \ref AUDStreamingInterfaceDescriptor_GENERAL
 * - \ref AUDStreamingInterfaceDescriptor_FORMATTYPE
 * - \ref AUDStreamingInterfaceDescriptor_FORMATSPECIFIC
 */
/** General descriptor subtype.*/
#define AUDStreamingInterfaceDescriptor_GENERAL             0x01

/** Format type descriptor subtype.*/
#define AUDStreamingInterfaceDescriptor_FORMATTYPE          0x02

/** Format specific descriptor subtype.*/
#define AUDStreamingInterfaceDescriptor_FORMATSPECIFIC      0x03
/**     @}*/


/** \addtogroup usb_audio_ctrl_if USB Audio Control Interface
 *      @{
 * This section lists the class, subclass and protocol codes that a USB Audio
 * Control interface descriptor should display.
 * - \ref AUDControlInterfaceDescriptor_CLASS
 * - \ref AUDControlInterfaceDescriptor_SUBCLASS
 * - \ref AUDControlInterfaceDescriptor_PROTOCOL
 */
/** Class code for an audio control interface. */
#define AUDControlInterfaceDescriptor_CLASS             0x01
/** Subclass code for an audio control interface. */
#define AUDControlInterfaceDescriptor_SUBCLASS          0x01
/** Protocol code for an audio control interface. */
#define AUDControlInterfaceDescriptor_PROTOCOL          0x00
/**     @}*/


/** Descriptor subtype for an Audio data endpoint. */
#define AUDDataEndpointDescriptor_SUBTYPE                   0x01

/** \addtogroup usb_audio_lock_delay USB Audio Lock delay units
 *      @{
 * This section lists the valid lock delay unit types.
 * - \ref AUDDataEndpointDescriptor_MILLISECONDS
 * - \ref AUDDataEndpointDescriptor_PCMSAMPLES
 */
/** Lock delay is expressed in milliseconds. */
#define AUDDataEndpointDescriptor_MILLISECONDS              1
/** Lock delay is expressed in decoded PCM samples. */
#define AUDDataEndpointDescriptor_PCMSAMPLES                2
/**     @}*/


/** \addtogroup usb_audio_class_ver USB Audio class releases
 *      @{
 * This section lists the existing versions of the Audio class specification.
 * - \ref AUDHeaderDescriptor_AUD1_00
 */
 
/** Indentifies the USB audio specification release 1.00.*/
#define AUDHeaderDescriptor_AUD1_00                 0x0100
/**     @}*/

#define AUDTerminalDescriptor_USBSTREAMING          0x0101
#define AUDTerminalDescriptor_MICROPHONE            0x0201
#define AUDTerminalDescriptor_HANDSETIN             0x0401
#define AUDTerminalDescriptor_SPEAKERPHONE          0x0403
#define AUDTerminalDescriptor_LINEIN                0x0501

/** \addtogroup usb_audio_in_term USB Audio Input terminal types
 *  @{
 * This section lists the available types for an Input terminal.
 * - \ref AUDInputTerminalDescriptor_USBSTREAMING
 * - \ref AUDInputTerminalDescriptor_MICROPHONE
 * - \ref AUDInputTerminalDescriptor_SPEAKERPHONE
 * - \ref AUDInputTerminalDescriptor_LINEIN
 */

/** A terminal receiving its data from a USB isochronous endpoint.*/
#define AUDInputTerminalDescriptor_USBSTREAMING         0x0101
/** A terminal sampling data from a microphone.*/
#define AUDInputTerminalDescriptor_MICROPHONE           0x0201
/** A terminal sampling data from a Handset In.*/
#define AUDInputTerminalDescriptor_HANDSETIN            0x0401
/** A terminal sampling data from a speakerphone*/
#define AUDInputTerminalDescriptor_SPEAKERPHONE         0x0403
/** A terminal sampling data from a Phone Line In.*/
#define AUDInputTerminalDescriptor_LINEIN               0x0501
/** @}*/

/** \addtogroup usb_audio_ch_loc USB Audio Channel spatial locations
 *  @{
 * This section lists the possible spatial locations for audio channels.
 * - \ref AUDInputTerminalDescriptor_LEFTFRONT
 * - \ref AUDInputTerminalDescriptor_RIGHTFRONT
 * - \ref AUDInputTerminalDescriptor_CENTERFRONT
 */
/** Front left channel.*/
#define AUDInputTerminalDescriptor_LEFTFRONT            (1 << 0)
/** Front right channel.*/
#define AUDInputTerminalDescriptor_RIGHTFRONT           (1 << 1)
/** Front center channel.*/
#define AUDInputTerminalDescriptor_CENTERFRONT          (1 << 2)
/** @}*/


/** \addtogroup usb_audio_out_term USB Audio Output terminal types
 *  @{
 * This section lists the available types for an output terminal.
 * - \ref AUDOutputTerminalDescriptor_USBTREAMING
 * - \ref AUDOutputTerminalDescriptor_SPEAKER
 * - \ref AUDOutputTerminalDescriptor_HANDSETOUT
 * - \ref AUDOutputTerminalDescriptor_LINEOUT
 */
/** A terminal sending data through USB isochronous endpoint.*/
#define AUDOutputTerminalDescriptor_USBTREAMING         0x0101
/** A terminal sending data to a USB host through an Isochronous endpoint.*/
#define AUDOutputTerminalDescriptor_SPEAKER             0x0301
/** A terminal sending data to Handset Out.*/
#define AUDOutputTerminalDescriptor_HANDSETOUT          0x0401
/** A terminal sending data to Phone Line Out.*/
#define AUDOutputTerminalDescriptor_LINEOUT             0x0501
/** @}*/

/** \addtogroup usb_audio_ch_ctrl USB Audio Channel controls
 *      @{
 * This section lists the available controls for each channel of the audio
 * interface. Each channel can have any control combination; simply perform
 * a bitwise OR ('|') to combine several values.
 * -# \ref AUDFeatureUnitDescriptor_MUTE
 * -# \ref AUDFeatureUnitDescriptor_VOLUME
 * -# \ref AUDFeatureUnitDescriptor_BASS
 * -# \ref AUDFeatureUnitDescriptor_MID
 * -# \ref AUDFeatureUnitDescriptor_TREBLE     
 */
/** Channel mute control. */
#define AUDFeatureUnitDescriptor_MUTE               (1 << 0)
/** Channel volume control. */
#define AUDFeatureUnitDescriptor_VOLUME             (1 << 1)
/** Channel bass control. */
#define AUDFeatureUnitDescriptor_BASS               (1 << 2)
/** Channel middle control. */
#define AUDFeatureUnitDescriptor_MID                (1 << 3)
/** Channel treble control. */
#define AUDFeatureUnitDescriptor_TREBLE             (1 << 4)
/**     @}*/


/** Format type for a format type I descriptor. */
#define AUDFormatTypeOneDescriptor_FORMATTYPEONE        0x01

/** AUDFormatTypeOneDescriptor_PCM - PCM format. */
#define AUDFormatTypeOneDescriptor_PCM                  0x0001

/** Indicates the sampling frequency can have any value in the provided range. */
#define AUDFormatTypeOneDescriptor_CONTINUOUS           0


/*----------------------------------------------------------------------------
 *         Types
 *----------------------------------------------------------------------------*/

#ifdef __ICCARM__          /* IAR */
#pragma pack(1)            /* IAR */
#define __attribute__(...) /* IAR */
#endif                     /* IAR */

/**
 * \typedef AUDStreamingInterfaceDescriptor
 * \brief Provides additional information about an audio streaming interface to
 *        the USB host.
 */
typedef struct _AUDStreamingInterfaceDescriptor {

    /** Size of descriptor in bytes.*/
    uint8_t bLength;
    /** Descriptor type (AUDGenericDescriptor_INTERFACE).*/
    uint8_t bDescriptorType;
    /** Descriptor subtype (AUDStreamingInterfaceDescriptor_GENERAL).*/
    uint8_t bDescriptorSubType;
    /** Terminal ID to which the interface is connected.*/
    uint8_t bTerminalLink;
    /** Delay introduced by the data path, in number of frames.*/
    uint8_t bDelay;
    /** Audio data format used by this interface.*/
    uint16_t wFormatTag;

} __attribute__ ((packed)) AUDStreamingInterfaceDescriptor; /* GCC */

/**
 *  \typedef AUDEndpointDescriptor
 *  \brief Modified endpoint descriptor with two additional fields, with are
 *         USB audio specific.
 */
typedef struct _AUDEndpointDescriptor {

   /** Size of the descriptor in bytes. */
   uint8_t bLength;
   /** Descriptor type (USBGenericDescriptor_ENDPOINT). */
   uint8_t bDescriptorType;
   /** Address and direction of the endpoint. */
   uint8_t bEndpointAddress;
   /** Endpoint type and additional characteristics (for isochronous endpoints). */
   uint8_t bmAttributes;
   /** Maximum packet size (in bytes) of the endpoint. */
   uint16_t wMaxPacketSize;
   /** Polling rate of the endpoint. */
   uint8_t bInterval;
   /** Refresh rate for a feedback endpoint. */
   uint8_t bRefresh;
   /** Address of the associated feedback endpoint if any. */
   uint8_t bSyncAddress;
                                   
} __attribute__ ((packed)) AUDEndpointDescriptor; /* GCC */

/**
 * \typedef AUDDataEndpointDescriptor
 * \brief Gives additional information about an USB endpoint used to transmit audio
 *        data to or from the host.
 */
typedef struct _AUDDataEndpointDescriptor {

    /** Size of descriptor in bytes. */
    uint8_t bLength;
    /** Descriptor type
        (\ref AUDGenericDescriptor_ENDPOINT). */
    uint8_t bDescriptorType;
    /** Descriptor subtype
        (\ref AUDDataEndpointDescriptor_SUBTYPE).  */
    uint8_t bDescriptorSubType;
    /** Indicates available controls and requirement on packet sizes. */
    uint8_t bmAttributes;
    /** Indicates the units of the wLockDelay fields.
        \sa usb_audio_lock_delay USB Audio Lock delay units */
    uint8_t bLockDelayUnits;
    /** Time it takes for the endpoint to lock its internal clock circuitry. */
    uint16_t wLockDelay;

} __attribute__ ((packed)) AUDDataEndpointDescriptor; /* GCC */

/**
 * \typedef AUDHeaderDescriptor
 * \brief Groups the various audio interfaces to display one single function to
 *        the USB host. Subclass this structure to add a particular number of
 *        slave interface descriptors.
 */
typedef struct _AUDHeaderDescriptor {

    /** Size of descriptor in bytes.*/
    uint8_t bLength;
    /** Descriptor type (\ref AUDGenericDescriptor_INTERFACE).*/
    uint8_t bDescriptorType;
    /** Descriptor subtype (\ref AUDGenericDescriptor_HEADER).*/
    uint8_t bDescriptorSubType;
    /** Audio class release number in BCD format
     * \sa usb_audio_class_ver USB Audio class releases */
    uint16_t bcdADC;
    /** Length of all descriptors used to qualify the Audio Control interface.*/
    uint16_t wTotalLength;
    /** Number of Streaming interfaces contained in this collection.*/
    uint8_t bInCollection;

} __attribute__ ((packed)) AUDHeaderDescriptor; /* GCC */

/**
 * \typedef AUDInputTerminalDescriptor
 * \brief Describes an input of a USB audio device.
 */
typedef struct _AUDInputTerminalDescriptor {

    /** Size of descriptor in bytes.*/
    uint8_t bLength;
    /** Descriptor type (AUDGenericDescriptor_INTERFACE).*/
    uint8_t bDescriptorType;
    /** Descriptor subtype (AUDGenericDescriptor_INPUTTERMINAL).*/
    uint8_t bDescriptorSubType;
    /** ID of the terminal in the audio function.*/
    uint8_t bTerminalID;
    /** Terminal type.
     * \sa usb_audio_in_term USB Audio Input terminal types
     */
    uint16_t wTerminalType;
    /** ID of the output terminal to which this input terminal is associated.*/
    uint8_t bAssocTerminal;
    /** Number of logical output channels in this terminal.*/
    uint8_t bNrChannels;
    /** Spatial configuration of the logical channels.*/
    uint16_t wChannelConfig;
    /** Index of a string descriptor for the first logical channel.*/
    uint8_t iChannelNames;
    /** Index of a string descriptor for this terminal. */
    uint8_t iTerminal;

} __attribute__ ((packed)) AUDInputTerminalDescriptor; /* GCC */

/**
 * \typedef AUDOutputTerminalDescriptor
 * \brief Describes an output of the USB audio function.
 */
typedef struct _AUDOutputTerminalDescriptor {

    /** Size of descriptor in bytes.*/
    uint8_t bLength;
    /** Descriptor type (\ref AUDGenericDescriptor_INTERFACE).*/
    uint8_t bDescriptorType;
    /** Descriptor subtype (\ref AUDGenericDescriptor_OUTPUTTERMINAL).  */
    uint8_t bDescriptorSubType;
    /** Identifier for this terminal.*/
    uint8_t bTerminalID;
    /** Terminal type.
     *  \sa "USB Audio Output terminal types" */
    uint16_t wTerminalType;
    /** Identifier of the associated input terminal.*/
    uint8_t bAssocTerminal;
    /** Identifier of the unit or terminal to which this terminal
     *  is connected.*/
    uint8_t bSourceID;
    /** Index of a string descriptor for this terminal.*/
    uint8_t iTerminal;

} __attribute__ ((packed)) AUDOutputTerminalDescriptor; /* GCC */

/**
 * \typedef AUDFeatureUnitDescriptor
 * \brief Describes available controls for each channel of the unit or terminal
 *        it is connected to.
 *
 * This type must be augmented with the relevant number
 * of bmaControls fields and the iFeature field.
 **/
typedef struct _AUDFeatureUnitDescriptor {

    /** Size of descriptor in bytes.*/
    uint8_t bLength;
    /** Descriptor type (AUDGenericDescriptor_INTERFACE). */
    uint8_t bDescriptorType;
    /** Descriptor subtype (AUDGenericDescriptor_FEATURE). */
    uint8_t bDescriptorSubType; 
    /** Identifier of this feature unit. */
    uint8_t bUnitID;
    /** Identifier of the unit or terminal this feature unit is connected to. */
    uint8_t bSourceID;
    /** Size in bytes of a channel controls field. */
    uint8_t bControlSize;

} __attribute__ ((packed)) AUDFeatureUnitDescriptor; /* GCC */

/**
 * \typedef AUDFormatTypeOneDescriptor
 * \brief Describes an audio data stream that is constructed on a sample-by-sample
 *        basis.
 *
 * This type must be augmented with either the continuous sampling frequency range
 * (if bSamFreqType = <\ref AUDFormatTypeOneDescriptor_CONTINUOUS>)
 * or with an array containing the possible discrete frequencies.
 */
typedef struct _AUDFormatTypeOneDescriptor {

    /** Size of descriptor in bytes. */
    uint8_t bLength;
    /** Descriptor type (\ref AUDGenericDescriptor_INTERFACE). */
    uint8_t bDescriptorType;
    /** Descriptor subtype (\ref AUDStreamingInterfaceDescriptor_FORMATTYPE).*/
    uint8_t bDescriptorSubType;
    /** Format type (\ref AUDFormatTypeOneDescriptor_FORMATTYPEONE).*/
    uint8_t bFormatType;
    /** Number of physical channels in the audio stream.*/
    uint8_t bNrChannels;
    /** Number of bytes occupied by one audio subframe.*/
    uint8_t bSubFrameSize;
    /** Number of bits effectively used in an audio subframe.*/
    uint8_t bBitResolution;
    /** Number of supported discrete sampling frequencies, or
     *  \ref AUDFormatTypeOneDescriptor_CONTINUOUS.*/
    uint8_t bSamFreqType;

} __attribute__ ((packed)) AUDFormatTypeOneDescriptor; /* GCC */

#ifdef __ICCARM__          /* IAR */
#pragma pack()             /* IAR */
#endif                     /* IAR */

/*----------------------------------------------------------------------------
 *         Functions
 *----------------------------------------------------------------------------*/



/**@}*/
#endif /* _AUDDESCRIPTORS_H_ */
