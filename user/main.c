#include "stm32f10x.h"
#include "stm32f10x_exti.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_usart.h"
#include "stm32f10x_rcc.h"
#include "misc.h"
#include "touch.h"
#include "lcd.h"

// gas, fire state
#define ON 1
#define OFF 0

// servo motor
// 75 -> 7.5% -> 0도
// 150 -> 15% -> 180
#define OPEN 75
#define CLOSE 115

/* function prototype */
void RCC_Configure(void);
void GPIO_Configure(void);
void NVIC_Configure(void);
void EXTI_Configure(void);
void TIM_Configure(void);

void USART1_Init(void);
void USART2_Init(void);
void sendDataUART1(uint16_t data);
void sendDataUART2(uint16_t data);
void sendMsgUART1(uint16_t data);
void sendMsgUART2(uint16_t data);

void Gas1(void);
void Gas2(void);
void Fire1(void);
void Fire2(void);
void ShutDown(void);

void Door1Open(void);
void Door1Close(void);
void Door2Open(void);
void Door2Close(void);

void Change3PWM(int percentx10);
void Change4PWM(int percentx10);

void Delay(void);

// ** Switch 핀 설정과  Handler 작성 필요 

// uint16_t adc_value = 0;
// uint16_t x_pos = 0;
// uint16_t y_pos = 0;

uint16_t human1 = 0;
uint16_t human2 = 0;
uint16_t shutdown = 0;

#define MUSIC_SOL 128
uint32_t Sound = 0;
uint32_t Music = 0;

int gas1_state = OFF;
int gas2_state = OFF;
int fire1_state = OFF;
int fire2_state = OFF;

char gas1_msg[] = "ROOM1: GAS SYSTEM\r\n";
char gas2_msg[] = "ROOM2: GAS SYSTEM\r\n";
char fire1_msg[] = "ROOM1: FIRE SYSTEM\r\n";
char fire2_msg[] = "ROOM2: FIRE SYSTEM\r\n";
char stop_msg[] = "SYSTEM ALL STOP\r\n";

//-------------------------------------------------------------------------//

void RCC_Configure(void)
{

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOE, ENABLE);

	/* Alternate Function IO clock enable */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

	/* ADC1 clock enable */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);

	/* TIM3 for Piezo */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
	
	/* TIM4 for Servo Motor */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);

    /* USART1 clock enable */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);

    /* USART2 clock enable */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
}


void GPIO_Configure(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	// 가스센서 A0, 불꽃센서 A5, A6
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_0;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	// 가스센서 C1
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
    
	// 보드 LED
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2 | GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOD, &GPIO_InitStructure);

	// 인체센서 A7, A8
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7 | GPIO_Pin_8;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    
	// 모터드라이버
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7 | GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	
    
	// user button1: GPIOD_11, this is pull-up switch
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(GPIOD, &GPIO_InitStructure);

	// user button2: GPIOD_12
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(GPIOD, &GPIO_InitStructure);

	// switch1: ROOM1
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	// switch2: ROOM2
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	// 피에조
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

	/* USART1 pin setting */
	// TX : PA9
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

	// RX : PA10
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
	
    /* USART2 pin setting */
    // TX : PA2
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

	// RX : PA3
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(GPIOA, &GPIO_InitStructure); 

    // 서보모터
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	// LED
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15 | GPIO_Pin_0;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
    
}

// 인터럽트 priority 설정 필요
// 나중에 할 예정
void NVIC_Configure(void)
{
	NVIC_InitTypeDef NVIC_InitStructure;

	// TODO: fill the arg you want
	// this is only subpriority 4bits
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);

    
	// 가스센서 인터럽트
	// A0
	NVIC_InitStructure.NVIC_IRQChannel = EXTI0_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1; // TODO
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0; // TODO
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	// 가스센서 인터럽트
	// C1
	NVIC_InitStructure.NVIC_IRQChannel = EXTI1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1; // TODO
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0; // TODO
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	// switch1
	// C2
	NVIC_InitStructure.NVIC_IRQChannel = EXTI2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2; // TODO
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0; // TODO
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	// switch2
	// C3
	NVIC_InitStructure.NVIC_IRQChannel = EXTI3_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2; // TODO
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0; // TODO
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	// 불꽃센서 인터럽트
	// A5 A6
	// 인체센서 인터럽트
	// A7 A8
	NVIC_InitStructure.NVIC_IRQChannel = EXTI9_5_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1; // TODO
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0; // TODO
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	// User Button pin1
    // User Button Pin2
	NVIC_InitStructure.NVIC_IRQChannel = EXTI15_10_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
    

	// USART1
	NVIC_EnableIRQ(USART1_IRQn);
    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0; // TODO
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0; // TODO
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

	// USART2
    NVIC_EnableIRQ(USART2_IRQn);
    NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0; // TODO
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0; // TODO
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

	// Piezo
    /* Enable the TIM3 global Interrupt */
	NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
}


