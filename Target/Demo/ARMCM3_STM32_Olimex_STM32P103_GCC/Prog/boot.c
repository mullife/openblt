/************************************************************************************//**
* \file         Demo\ARMCM3_STM32_Olimex_STM32P103_GCC\Prog\boot.c
* \brief        Demo program bootloader interface source file.
* \ingroup      Prog_ARMCM3_STM32_Olimex_STM32P103_GCC
* \internal
*----------------------------------------------------------------------------------------
*                          C O P Y R I G H T
*----------------------------------------------------------------------------------------
*   Copyright (c) 2012  by Feaser    http://www.feaser.com    All rights reserved
*
*----------------------------------------------------------------------------------------
*                            L I C E N S E
*----------------------------------------------------------------------------------------
* This file is part of OpenBLT. OpenBLT is free software: you can redistribute it and/or
* modify it under the terms of the GNU General Public License as published by the Free
* Software Foundation, either version 3 of the License, or (at your option) any later
* version.
*
* OpenBLT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
* without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
* PURPOSE. See the GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License along with OpenBLT.
* If not, see <http://www.gnu.org/licenses/>.
*
* A special exception to the GPL is included to allow you to distribute a combined work 
* that includes OpenBLT without being obliged to provide the source code for any 
* proprietary components. The exception text is included at the bottom of the license
* file <license.html>.
* 
* \endinternal
****************************************************************************************/

/****************************************************************************************
* Include files
****************************************************************************************/
#include "header.h"                                    /* generic header               */


/************************************************************************************//**
** \brief     Bootloader activation function.
** \return    none.
**
****************************************************************************************/
static void BootActivate(void)
{
  void (*pEntryFromProgFnc)(void);

  /* stop the timer interrupt */
  TimerDeinit();
  /* set pointer to the address of function EntryFromProg in the bootloader. note that
   * 1 is added to this address to enable a switch from Thumb2 to Thumb mode
   */
  pEntryFromProgFnc = (void*)0x08000150 + 1;
  /* call EntryFromProg to activate the bootloader. */
  pEntryFromProgFnc();
} /*** end of BootActivate ***/


#if (BOOT_COM_UART_ENABLE > 0)
/****************************************************************************************
*     U N I V E R S A L   A S Y N C H R O N O U S   R X   T X   I N T E R F A C E
****************************************************************************************/

/****************************************************************************************
* Function prototypes
****************************************************************************************/
static unsigned char UartReceiveByte(unsigned char *data);


/************************************************************************************//**
** \brief     Initializes the UART communication interface.
** \return    none.
**
****************************************************************************************/
void BootComInit(void)
{
  GPIO_InitTypeDef  GPIO_InitStruct;
  USART_InitTypeDef USART_InitStruct;  

  /* enable UART peripheral clock */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
  /* enable GPIO peripheral clock for transmitter and receiver pins */
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO, ENABLE);
  /* configure USART Tx as alternate function push-pull */
  GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF_PP;
  GPIO_InitStruct.GPIO_Pin = GPIO_Pin_2;
  GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOA, &GPIO_InitStruct);
  /* Configure USART Rx as alternate function input floating */
  GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN_FLOATING;
  GPIO_InitStruct.GPIO_Pin = GPIO_Pin_3;
  GPIO_Init(GPIOA, &GPIO_InitStruct);
  /* configure UART communcation parameters */  
  USART_InitStruct.USART_BaudRate = BOOT_COM_UART_BAUDRATE;
  USART_InitStruct.USART_WordLength = USART_WordLength_8b;
  USART_InitStruct.USART_StopBits = USART_StopBits_1;
  USART_InitStruct.USART_Parity = USART_Parity_No;
  USART_InitStruct.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  USART_InitStruct.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
  USART_Init(USART2, &USART_InitStruct);
  /* enable UART */
  USART_Cmd(USART2, ENABLE);
} /*** end of BootComInit ***/


/************************************************************************************//**
** \brief     Receives the CONNECT request from the host, which indicates that the
**            bootloader should be activated and, if so, activates it.
** \return    none.
**
****************************************************************************************/
void BootComCheckActivationRequest(void)
{
  static unsigned char xcpCtoReqPacket[BOOT_COM_UART_RX_MAX_DATA+1];
  static unsigned char xcpCtoRxLength;
  static unsigned char xcpCtoRxInProgress = 0;

  /* start of cto packet received? */
  if (xcpCtoRxInProgress == 0)
  {
    /* store the message length when received */
    if (UartReceiveByte(&xcpCtoReqPacket[0]) == 1)
    {
      /* indicate that a cto packet is being received */
      xcpCtoRxInProgress = 1;

      /* reset packet data count */
      xcpCtoRxLength = 0;
    }
  }
  else
  {
    /* store the next packet byte */
    if (UartReceiveByte(&xcpCtoReqPacket[xcpCtoRxLength+1]) == 1)
    {
      /* increment the packet data count */
      xcpCtoRxLength++;

      /* check to see if the entire packet was received */
      if (xcpCtoRxLength == xcpCtoReqPacket[0])
      {
        /* done with cto packet reception */
        xcpCtoRxInProgress = 0;

        /* check if this was an XCP CONNECT command */
        if ((xcpCtoReqPacket[1] == 0xff) && (xcpCtoReqPacket[2] == 0x00))
        {
          /* connection request received so start the bootloader */
          BootActivate();
        }
      }
    }
  }
} /*** end of BootComCheckActivationRequest ***/


