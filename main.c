/******************************************************************************
* File Name:   main.c
*
* Description: This is the source code for the MDMA Scatter Transfer
*              example for ModusToolbox.
*
* Related Document: See README.md
*
*
*******************************************************************************
* Copyright 2024-2025, Cypress Semiconductor Corporation (an Infineon company) or
* an affiliate of Cypress Semiconductor Corporation.  All rights reserved.
*
* This software, including source code, documentation and related
* materials ("Software") is owned by Cypress Semiconductor Corporation
* or one of its affiliates ("Cypress") and is protected by and subject to
* worldwide patent protection (United States and foreign),
* United States copyright laws and international treaty provisions.
* Therefore, you may use this Software only as provided in the license
* agreement accompanying the software package from which you
* obtained this Software ("EULA").
* If no EULA applies, Cypress hereby grants you a personal, non-exclusive,
* non-transferable license to copy, modify, and compile the Software
* source code solely for use in connection with Cypress's
* integrated circuit products.  Any reproduction, modification, translation,
* compilation, or representation of this Software except as specified
* above is prohibited without the express written permission of Cypress.
*
* Disclaimer: THIS SOFTWARE IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, NONINFRINGEMENT, IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. Cypress
* reserves the right to make changes to the Software without notice. Cypress
* does not assume any liability arising out of the application or use of the
* Software or any product or circuit described in the Software. Cypress does
* not authorize its products for use in any products where a malfunction or
* failure of the Cypress product may reasonably be expected to result in
* significant property damage, injury or death ("High Risk Product"). By
* including Cypress's product in a High Risk Product, the manufacturer
* of such system or application assumes all risk of such use and in doing
* so agrees to indemnify Cypress against all liability.
*******************************************************************************/

/******************************************************************************
* Include header files
*******************************************************************************/
#include "cy_pdl.h"
#include "cybsp.h"
#include "cy_retarget_io.h"
#include "mtb_hal.h"

/* For the Retarget -IO (Debug UART) usage */
static cy_stc_scb_uart_context_t    UART_context;           /** UART context */
static mtb_hal_uart_t               UART_hal_obj;           /** Debug UART HAL object  */

/*******************************************************************************
 * Function Name: printArrayToUART
 * Summary:
 *  Helper function to print all elements of an uint32_t array into UART. 
 *  Since it uses the printf function, it has to be assured by the caller 
 *  that this function works as intended (retarget-io initialized)
 * Parameters:
 *  ARRAY - Pointer to array to print
 *  NUM - Number of elements of ARRAY 
 *  ARRAY_NAME - Name of the array. Will be printed in the beginning
 * Return:
 *  none
 *******************************************************************************
 */
static void printArrayToUART(const uint32_t* array, const uint32_t num, const char* arrayName)
{
    if (array == NULL || num <= 0 || arrayName == NULL)
    {
        return;
    }

    printf("Data of %s: {%#x", arrayName, (unsigned int) array[0]);
    for (uint32_t i = 1; i < num; ++i)
    {
        printf(", %#x", (unsigned int) array[i]);
    }
    printf("}\r\n");
}

/*******************************************************************************
 * Function Name: main
 * Summary:
 *  This is the main function for CPU. It initializes the DMA transfer, waits 
 *  until it is finished and verifies the results.
 * Parameters:
 *  none
 * Return:
 *  none
 *******************************************************************************
 */