void EXTI_Configure(void)
{
	EXTI_InitTypeDef EXTI_InitStructure;

	// A0: Room1 가스
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource0);
	EXTI_InitStructure.EXTI_Line = EXTI_Line0;
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStructure);

	// C1: Romm2 가스
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOC, GPIO_PinSource1);
	EXTI_InitStructure.EXTI_Line = EXTI_Line1;
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStructure);

	// A5: Room1 불꽃
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource5);
	EXTI_InitStructure.EXTI_Line = EXTI_Line5;
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStructure);

	// A6: Room2 불꽃
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource6);
	EXTI_InitStructure.EXTI_Line = EXTI_Line6;
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStructure);

	// A7: Room1 인체
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource7);
	EXTI_InitStructure.EXTI_Line = EXTI_Line7;
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising_Falling;
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStructure);

	// A8: Room2 인체
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource8);
	EXTI_InitStructure.EXTI_Line = EXTI_Line8;
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising_Falling;
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStructure);

	// User Button: Port D Pin_11
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOD, GPIO_PinSource11);
	EXTI_InitStructure.EXTI_Line = EXTI_Line11;
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStructure);

	// User Button: Port D Pin_12?
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOD, GPIO_PinSource12);
	EXTI_InitStructure.EXTI_Line = EXTI_Line12;
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStructure);
	
	// switch1
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOC, GPIO_PinSource2);
	EXTI_InitStructure.EXTI_Line = EXTI_Line2;
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStructure);

	// switch2
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOC, GPIO_PinSource3);
	EXTI_InitStructure.EXTI_Line = EXTI_Line3;
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStructure);
	
}

void TIM_Configure(void)
{
    TIM_TimeBaseInitTypeDef TIM_InitStructure;

	// 피에조
	// TIM3
	TIM_InitStructure.TIM_Period = 10;
	TIM_InitStructure.TIM_Prescaler = 72; 
	TIM_InitStructure.TIM_ClockDivision = 0;
	TIM_InitStructure.TIM_CounterMode = TIM_CounterMode_Up; 
	TIM_TimeBaseInit(TIM3, &TIM_InitStructure);

	TIM_Cmd(TIM3, ENABLE);

	TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);
	
	// servo motor
	// TIM4
	// set 1MHz Counter Clock
    TIM_InitStructure.TIM_Prescaler = (SystemCoreClock / 1000000) - 1;
    TIM_InitStructure.TIM_Period = 20000-1;
    TIM_InitStructure.TIM_ClockDivision = 0;
    TIM_InitStructure.TIM_CounterMode = TIM_CounterMode_Down;
    TIM_TimeBaseInit(TIM4, &TIM_InitStructure);

	TIM_OCInitTypeDef TIM_OCInitStructure;
    // 7.5% duty cycle PWM mode
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_Pulse = 1500; 

	// channel 3 - room1
    TIM_OC3Init(TIM4, &TIM_OCInitStructure);
    TIM_OC3PreloadConfig(TIM4, TIM_OCPreload_Disable);
    
	/*
	// channel 4 - room2
	TIM_OC4Init(TIM4, &TIM_OCInitStructure);
    TIM_OC4PreloadConfig(TIM4, TIM_OCPreload_Disable);
	*/

	TIM_Cmd(TIM4, ENABLE);
	TIM_ARRPreloadConfig(TIM4, ENABLE);
}

