/* ----------------------------------------------------------------------------
 *         ATMEL Microcontroller Software Support
 * ----------------------------------------------------------------------------
 * Copyright (c) 2008, Atmel Corporation
 * Copyright (c) 2018, Kevin Redon <kredon@sysmocom.de>
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
 * \addtogroup usbd_interface
 *@{
 */

/*------------------------------------------------------------------------------
 *      Headers
 *------------------------------------------------------------------------------*/

#include <board.h>

#include "USBDDriver.h"
#include "USBD.h"
#include "USBD_HAL.h"

#include <string.h>

/*------------------------------------------------------------------------------
 *      Local variables
 *------------------------------------------------------------------------------*/

/** Default device driver instance, for all class drivers in USB Lib. */
static USBDDriver usbdDriver;

/*------------------------------------------------------------------------------
 *      Local functions
 *------------------------------------------------------------------------------*/

/**
 * Send a NULL packet
 */
static void TerminateCtrlInWithNull(void *pArg,
                                    uint8_t status,
                                    uint32_t transferred,
                                    uint32_t remaining)
{
    USBD_Write(0, /* Endpoint #0 */
               0, /* No data buffer */
               0, /* No data buffer */
               (TransferCallback) 0,
               (void *)  0);
}

/**
 * Configures the device by setting it into the Configured state and
 * initializing all endpoints.
 * \param pDriver  Pointer to a USBDDriver instance.
 * \param cfgnum  Configuration number to set.
 */
static void SetConfiguration(USBDDriver *pDriver, uint8_t cfgnum)
{
    USBEndpointDescriptor *pEndpoints[17];
    const USBConfigurationDescriptor *pConfiguration;

    /* Use different descriptor depending on device speed */
    TRACE_DEBUG("%s\n\r", "SetConfiguration");

    if (USBD_IsHighSpeed()) {

        pConfiguration = pDriver->pDescriptors->pHsConfiguration;
    }
    else {

        pConfiguration = pDriver->pDescriptors->pFsConfiguration[cfgnum-1];
    }

    /* Set & save the desired configuration */
    USBD_SetConfiguration(cfgnum);

    pDriver->cfgnum = cfgnum;
    pDriver->isRemoteWakeUpEnabled =
        ((pConfiguration->bmAttributes & 0x20) > 0);

    /* If the configuration is not 0, configure endpoints */
    if (cfgnum != 0) {

        /* Parse configuration to get endpoint descriptors */
        USBConfigurationDescriptor_Parse(pConfiguration, 0, pEndpoints, 0);

        /* Configure endpoints */
        int i = 0;
        while (pEndpoints[i] != 0) {

            USBD_ConfigureEndpoint(pEndpoints[i]);
            i++;
        }
    }
    /* Should be done before send the ZLP */
    USBDDriverCallbacks_ConfigurationChanged(cfgnum);

    /* Acknowledge the request */
    USBD_Write(0, /* Endpoint #0 */
               0, /* No data buffer */
               0, /* No data buffer */
               (TransferCallback) 0,
               (void *)  0);
}

/**
 * Sends the current configuration number to the host.
 * \param pDriver  Pointer to a USBDDriver instance.
 */
static void GetConfiguration(const USBDDriver *pDriver)
{
    USBD_Write(0, &(pDriver->cfgnum), 1, 0, 0);
}

/**
 * Sends the current status of the device to the host.
 * \param pDriver  Pointer to a USBDDriver instance.
 */
static void GetDeviceStatus(const USBDDriver *pDriver)
{
    static unsigned short data;
    const USBConfigurationDescriptor *pConfiguration;

    data = 0;
    /* Use different configuration depending on device speed */

    if (USBD_IsHighSpeed()) {

        pConfiguration = pDriver->pDescriptors->pHsConfiguration;
    }
    else {

        pConfiguration = pDriver->pDescriptors->pFsConfiguration[0];
    }

    /* Check current configuration for power mode (if device is configured) */

    if (pDriver->cfgnum != 0) {

        if (USBConfigurationDescriptor_IsSelfPowered(pConfiguration)) {

            data |= 1;
        }
    }

    /* Check if remote wake-up is enabled */

    if (pDriver->isRemoteWakeUpEnabled) {

        data |= 2;
    }

    /* Send the device status */

    USBD_Write(0, &data, 2, 0, 0);
}