int main(void)
{
    cy_rslt_t result;

    /* Initialize the device and board peripherals */
    if (cybsp_init() != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(0);
    }

    /* Disable instruction and data cache. */
    SCB_DisableDCache();
    SCB_DisableICache();

    /* Enable global interrupts */
    __enable_irq();

    /* Debug UART init */
    result = (cy_rslt_t)Cy_SCB_UART_Init(UART_HW, &UART_config, &UART_context);

    /* UART init failed. Stop program execution */
    if (result != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(0);
    }

    Cy_SCB_UART_Enable(UART_HW);

    /* Setup the HAL UART */
    result = mtb_hal_uart_setup(&UART_hal_obj, &UART_hal_config, &UART_context, NULL);

    /* HAL UART init failed. Stop program execution */
    if (result != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(0);
    }

    result = cy_retarget_io_init(&UART_hal_obj);

    /* retarget-io init failed. Stop program execution */
    if (result != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(0);
    }

    /* Variables for DMAC */
    /* Array to which the source data is getting copied to */
    uint32_t dst[] = { 0, 0, 0, 0, 0, 0, 0, 0 };
    /* Array to copy data from, given in the scatter format */
    uint32_t src[] = {
            (uint32_t) &dst[2], 0, (uint32_t) &dst[7], 1,
            (uint32_t) &dst[0], 2, (uint32_t) &dst[1], 3,
            (uint32_t) &dst[5], 4, (uint32_t) &dst[4], 5,
            (uint32_t) &dst[3], 6, (uint32_t) &dst[6], 7
    };
    /* Reference array of how the data should look like in dst. Will be verified */
    const uint32_t REFERENCE[] = { 2, 3, 0, 6, 5, 4, 7, 1 };

    /* \x1b[2J\x1b[;H - ANSI ESC sequence for clear screen */
    printf("\x1b[2J\x1b[;H");
    printf("--- M-DMA Scatter Transfer Example ---\r\n\r\n");
    printf("Address of src array: 0x%08x\r\n", (unsigned int) src);
    printArrayToUART(src, sizeof(src) / sizeof(src[0]), "src");
    printf("Address of dst array: 0x%08x\r\n", (unsigned int) dst);
    printArrayToUART(dst, sizeof(dst) / sizeof(dst[0]), "dst");

    /* Initialize descriptor. Halts application if it fails */
    if (Cy_DMAC_Descriptor_Init(&myDMA_Descriptor_0, &myDMA_Descriptor_0_config) != CY_DMAC_SUCCESS)
    {
        CY_ASSERT(0);
    }

    /* Set the source address of the descriptor. Destination is not necessary for scatter transfer */
    Cy_DMAC_Descriptor_SetSrcAddress(&myDMA_Descriptor_0, src);

    /* Initialize channel. Halts application if it fails */
    if (Cy_DMAC_Channel_Init(myDMA_HW, myDMA_CHANNEL, &myDMA_channelConfig) != CY_DMAC_SUCCESS)
    {
        CY_ASSERT(0);
    }

    /* Set descriptor to channel and enable it */
    Cy_DMAC_Channel_SetDescriptor(myDMA_HW, myDMA_CHANNEL, &myDMA_Descriptor_0);
    Cy_DMAC_Channel_Enable(myDMA_HW, myDMA_CHANNEL);

    /* Enable the DMAC */
    Cy_DMAC_Enable(myDMA_HW);

    /* Trigger DMA transfer via software */
    printf("Start DMA transfer!\r\n");
    Cy_TrigMux_SwTrigger(TRIG_OUT_MUX_3_MDMA_TR_IN0, CY_TRIGGER_TWO_CYCLES);

    /* We poll until DMA is finished. Alternatively, we could have registered an interrupt
     * handler and wait until the interrupt occurs.
     */
    while ((Cy_DMAC_Channel_GetInterruptStatus(DMAC, myDMA_CHANNEL) & CY_DMAC_INTR_COMPLETION)
            != CY_DMAC_INTR_COMPLETION)
    {
    }
    
    /* DMA transfer is finished now */
    printf("DMA transfer finished!\r\n");
    printArrayToUART(dst, sizeof(dst) / sizeof(dst[0]), "dst");

    /* Verify data against reference */
    if (memcmp((void*) dst, (void*) REFERENCE, sizeof(dst)) == 0)
    {
        /* Verification successful. Turn LED on */
        printf("Data was transferred correctly!\r\n");
        Cy_GPIO_Write(CYBSP_USER_LED_PORT, CYBSP_USER_LED_PIN, CYBSP_LED_STATE_ON);
    }
    else
    {
        /* Verification failed. Something went wrong */
        printf("Data in dst array is not as expected. Something went wrong.\r\n");
    }

    for (;;)
    {
    }

}

/* [] END OF FILE */