void USART1_Init(void)
{
	USART_InitTypeDef USART1_InitStructure;

	// Enable the USART1 peripheral
	USART_Cmd(USART1, ENABLE);
	
	// TODO: Initialize the USART using the structure 'USART_InitTypeDef' and the function 'USART_Init'
	USART1_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART1_InitStructure.USART_Parity = USART_Parity_No;
    USART1_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART1_InitStructure.USART_StopBits = USART_StopBits_1;
    USART1_InitStructure.USART_BaudRate = 9600;
    USART1_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_Init(USART1, &USART1_InitStructure);
	
	// TODO: Enable the USART1 RX interrupts using the function 'USART_ITConfig' and the argument value 'Receive Data register not empty interrupt'
	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
}

void USART2_Init(void)
{
	USART_InitTypeDef USART2_InitStructure;

	// Enable the USART1 peripheral
	USART_Cmd(USART2, ENABLE);
	
	// TODO: Initialize the USART using the structure 'USART_InitTypeDef' and the function 'USART_Init'
	USART2_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART2_InitStructure.USART_Parity = USART_Parity_No;
    USART2_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART2_InitStructure.USART_StopBits = USART_StopBits_1;
    USART2_InitStructure.USART_BaudRate = 9600;
    USART2_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_Init(USART2, &USART2_InitStructure);
	
	// TODO: Enable the USART1 RX interrupts using the function 'USART_ITConfig' and the argument value 'Receive Data register not empty interrupt'
	USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);
}

// A0: Room1 가스
void EXTI0_IRQHandler(void)
{
	if (EXTI_GetITStatus(EXTI_Line0) != RESET)
	{
		if (gas1_state == OFF && fire1_state == OFF) {
            Gas1();
        }

		EXTI_ClearITPendingBit(EXTI_Line0);
	}

}

// C1: Room2 가스
void EXTI1_IRQHandler(void)
{
	if (EXTI_GetITStatus(EXTI_Line1) != RESET)
	{

        if (gas2_state == OFF && fire2_state == OFF) {
            Gas2();
        }
		
		EXTI_ClearITPendingBit(EXTI_Line1);
	}
}

// C2: switch1
void EXTI2_IRQHandler(void)
{
	if (EXTI_GetITStatus(EXTI_Line2) != RESET)
	{
		char msg[] = "Room1 Request!\r\n";

        for(int i=0; i < sizeof(msg)/sizeof(char); i++) {
			sendMsgUART1(msg[i]);
            sendMsgUART2(msg[i]);
        }

		EXTI_ClearITPendingBit(EXTI_Line2);
	}
}

// C3: switch3
void EXTI3_IRQHandler(void)
{
	if (EXTI_GetITStatus(EXTI_Line3) != RESET)
	{
		char msg[] = "Room2 Request!\r\n";

        for(int i=0; i<sizeof(msg)/sizeof(char); i++) {
			sendMsgUART1(msg[i]);
            sendMsgUART2(msg[i]);
        }

		EXTI_ClearITPendingBit(EXTI_Line3);
	}
}

// A5, A6: 불꽃
void EXTI9_5_IRQHandler(void)
{
	if (EXTI_GetITStatus(EXTI_Line5) != RESET)
	{
		
		if (fire1_state == OFF) {
            Fire1();
        }
		
		EXTI_ClearITPendingBit(EXTI_Line5);
	}
	else if (EXTI_GetITStatus(EXTI_Line6) != RESET)
	{
		
		if (fire2_state == OFF) {
            Fire2();
        }

		EXTI_ClearITPendingBit(EXTI_Line6);
	}
	else if (EXTI_GetITStatus(EXTI_Line7) != RESET)
	{
		if (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_7) == RESET)
    	{
			human1 = 0;
    	}
    	else if  (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_7) == SET)
		{
			human1 = 1;
    	}
		EXTI_ClearITPendingBit(EXTI_Line7);
	}
	else if (EXTI_GetITStatus(EXTI_Line8) != RESET)
	{
		if (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_8) == RESET)
    	{
			human2 = 0;
    	}
    	else if  (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_8) == SET)
		{
			human2 = 1;
    	}
		EXTI_ClearITPendingBit(EXTI_Line8);
	}
}