/**
 * Sends the current status of an endpoints to the USB host.
 * \param bEndpoint  Endpoint number.
 */
static void GetEndpointStatus(uint8_t bEndpoint)
{
    static unsigned short data;

    data = 0;

    switch (USBD_HAL_Halt(bEndpoint, 0xFF)) {

        case USBD_STATUS_INVALID_PARAMETER: /* the endpoint not exists */
            USBD_Stall(0);
            break;

        case 1:
            data = 1;
        case 0:
            /* Send the endpoint status */
            USBD_Write(0, &data, 2, 0, 0);
            break;
    }
}

/**
 * Sends the requested USB descriptor to the host if available, or STALLs  the
 * request.
 * \param pDriver  Pointer to a USBDDriver instance.
 * \param type  Type of the requested descriptor
 * \param index  Index of the requested descriptor.
 * \param length  Maximum number of bytes to return.
 */
static void GetDescriptor(
    const USBDDriver *pDriver,
    uint8_t type,
    uint8_t indexRDesc,
    uint32_t length)
{
    const USBDeviceDescriptor *pDevice;
    const USBConfigurationDescriptor *pConfiguration;
    const USBDeviceQualifierDescriptor *pQualifier;
    const USBConfigurationDescriptor *pOtherSpeed;
    const USBGenericDescriptor **pStrings =
        (const USBGenericDescriptor **) pDriver->pDescriptors->pStrings;
    const USBGenericDescriptor *pString;
    uint8_t numStrings = pDriver->pDescriptors->numStrings;
    uint8_t terminateWithNull = 0;

    /* Use different set of descriptors depending on device speed */

    if (USBD_IsHighSpeed()) {

        TRACE_DEBUG("%s", "HS ");
        pDevice = pDriver->pDescriptors->pHsDevice;
        pConfiguration = pDriver->pDescriptors->pHsConfiguration;
        pQualifier = pDriver->pDescriptors->pHsQualifier;
        pOtherSpeed = pDriver->pDescriptors->pHsOtherSpeed;
    }
    else {

        TRACE_DEBUG("%s", "FS ");
        pDevice = pDriver->pDescriptors->pFsDevice;
        pConfiguration = pDriver->pDescriptors->pFsConfiguration[indexRDesc];
        pQualifier = pDriver->pDescriptors->pFsQualifier;
        pOtherSpeed = pDriver->pDescriptors->pFsOtherSpeed;
    }

    /* Check the descriptor type */

    switch (type) {

        case USBGenericDescriptor_DEVICE:
            TRACE_DEBUG_WP("Dev ");

            /* Adjust length and send descriptor */

            if (length > USBGenericDescriptor_GetLength((USBGenericDescriptor *) pDevice)) {

                length = USBGenericDescriptor_GetLength((USBGenericDescriptor *) pDevice);
            }
            USBD_Write(0, pDevice, length, 0, 0);
            break;

        case USBGenericDescriptor_CONFIGURATION:
            TRACE_DEBUG_WP("Cfg ");

            /* Adjust length and send descriptor */

            if (length > USBConfigurationDescriptor_GetTotalLength(pConfiguration)) {

                length = USBConfigurationDescriptor_GetTotalLength(pConfiguration);
                terminateWithNull = ((length % pDevice->bMaxPacketSize0) == 0);
            }
            USBD_Write(0,
                       pConfiguration,
                       length,
                       terminateWithNull ? TerminateCtrlInWithNull : 0,
                       0);
            break;

        case USBGenericDescriptor_DEVICEQUALIFIER:
            TRACE_DEBUG_WP("Qua ");

            /* Check if descriptor exists */

            if (!pQualifier) {

                USBD_Stall(0);
            }
            else {

                /* Adjust length and send descriptor */

                if (length > USBGenericDescriptor_GetLength((USBGenericDescriptor *) pQualifier)) {

                    length = USBGenericDescriptor_GetLength((USBGenericDescriptor *) pQualifier);
                }
                USBD_Write(0, pQualifier, length, 0, 0);
            }
            break;

        case USBGenericDescriptor_OTHERSPEEDCONFIGURATION:
            TRACE_DEBUG_WP("OSC ");

            /* Check if descriptor exists */

            if (!pOtherSpeed) {

                USBD_Stall(0);
            }
            else {

                /* Adjust length and send descriptor */

                if (length > USBConfigurationDescriptor_GetTotalLength(pOtherSpeed)) {

                    length = USBConfigurationDescriptor_GetTotalLength(pOtherSpeed);
                    terminateWithNull = ((length % pDevice->bMaxPacketSize0) == 0);
                }
                USBD_Write(0,
                           pOtherSpeed,
                           length,
                           terminateWithNull ? TerminateCtrlInWithNull : 0,
                           0);
            }
            break;

        case USBGenericDescriptor_STRING:
            TRACE_DEBUG_WP("Str%d ", indexRDesc);

            /* Check if descriptor exists */

            if (indexRDesc >= numStrings) {
                /* Sometimes descriptor string 0xee is requested.
                 * This is a mechanism used by Microsoft Windows to further identify the USB device.
                 * Instead of stalling, as is the original code, leading to an USB reset, we send an empty packet.
                 * I am not sure if sending an empty string would be better, but an empty packet seems sufficient.
                 */
                //USBD_Stall(0);
                USBD_Write(0, NULL, 0, 0, 0);
            }
            else {

                pString = pStrings[indexRDesc];

                /* Adjust length and send descriptor */

                if (length > USBGenericDescriptor_GetLength(pString)) {

                    length = USBGenericDescriptor_GetLength(pString);
                    terminateWithNull = ((length % pDevice->bMaxPacketSize0) == 0);
                }
                USBD_Write(0,
                           pString,
                           length,
                           terminateWithNull ? TerminateCtrlInWithNull : 0,
                           0);
            }
            break;

        default:
            TRACE_WARNING(
                      "USBDDriver_GetDescriptor: Unknown descriptor type (%d)\n\r",
                      type);
            USBD_Stall(0);
    }
}