/************************************************************************************//**
** \brief     Receives a communication interface byte if one is present.
** \param     data Pointer to byte where the data is to be stored.
** \return    1 if a byte was received, 0 otherwise.
**
****************************************************************************************/
static unsigned char UartReceiveByte(unsigned char *data)
{
  /* check flag to see if a byte was received */
  if (USART_GetFlagStatus(USART2, USART_FLAG_RXNE) == SET)
  {
    /* retrieve and store the newly received byte */
    *data = (unsigned char)USART_ReceiveData(USART2);
    /* all done */
    return 1;
  }
  /* still here to no new byte received */
  return 0;
} /*** end of UartReceiveByte ***/
#endif /* BOOT_COM_UART_ENABLE > 0 */


#if (BOOT_COM_CAN_ENABLE > 0)
/****************************************************************************************
*        C O N T R O L L E R   A R E A   N E T W O R K   I N T E R F A C E
****************************************************************************************/

/****************************************************************************************
* Type definitions
****************************************************************************************/
/** \brief Structure type for grouping CAN bus timing related information. */
typedef struct t_can_bus_timing
{
  unsigned char tseg1;                                /**< CAN time segment 1          */
  unsigned char tseg2;                                /**< CAN time segment 2          */
} tCanBusTiming;


/****************************************************************************************
* Local constant declarations
****************************************************************************************/
/** \brief CAN bittiming table for dynamically calculating the bittiming settings.
 *  \details According to the CAN protocol 1 bit-time can be made up of between 8..25 
 *           time quanta (TQ). The total TQ in a bit is SYNC + TSEG1 + TSEG2 with SYNC 
 *           always being 1. The sample point is (SYNC + TSEG1) / (SYNC + TSEG1 + SEG2) * 
 *           100%. This array contains possible and valid time quanta configurations with
 *           a sample point between 68..78%.
 */
static const tCanBusTiming canTiming[] =
{                       /*  TQ | TSEG1 | TSEG2 | SP  */
                        /* ------------------------- */
    {  5, 2 },          /*   8 |   5   |   2   | 75% */
    {  6, 2 },          /*   9 |   6   |   2   | 78% */
    {  6, 3 },          /*  10 |   6   |   3   | 70% */
    {  7, 3 },          /*  11 |   7   |   3   | 73% */
    {  8, 3 },          /*  12 |   8   |   3   | 75% */
    {  9, 3 },          /*  13 |   9   |   3   | 77% */
    {  9, 4 },          /*  14 |   9   |   4   | 71% */
    { 10, 4 },          /*  15 |  10   |   4   | 73% */
    { 11, 4 },          /*  16 |  11   |   4   | 75% */
    { 12, 4 },          /*  17 |  12   |   4   | 76% */
    { 12, 5 },          /*  18 |  12   |   5   | 72% */
    { 13, 5 },          /*  19 |  13   |   5   | 74% */
    { 14, 5 },          /*  20 |  14   |   5   | 75% */
    { 15, 5 },          /*  21 |  15   |   5   | 76% */
    { 15, 6 },          /*  22 |  15   |   6   | 73% */
    { 16, 6 },          /*  23 |  16   |   6   | 74% */
    { 16, 7 },          /*  24 |  16   |   7   | 71% */
    { 16, 8 }           /*  25 |  16   |   8   | 68% */
};


/************************************************************************************//**
** \brief     Search algorithm to match the desired baudrate to a possible bus 
**            timing configuration.
** \param     baud The desired baudrate in kbps. Valid values are 10..1000.
** \param     prescaler Pointer to where the value for the prescaler will be stored.
** \param     tseg1 Pointer to where the value for TSEG2 will be stored.
** \param     tseg2 Pointer to where the value for TSEG2 will be stored.
** \return    1 if the CAN bustiming register values were found, 0 otherwise.
**
****************************************************************************************/
static unsigned char CanGetSpeedConfig(unsigned short baud, unsigned short *prescaler, 
                                       unsigned char *tseg1, unsigned char *tseg2)
{
  unsigned char cnt;

  /* loop through all possible time quanta configurations to find a match */
  for (cnt=0; cnt < sizeof(canTiming)/sizeof(canTiming[0]); cnt++)
  {
    if (((BOOT_CPU_SYSTEM_SPEED_KHZ/2) % (baud*(canTiming[cnt].tseg1+canTiming[cnt].tseg2+1))) == 0)
    {
      /* compute the prescaler that goes with this TQ configuration */
      *prescaler = (BOOT_CPU_SYSTEM_SPEED_KHZ/2)/(baud*(canTiming[cnt].tseg1+canTiming[cnt].tseg2+1));

      /* make sure the prescaler is valid */
      if ( (*prescaler > 0) && (*prescaler <= 1024) )
      {
        /* store the bustiming configuration */
        *tseg1 = canTiming[cnt].tseg1;
        *tseg2 = canTiming[cnt].tseg2;
        /* found a good bus timing configuration */
        return 1;
      }
    }
  }
  /* could not find a good bus timing configuration */
  return 0;
} /*** end of CanGetSpeedConfig ***/