void EXTI15_10_IRQHandler(void)
{
    
	// when the user button1(GPIOD_11) is pressed
	if (EXTI_GetITStatus(EXTI_Line11) != RESET)
	{
        // User Button 1: System Shut Down
		if (GPIO_ReadInputDataBit(GPIOD, GPIO_Pin_11) == Bit_RESET)
		{
			ShutDown();
		}
		
		EXTI_ClearITPendingBit(EXTI_Line11);
	}

    
    if (EXTI_GetITStatus(EXTI_Line12) != RESET)
    {
        // User Button 2: System all Start
        if (GPIO_ReadInputDataBit(GPIOD, GPIO_Pin_12) == Bit_RESET)
		{
			GPIO_SetBits(GPIOD, GPIO_Pin_7);
			if (fire1_state == OFF) {
                Fire1();
            }
            if (fire2_state == OFF) {
                Fire2();
            }
		}
        
		EXTI_ClearITPendingBit(EXTI_Line12);
    }
}

// 스피커 
void TIM3_IRQHandler(void)
{
	
	// ** TODO 
	if(TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET)
	{
		TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
		Sound++;
		// Music = 128
		if(Sound >= Music) {
			
			if (gas1_state == ON || fire1_state == ON)
            	GPIOB->ODR ^= GPIO_Pin_0;
			if (gas2_state == ON || fire2_state == ON)
                GPIOB->ODR ^= GPIO_Pin_1;
			
			Sound = 0;
		}
	}
}


void USART1_IRQHandler() {
	uint16_t word;
    if(USART_GetITStatus(USART1,USART_IT_RXNE)!=RESET){
    	// the most recent received data by the USART1 peripheral
        word = USART_ReceiveData(USART1);

        // TODO implement
        sendDataUART2(word);

        // clear 'Read data register not empty' flag
    	USART_ClearITPendingBit(USART1,USART_IT_RXNE);
    }
}

void USART2_IRQHandler() {
	uint16_t word;
    if(USART_GetITStatus(USART2,USART_IT_RXNE)!=RESET){
    	// the most recent received data by the USART1 peripheral
        word = USART_ReceiveData(USART2);

        // TODO implement
        sendDataUART1(word);

        // clear 'Read data register not empty' flag
    	USART_ClearITPendingBit(USART2,USART_IT_RXNE);
    }
}


// input value
// 75 -> 7.5% -> 0도
// 150 -> 15% -> 180
// room1
void Change3PWM(int percentx10)
{
    TIM_OCInitTypeDef TIM_OCInitStructure;
    int pwm_pulse = percentx10 * 20000 / 1000;
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_Pulse = pwm_pulse; 
    TIM_OC3Init(TIM4, &TIM_OCInitStructure);
}

// room2
void Change4PWM(int percentx10)
{
    TIM_OCInitTypeDef TIM_OCInitStructure;
    int pwm_pulse = percentx10 * 20000 / 1000;
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_Pulse = pwm_pulse; 
    TIM_OC4Init(TIM4, &TIM_OCInitStructure);
}

void sendDataUART1(uint16_t data) {
	/* Wait till TC is set */
	// while ((USART1->SR & USART_SR_TC) == 0);
	USART_SendData(USART1, data);
}

void sendMsgUART1(uint16_t data) {
	/* Wait till TC is set */
	while ((USART1->SR & USART_SR_TC) == 0);
	USART_SendData(USART1, data);
}

void sendDataUART2(uint16_t data) {
	/* Wait till TC is set */
	// while ((USART1->SR & USART_SR_TC) == 0);
	USART_SendData(USART2, data);
}

void sendMsgUART2(uint16_t data) {
	/* Wait till TC is set */
	while ((USART2->SR & USART_SR_TC) == 0);
	USART_SendData(USART2, data);
}

void Door1Open()
{
	Change3PWM(OPEN);
}

void Door1Close()
{
	if (gas1_state == ON || fire1_state == ON)
	{
		Change3PWM(CLOSE);
	}
}

void Door2Open()
{
	Change4PWM(OPEN);
}

void Door2Close()
{
	if (gas2_state == ON || fire2_state == ON)
	{
		Change4PWM(CLOSE);
	}
}