/**
 * Sets the active setting of the given interface if the configuration supports
 * it; otherwise, the control pipe is STALLed. If the setting of an interface
 * changes.
 * \parma pDriver  Pointer to a USBDDriver instance.
 * \parma infnum  Interface number.
 * \parma setting  New active setting for the interface.
 */
static void SetInterface(
    USBDDriver *pDriver,
    uint8_t infnum,
    uint8_t setting)
{
    /* Make sure alternate settings are supported */

    if (!pDriver->pInterfaces) {

        USBD_Stall(0);
    }
    else {

        /* Change the current setting of the interface and trigger the callback */
        /* if necessary */
        if (pDriver->pInterfaces[infnum] != setting) {

            pDriver->pInterfaces[infnum] = setting;
            USBDDriverCallbacks_InterfaceSettingChanged(infnum, setting);
        }

        /* Acknowledge the request */

        USBD_Write(0, 0, 0, 0, 0);
    }
}

/**
 * Sends the currently active setting of the given interface to the USB
 * host. If alternate settings are not supported, this function STALLs the
 * control pipe.
 * \param pDriver  Pointer to a USBDDriver instance.
 * \param infnum  Interface number.
 */
static void GetInterface(
    const USBDDriver *pDriver,
    uint8_t infnum)
{
    /* Make sure alternate settings are supported, or STALL the control pipe */

    if (!pDriver->pInterfaces) {

        USBD_Stall(0);
    }
    else {

        /* Sends the current interface setting to the host */

        USBD_Write(0, &(pDriver->pInterfaces[infnum]), 1, 0, 0);
    }
}
/*------------------------------------------------------------------------------
 *      Exported functions
 *------------------------------------------------------------------------------*/

/**
 * Return USBDDriver instance pointer for global usage.
 */
USBDDriver *USBD_GetDriver(void)
{
    return &usbdDriver;
}

/**
 * Initializes a USBDDriver instance with a list of descriptors. If
 * interfaces can have multiple alternate settings, an array to store the
 * current setting for each interface must be provided.
 * \param pDriver  Pointer to a USBDDriver instance.
 * \param pDescriptors  Pointer to a USBDDriverDescriptors instance.
 * \param pInterfaces  Pointer to an array for storing the current alternate
 *                     setting of each interface (optional).
 */
void USBDDriver_Initialize(
    USBDDriver *pDriver,
    const USBDDriverDescriptors *pDescriptors,
    uint8_t *pInterfaces)
{

    pDriver->cfgnum = 0;
    pDriver->isRemoteWakeUpEnabled = 0;

    pDriver->pDescriptors = pDescriptors;
    pDriver->pInterfaces = pInterfaces;

    /* Initialize interfaces array if not null */

    if (pInterfaces != 0) {

        memset(pInterfaces, 0, sizeof(*pInterfaces));
    }
}

