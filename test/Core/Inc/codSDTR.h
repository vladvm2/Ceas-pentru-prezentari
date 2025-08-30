#ifndef CODSDTR_H
#define CODSDTR_H

#include "main.h"
#include "cmsis_os.h"
#include <stdio.h>        // <-- Add this

osSemaphoreId semHandle;


volatile uint8_t startFlag = 0;
volatile uint32_t startTime = 0;
volatile uint32_t elapsedTime = 0;
volatile uint32_t lastRunTime = 0;
volatile uint32_t totalTime = 0;
volatile uint32_t lastTimeDisplayed = 0;
volatile uint8_t sendStatus = 0;

// Variabilă globală de RX (definită în codSDTR.c)
extern uint8_t rxData;

// Protocoale funcții task-uri
void StartDefaultTask(void const * argument);
void StartTask01(void const * argument);
void StartTask02(void const * argument);
// variabila globala pentru comunicare USART -> bluetooth
uint8_t rxData;

// Extern din main.c
extern UART_HandleTypeDef huart2;

// DIGIT ports and pins
GPIO_TypeDef* digitPorts[4] = {Dig1_GPIO_Port, Dig2_GPIO_Port, Dig3_GPIO_Port, Dig4_GPIO_Port};
uint16_t digitPins[4]       = {Dig1_Pin, Dig2_Pin, Dig3_Pin, Dig4_Pin};

// SEGMENT ports and pins (A,B,C,D,E,F,G,DP)
GPIO_TypeDef* segmentPorts[8] = {A_GPIO_Port, B_GPIO_Port, C_GPIO_Port, D_GPIO_Port,
		E_GPIO_Port, F_GPIO_Port, G_GPIO_Port, DP_GPIO_Port};
uint16_t segmentPins[8]       = {A_Pin, B_Pin, C_Pin, D_Pin, E_Pin, F_Pin, G_Pin, DP_Pin};




#endif