void Gas1()
{
	gas1_state = ON;
    LCD_ShowString(10, 30, "GAS1 ON ", BLACK, WHITE);
	

	
	// 스피커
	TIM_Cmd(TIM3, ENABLE);

	// RLED
	GPIO_SetBits(GPIOC, GPIO_Pin_13);
	// GLED
	GPIO_ResetBits(GPIOC, GPIO_Pin_14);

		// 팬모터 
	GPIO_SetBits(GPIOB, GPIO_Pin_4);
	GPIO_ResetBits(GPIOB, GPIO_Pin_5);	

	// ** TODO : 인체 
	if (human1 == ON)
	{
		// 문열림
		Door1Open(); 
	}
	else
	{
		// 문닫힘  
		Door1Close();
	}


	// ** TODO : USART 메세지 전송 코드
    for(int i=0; i<sizeof(gas1_msg)/sizeof(char); i++) {
        sendMsgUART2(gas1_msg[i]);
    }

}

void Gas2()
{
	gas2_state = ON;
    LCD_ShowString(10, 50, "GAS2 ON ", BLACK, WHITE);

	// /*
	// 팬모터  
	GPIO_SetBits(GPIOB, GPIO_Pin_12);
	GPIO_ResetBits(GPIOB, GPIO_Pin_13);
	
	// 스피커
	TIM_Cmd(TIM3, ENABLE);

	// RLED
	GPIO_SetBits(GPIOC, GPIO_Pin_15);
	// GLED
	GPIO_ResetBits(GPIOC, GPIO_Pin_0);

	// ** TODO : 인체 
	if (human2 == ON)
	{
		// 문열림
		Door2Open(); 
	}
	else
	{
		// 문닫힘  
		Door2Close();
	}

	// ** TODO : USART 메세지 전송 코드
	for(int i=0; i<sizeof(gas2_msg)/sizeof(char); i++) {
        sendMsgUART2(gas2_msg[i]);
    }
}


void Fire1()
{
	fire1_state = ON;
    LCD_ShowString(10, 70, "FIRE1 ON ", BLACK, WHITE);

	
	// 팬모터  
	GPIO_SetBits(GPIOB, GPIO_Pin_4);
	GPIO_ResetBits(GPIOB, GPIO_Pin_5);
		
	// 펌프모터  
	GPIO_SetBits(GPIOB, GPIO_Pin_6);
	GPIO_ResetBits(GPIOB, GPIO_Pin_7);

	// 스피커
	TIM_Cmd(TIM3, ENABLE);

	// RLED
	GPIO_SetBits(GPIOC, GPIO_Pin_13);
	// GLED
	GPIO_ResetBits(GPIOC, GPIO_Pin_14);

	// ** TODO : 인체 
	if (human1 == ON)
	{
		// 문열림
		Door1Open(); 
	}
	else
	{
		// 문닫힘  
		Door1Close();
	}
	
	// ** TODO : USART 메세지 전송 코드
	for(int i=0; i<sizeof(fire1_msg)/sizeof(char); i++) {
        sendMsgUART2(fire1_msg[i]);
    }
	
}


void Fire2()
{
	fire2_state = ON;
    LCD_ShowString(10, 90, "FIRE2 ON ", BLACK, WHITE);

	
	// /*
	// 팬모터  
	GPIO_SetBits(GPIOB, GPIO_Pin_12);
	GPIO_ResetBits(GPIOB, GPIO_Pin_13);

	// 펌프모터  
	GPIO_SetBits(GPIOB, GPIO_Pin_14);
	GPIO_ResetBits(GPIOB, GPIO_Pin_15);

	// 스피커 B3
	TIM_Cmd(TIM3, ENABLE);

	// RLED
	GPIO_SetBits(GPIOC, GPIO_Pin_15);
	// GLED
	GPIO_ResetBits(GPIOC, GPIO_Pin_0);

	// ** TODO : 인체 
	if (human2 == ON)
	{
		// 문열림
		Door2Open(); 
	}
	else
	{
		// 문닫힘  
		Door2Close();
	}

	// ** TODO : USART 메세지 전송 코드
	for(int i=0; i<sizeof(fire2_msg)/sizeof(char); i++) {
        sendMsgUART2(fire2_msg[i]);
    }
}


