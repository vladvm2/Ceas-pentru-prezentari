#ifndef CODSDTR_H
#define CODSDTR_H

#include "main.h"
#include "cmsis_os.h"
#include <stdio.h>        // <-- Add this

volatile uint8_t startFlag = 0;
volatile uint32_t startTime = 0;
volatile uint32_t elapsedTime = 0;
volatile uint32_t lastRunTime = 0;
volatile uint32_t totalTime = 0;


// Variabilă globală de RX (definită în codSDTR.c)
extern uint8_t rxData;

// Protocoale funcții task-uri
void StartDefaultTask(void const * argument);
void StartTask01(void const * argument);
void StartTask02(void const * argument);

#endif
