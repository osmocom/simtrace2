/* ----------------------------------------------------------------------------
 *         ATMEL Microcontroller Software Support
 * ----------------------------------------------------------------------------
 * Copyright (c) 2009, Atmel Corporation
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

/**
 * \addtogroup external_component External Component
 *
 * \addtogroup at45d_module AT45 driver
 * \ingroup external_component
 * The AT45 Dataflash driver is based on the corresponding AT45 driver.
 * A AT45 instance has to be initialized using the Dataflash levle function
 * AT45_Configure(). AT45 Dataflash can be automatically detected using
 * the AT45_FindDevice() function. Then AT45 dataflash operations such as
 * read, write and erase DF can be launched using AT45_SendCommand function
 * with corresponding AT45 command set.
 *
 * \section Usage
 * <ul>
 * <li> Reads data from the At45 at the specified address using AT45D_Read().</li>
 * <li> Writes data on the At45 at the specified address using AT45D_Write().</li>
 * <li> Erases a page of data at the given address using AT45D_Erase().</li>
 * <li> Poll until the At45 has completed of corresponding operations using
 * AT45D_WaitReady().</li>
 * <li> Retrieves and returns the At45 current using AT45D_GetStatus().</li>
 * </ul>
 * Related files :\n
 * \ref at45d.c\n
 * \ref at45d.h.\n
 */
 /*@{*/
 /*@}*/


/**
  * \file
  *
  * Implementation of At45 driver.
  *
  */

/*----------------------------------------------------------------------------
 *        Headers
 *----------------------------------------------------------------------------*/

#include "board.h"

#include <assert.h>
#include <string.h>

/*----------------------------------------------------------------------------
 *        Local functions
 *----------------------------------------------------------------------------*/

/**
 * \brief Wait for transfer to finish calling the SPI driver ISR (interrupts are
 * disabled).
 *
 * \param pAt45  Pointer to an AT45 driver instance.
 */
static void AT45D_Wait( At45* pAt45 )
{
    assert( pAt45 != NULL ) ;

    /* Wait for transfer to finish */
    while ( AT45_IsBusy( pAt45 ) )
    {
        SPID_Handler( pAt45->pSpid ) ;
    }
}

/*----------------------------------------------------------------------------
 *        Global functions
 *----------------------------------------------------------------------------*/

/**
 * \brief Waits for the At45 to be ready to accept new commands.
 *
 * \param pAt45  Pointer to an AT45 driver instance.
 */
extern void AT45D_WaitReady( At45* pAt45 )
{
    uint8_t ready = 0;

    assert( pAt45 != NULL ) ;

    /* Poll device until it is ready. */
    while (!ready)
    {
        ready = AT45_STATUS_READY(AT45D_GetStatus(pAt45));
    }
}

/**
 * \brief Retrieves and returns the At45 current status, or 0 if an error happened.
 *
 * \param pAt45  Pointer to an AT45 driver instance.
 */
extern uint32_t AT45D_GetStatus( At45* pAt45 )
{
    uint32_t dwError ;
    uint8_t ucStatus ;

    assert( pAt45 != NULL ) ;

    /* Issue a status register read command */
    dwError = AT45_SendCommand( pAt45, AT45_STATUS_READ, 1, &ucStatus, 1, 0, 0, 0 ) ;
    assert( !dwError ) ;

    /* Wait for command to terminate */
    while ( AT45_IsBusy( pAt45 ) )
    {
        AT45D_Wait( pAt45 ) ;
    }

    return ucStatus ;
}

/**
 * \brief Reads data from the At45 inside the provided buffer. Since a continuous
 * read command is used, there is no restriction on the buffer size and read address.
 *
 * \param pAt45  Pointer to an AT45 driver instance.
 * \param pBuffer  Data buffer.
 * \param size  Number of bytes to read.
 * \param address  Address at which data shall be read.
 */
extern void AT45D_Read( At45* pAt45, uint8_t* pucBuffer, uint32_t dwSize, uint32_t dwAddress )
{
    uint32_t dwError ;

    assert( pAt45 != NULL ) ;
    assert( pucBuffer != NULL ) ;

    /* Issue a continuous read array command. */
    dwError = AT45_SendCommand( pAt45, AT45_CONTINUOUS_READ_LEG, 8, pucBuffer, dwSize, dwAddress, 0, 0 ) ;
    assert( !dwError ) ;

    /* Wait for the read command to execute. */
    while ( AT45_IsBusy( pAt45 ) )
    {
        AT45D_Wait( pAt45 ) ;
    }
}

/**
 * \brief Writes data on the At45 at the specified address. Only one page of
 * data is written that way; if the address is not at the beginning of the
 * page, the data is written starting from this address and wraps around to
 * the beginning of the page.
 *
 * \param pAt45  Pointer to an AT45 driver instance.
 * \param pucBuffer  Data buffer.
 * \param dwSize  Number of bytes to write.
 * \param dwAddress  Destination address on the At45.
 */
extern void AT45D_Write( At45* pAt45, uint8_t *pucBuffer, uint32_t dwSize, uint32_t dwAddress )
{
    uint8_t dwError ;

    assert( pAt45 != NULL ) ;
    assert( pucBuffer != NULL ) ;
    assert( dwSize <= pAt45->pDesc->pageSize ) ;

    /* Issue a page write through buffer 1 command. */
    dwError = AT45_SendCommand( pAt45, AT45_PAGE_WRITE_BUF1, 4, pucBuffer, dwSize, dwAddress, 0, 0 ) ;
    assert( !dwError ) ;

    /* Wait until the command is sent. */
    while ( AT45_IsBusy( pAt45 ) )
    {
        AT45D_Wait( pAt45 ) ;
    }

    /* Wait until the At45 becomes ready again.*/
    AT45D_WaitReady( pAt45 ) ;
}

/**
 * \brief Erases a page of data at the given address in the At45.
 *
 * \param pAt45  Pointer to an AT45 driver instance.
 * \param dwAddress  Address of page to erase.
 */
extern void AT45D_Erase( At45* pAt45, uint32_t dwAddress )
{
    uint32_t dwError ;

    assert( pAt45 != NULL ) ;

    /* Issue a page erase command. */
    dwError = AT45_SendCommand( pAt45, AT45_PAGE_ERASE, 4, 0, 0, dwAddress, 0, 0 ) ;
    assert( !dwError ) ;

    /* Wait for end of transfer. */
    while ( AT45_IsBusy(pAt45 ) )
    {
        AT45D_Wait( pAt45 ) ;
    }

    /* Poll until the At45 has completed the erase operation. */
    AT45D_WaitReady( pAt45 ) ;
}

/**
 * \brief Configure power-of-2 binary page size in the At45.
 *
 * \param pAt45  Pointer to an AT45 driver instance.
 */
extern void AT45D_BinaryPage( At45* pAt45 )
{
    uint8_t dwError ;
    uint8_t opcode[3]= {AT45_BINARY_PAGE};
    assert( pAt45 != NULL ) ;

    /* Issue a binary page command. */

    dwError = AT45_SendCommand( pAt45, AT45_BINARY_PAGE_FIRST_OPCODE, 1, opcode, 3, 0, 0, 0 ) ;

    assert( !dwError ) ;

    /* Wait for end of transfer.*/
    while ( AT45_IsBusy( pAt45 ) )
    {
        AT45D_Wait( pAt45 ) ;
    }

    /* Wait until the At45 becomes ready again.*/
    AT45D_WaitReady( pAt45 ) ;
}