/**
 * Returns configuration descriptor list.
 * \param pDriver  Pointer to a USBDDriver instance.
 * \param cfgNum   Reserved.
 */
USBConfigurationDescriptor *USBDDriver_GetCfgDescriptors(
    USBDDriver *pDriver, uint8_t cfgNum)
{
    USBDDriverDescriptors *pDescList = (USBDDriverDescriptors *)pDriver->pDescriptors;
    USBConfigurationDescriptor *pCfg;

    if (USBD_HAL_IsHighSpeed() && pDescList->pHsConfiguration)
        pCfg = (USBConfigurationDescriptor *)pDescList->pHsConfiguration;
    else
        pCfg = (USBConfigurationDescriptor *)pDescList->pFsConfiguration[0];

    return pCfg;
}

/**
 * Handles the given request if it is standard, otherwise STALLs it.
 * \param pDriver  Pointer to a USBDDriver instance.
 * \param pRequest  Pointer to a USBGenericRequest instance.
 */
void USBDDriver_RequestHandler(
    USBDDriver *pDriver,
    const USBGenericRequest *pRequest)
{
    uint8_t cfgnum;
    uint8_t infnum;
    uint8_t eptnum;
    uint8_t setting;
    uint8_t type;
    uint8_t indexDesc;
    uint32_t length;
    uint32_t address;

    TRACE_DEBUG_WP("Std ");

    /* Check request code */
    switch (USBGenericRequest_GetRequest(pRequest)) {

        case USBGenericRequest_GETDESCRIPTOR:
            TRACE_DEBUG_WP("gDesc ");

            /* Send the requested descriptor */
            type = USBGetDescriptorRequest_GetDescriptorType(pRequest);
            indexDesc = USBGetDescriptorRequest_GetDescriptorIndex(pRequest);
            length = USBGenericRequest_GetLength(pRequest);
            GetDescriptor(pDriver, type, indexDesc, length);
            break;

        case USBGenericRequest_SETADDRESS:
            TRACE_DEBUG_WP("sAddr ");

            /* Sends a zero-length packet and then set the device address */
            address = USBSetAddressRequest_GetAddress(pRequest);
            USBD_Write(0, 0, 0, (TransferCallback) USBD_SetAddress, (void *) address);
            break;

        case USBGenericRequest_SETCONFIGURATION:
            TRACE_DEBUG_WP("sCfg ");

            /* Set the requested configuration */
            cfgnum = USBSetConfigurationRequest_GetConfiguration(pRequest);
            SetConfiguration(pDriver, cfgnum);
            break;

        case USBGenericRequest_GETCONFIGURATION:
            TRACE_DEBUG_WP("gCfg ");

            /* Send the current configuration number */
            GetConfiguration(pDriver);
            break;

        case USBGenericRequest_GETSTATUS:
            TRACE_DEBUG_WP("gSta ");

            /* Check who is the recipient */
            switch (USBGenericRequest_GetRecipient(pRequest)) {

                case USBGenericRequest_DEVICE:
                    TRACE_DEBUG_WP("Dev ");

                    /* Send the device status */
                    GetDeviceStatus(pDriver);
                    break;

                case USBGenericRequest_ENDPOINT:
                    TRACE_DEBUG_WP("Ept ");

                    /* Send the endpoint status */
                    eptnum = USBGenericRequest_GetEndpointNumber(pRequest);
                    GetEndpointStatus(eptnum);
                    break;

                default:
                    TRACE_WARNING(
                              "USBDDriver_RequestHandler: Unknown recipient (%d)\n\r",
                              USBGenericRequest_GetRecipient(pRequest));
                    USBD_Stall(0);
            }
            break;

        case USBGenericRequest_CLEARFEATURE:
            TRACE_DEBUG_WP("cFeat ");

            /* Check which is the requested feature */
            switch (USBFeatureRequest_GetFeatureSelector(pRequest)) {

                case USBFeatureRequest_ENDPOINTHALT:
                    TRACE_DEBUG_WP("Hlt ");

                    /* Unhalt endpoint and send a zero-length packet */
                    USBD_Unhalt(USBGenericRequest_GetEndpointNumber(pRequest));
                    USBD_Write(0, 0, 0, 0, 0);
                    break;

                case USBFeatureRequest_DEVICEREMOTEWAKEUP:
                    TRACE_DEBUG_WP("RmWU ");

                    /* Disable remote wake-up and send a zero-length packet */
                    pDriver->isRemoteWakeUpEnabled = 0;
                    USBD_Write(0, 0, 0, 0, 0);
                    break;

                default:
                    TRACE_WARNING(
                              "USBDDriver_RequestHandler: Unknown feature selector (%d)\n\r",
                              USBFeatureRequest_GetFeatureSelector(pRequest));
                    USBD_Stall(0);
            }
            break;

    case USBGenericRequest_SETFEATURE:
        TRACE_DEBUG_WP("sFeat ");

        /* Check which is the selected feature */
        switch (USBFeatureRequest_GetFeatureSelector(pRequest)) {

            case USBFeatureRequest_DEVICEREMOTEWAKEUP:
                TRACE_DEBUG_WP("RmWU ");

                /* Enable remote wake-up and send a ZLP */
                pDriver->isRemoteWakeUpEnabled = 1;
                USBD_Write(0, 0, 0, 0, 0);
                break;

            case USBFeatureRequest_ENDPOINTHALT:
                TRACE_DEBUG_WP("Halt ");
                /* Halt endpoint */
                USBD_Halt(USBGenericRequest_GetEndpointNumber(pRequest));
                USBD_Write(0, 0, 0, 0, 0);
                break;
            case USBFeatureRequest_OTG_B_HNP_ENABLE:
                    TRACE_DEBUG_WP("OTG_B_HNP_ENABLE ");
                    pDriver->otg_features_supported |=
                        1<<USBFeatureRequest_OTG_B_HNP_ENABLE;
                    USBD_Write(0, 0, 0, 0, 0);
                break;
            case USBFeatureRequest_OTG_A_HNP_SUPPORT:
                    TRACE_DEBUG_WP("OTG_A_HNP_SUPPORT ");
                    pDriver->otg_features_supported |=
                        1<<USBFeatureRequest_OTG_A_HNP_SUPPORT;
                    USBD_Write(0, 0, 0, 0, 0);
                break;
            case USBFeatureRequest_OTG_A_ALT_HNP_SUPPORT:
                    TRACE_DEBUG_WP("OTG_A_ALT_HNP_SUPPORT ");
                    pDriver->otg_features_supported |=
                        1<<USBFeatureRequest_OTG_A_ALT_HNP_SUPPORT;
                    USBD_Write(0, 0, 0, 0, 0);
                break;

            default:
                TRACE_WARNING(
                          "USBDDriver_RequestHandler: Unknown feature selector (%d)\n\r",
                          USBFeatureRequest_GetFeatureSelector(pRequest));
                USBD_Stall(0);
        }
        break;

    case USBGenericRequest_SETINTERFACE:
        TRACE_DEBUG_WP("sInterface ");

        infnum = USBInterfaceRequest_GetInterface(pRequest);
        setting = USBInterfaceRequest_GetAlternateSetting(pRequest);
        SetInterface(pDriver, infnum, setting);
        break;

    case USBGenericRequest_GETINTERFACE:
        TRACE_DEBUG_WP("gInterface ");

        infnum = USBInterfaceRequest_GetInterface(pRequest);
        GetInterface(pDriver, infnum);
        break;

    default:
        TRACE_WARNING(
                  "USBDDriver_RequestHandler: Unknown request code (%d)\n\r",
                  USBGenericRequest_GetRequest(pRequest));
        USBD_Stall(0);
    }
}


/**
 * Test if RemoteWakeUP feature is enabled
 * \param pDriver  Pointer to an USBDDriver instance.
 * \return 1 if remote wake up has been enabled by the host; otherwise, returns
 * 0
 */
uint8_t USBDDriver_IsRemoteWakeUpEnabled(const USBDDriver *pDriver)
{
    return pDriver->isRemoteWakeUpEnabled;
}

/**
 * Return OTG features supported
 * \param pDriver  Pointer to an USBDDriver instance.
 * \return the OTG features
 */
uint8_t USBDDriver_returnOTGFeatures(const USBDDriver *pDriver)
{
    return pDriver->otg_features_supported;
}

/**
 * Clear OTG features supported
 * \param pDriver  Pointer to an USBDDriver instance.
 * \return none
 */
void USBDDriver_clearOTGFeatures(USBDDriver *pDriver)
{
    pDriver->otg_features_supported = 0;
}

/**@}*/