/************************************************************************************//**
** \brief     Initializes the CAN communication interface.
** \return    none.
**
****************************************************************************************/
void BootComInit(void)
{
  GPIO_InitTypeDef  GPIO_InitStructure;
  CAN_InitTypeDef        CAN_InitStructure;
  CAN_FilterInitTypeDef  CAN_FilterInitStructure;
  unsigned short         prescaler;
  unsigned char          tseg1, tseg2;
  
  /* GPIO clock enable */
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
  /* Configure CAN pin: RX */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
  GPIO_Init(GPIOB, &GPIO_InitStructure);
  /* Configure CAN pin: TX */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOB, &GPIO_InitStructure);
  /* Remap CAN1 pins to PortB */
  GPIO_PinRemapConfig(GPIO_Remap1_CAN1 , ENABLE);
  /* CAN1 Periph clock enable */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_CAN1, ENABLE);
  /* CAN register init */
  CAN_DeInit(CAN1);
  CAN_StructInit(&CAN_InitStructure);
  /* obtain the bittiming configuration for this baudrate */
  CanGetSpeedConfig(BOOT_COM_CAN_BAUDRATE/1000, &prescaler, &tseg1, &tseg2);
  /* CAN controller init */
  CAN_InitStructure.CAN_TTCM = DISABLE;
  CAN_InitStructure.CAN_ABOM = DISABLE;
  CAN_InitStructure.CAN_AWUM = DISABLE;
  CAN_InitStructure.CAN_NART = DISABLE;
  CAN_InitStructure.CAN_RFLM = DISABLE;
  CAN_InitStructure.CAN_TXFP = DISABLE;
  CAN_InitStructure.CAN_Mode = CAN_Mode_Normal;
  /* CAN Baudrate init */
  CAN_InitStructure.CAN_SJW = CAN_SJW_1tq;
  CAN_InitStructure.CAN_BS1 = tseg1 - 1;
  CAN_InitStructure.CAN_BS2 = tseg2 - 1;
  CAN_InitStructure.CAN_Prescaler = prescaler;
  CAN_Init(CAN1, &CAN_InitStructure);
  /* CAN filter init - receive all messages */
  CAN_FilterInitStructure.CAN_FilterNumber = 0;
  CAN_FilterInitStructure.CAN_FilterMode = CAN_FilterMode_IdMask;
  CAN_FilterInitStructure.CAN_FilterScale = CAN_FilterScale_32bit;
  CAN_FilterInitStructure.CAN_FilterIdHigh = 0x0000;
  CAN_FilterInitStructure.CAN_FilterIdLow = 0x0000;
  CAN_FilterInitStructure.CAN_FilterMaskIdHigh = 0x0000;
  CAN_FilterInitStructure.CAN_FilterMaskIdLow = 0x0000;
  CAN_FilterInitStructure.CAN_FilterFIFOAssignment = 0;
  CAN_FilterInitStructure.CAN_FilterActivation = ENABLE;
  CAN_FilterInit(&CAN_FilterInitStructure);
} /*** end of BootComInit ***/


/************************************************************************************//**
** \brief     Receives the CONNECT request from the host, which indicates that the
**            bootloader should be activated and, if so, activates it.
** \return    none.
**
****************************************************************************************/
void BootComCheckActivationRequest(void)
{
  CanRxMsg RxMessage;

  /* check if a new message was received */
  if (CAN_MessagePending(CAN1, CAN_FIFO0) > 0)
  {
    /* receive the message */
    CAN_Receive(CAN1, CAN_FIFO0, &RxMessage);
    if (RxMessage.StdId == BOOT_COM_CAN_RX_MSG_ID)
    {
      /* check if this was an XCP CONNECT command */
      if ((RxMessage.Data[0] == 0xff) && (RxMessage.Data[1] == 0x00))
      {
        /* connection request received so start the bootloader */
        BootActivate();
       }
    }
  }
} /*** end of BootComCheckActivationRequest ***/
#endif /* BOOT_COM_CAN_ENABLE > 0 */


/*********************************** end of boot.c *************************************/