void ShutDown()
{
    fire1_state = OFF;
    fire2_state = OFF;
    gas1_state = OFF;
    gas2_state = OFF;

    LCD_ShowString(10, 30, "GAS1 OFF", BLACK, WHITE);
    LCD_ShowString(10, 50, "GAS2 OFF", BLACK, WHITE);
    LCD_ShowString(10, 70, "FIRE1 OFF", BLACK, WHITE);
    LCD_ShowString(10, 90, "FIRE2 OFF", BLACK, WHITE);

	// /*
	// 팬모터  
	GPIO_ResetBits(GPIOB, GPIO_Pin_4);
	GPIO_ResetBits(GPIOB, GPIO_Pin_5);
	GPIO_ResetBits(GPIOB, GPIO_Pin_6);
	GPIO_ResetBits(GPIOB, GPIO_Pin_7);

	// 펌프모터
	GPIO_ResetBits(GPIOB, GPIO_Pin_12);
	GPIO_ResetBits(GPIOB, GPIO_Pin_13);
	GPIO_ResetBits(GPIOB, GPIO_Pin_14);
	GPIO_ResetBits(GPIOB, GPIO_Pin_15); 

	// 스피커
	TIM_Cmd(TIM3, DISABLE);

	// RLED
	GPIO_ResetBits(GPIOC, GPIO_Pin_13);
	// GLED
	GPIO_SetBits(GPIOC, GPIO_Pin_14);
	// RLED
	GPIO_ResetBits(GPIOC, GPIO_Pin_15);
	// GLED
	GPIO_SetBits(GPIOC, GPIO_Pin_0);

	Door1Open();
	Door2Open();

	/*
	// 스피커  
	//GPIO_ResetBits(GPIOB , GPIO_Pin_ 0);
	//GPIO_ResetBits(GPIOB , GPIO_Pin_ 3);
	*/

	// ** TODO : USART 메세지 전송 코드
	for(int i=0; i<sizeof(stop_msg)/sizeof(char); i++) {
        sendMsgUART2(stop_msg[i]);
    }
}

int main(void)
{
	SystemInit();
	RCC_Configure();
	GPIO_Configure();
	NVIC_Configure();
	EXTI_Configure();
	TIM_Configure();
    USART1_Init();
    USART2_Init(); 

	// 피에조
	TIM_Cmd(TIM3, DISABLE);

	LCD_Init();
	Touch_Configuration();
	Touch_Adjust();
	LCD_Clear(WHITE);

    LCD_ShowString(10, 30, "GAS1 OFF", BLACK, WHITE);
    LCD_ShowString(10, 50, "GAS2 OFF", BLACK, WHITE);
    LCD_ShowString(10, 70, "FIRE1 OFF", BLACK, WHITE);
    LCD_ShowString(10, 90, "FIRE2 OFF", BLACK, WHITE);
    
    // R1모터  
	GPIO_ResetBits(GPIOB, GPIO_Pin_4);
	GPIO_ResetBits(GPIOB, GPIO_Pin_5);
	GPIO_ResetBits(GPIOB, GPIO_Pin_6);
	GPIO_ResetBits(GPIOB, GPIO_Pin_7);

	// R2모터
	GPIO_ResetBits(GPIOB, GPIO_Pin_12);
	GPIO_ResetBits(GPIOB, GPIO_Pin_13);
	GPIO_ResetBits(GPIOB, GPIO_Pin_14);
	GPIO_ResetBits(GPIOB, GPIO_Pin_15);

	// RLED
	GPIO_ResetBits(GPIOC, GPIO_Pin_13);
	// GLED
	GPIO_SetBits(GPIOC, GPIO_Pin_14);
	// RLED
	GPIO_ResetBits(GPIOC, GPIO_Pin_15);
	// GLED
	GPIO_SetBits(GPIOC, GPIO_Pin_0);

	// 문 열림
	Door1Open();
	Door2Open();

	while (1)
   {
	LCD_ShowNum(10, 130, human2, 4, BLACK, WHITE);
	Music = MUSIC_SOL;
   }

	return 0;
}