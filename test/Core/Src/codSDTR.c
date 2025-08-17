#include "codSDTR.h"

// Definim variabila globală aici
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


// Segment patterns for digits 0-9 (1=ON)
int segmentMap[11][8] = {
		{1,1,1,1,1,1,0,0}, //0
		{0,1,1,0,0,0,0,0}, //1
		{1,1,0,1,1,0,1,0}, //2
		{1,1,1,1,0,0,1,0}, //3
		{0,1,1,0,0,1,1,0}, //4
		{1,0,1,1,0,1,1,0}, //5
		{1,0,1,1,1,1,1,0}, //6
		{1,1,1,0,0,0,0,0}, //7
		{1,1,1,1,1,1,1,0}, //8
		{1,1,1,1,0,1,1,0},  //9
		{0,0,0,0,0,0,1,0}  // debbuging
};

int digits[4] = {0,0,0,0};
int currentDigit = 0;
// global variable to track debug mode
uint32_t debugTimestamp = 0;
uint8_t debugActive = 0;

void Display_Update(void) {
    for (int digit = 0; digit < 4; digit++) {
        // Turn off all digits
        for (int i = 0; i < 4; i++)
            HAL_GPIO_WritePin(digitPorts[i], digitPins[i], GPIO_PIN_RESET);

        // Set segments for current digit
        for (int seg = 0; seg < 8; seg++) {
            HAL_GPIO_WritePin(segmentPorts[seg], segmentPins[seg],
                segmentMap[digits[digit]][seg] ? GPIO_PIN_SET : GPIO_PIN_RESET);
        }

        // Enable current digit
        HAL_GPIO_WritePin(digitPorts[digit], digitPins[digit], GPIO_PIN_SET);
        osDelay(2);  // Display each digit for 2ms
        HAL_GPIO_WritePin(digitPorts[digit], digitPins[digit], GPIO_PIN_RESET);
    }
}

void UpdateDisplayFromTime(void)
{
	uint32_t totalSeconds = totalTime / 1000;
	uint32_t minutes = totalSeconds / 60;
	uint32_t seconds = totalSeconds % 60;

	// MM:SS
	digits[0] = (minutes / 10) % 10;
	digits[1] =  minutes % 10;
	digits[2] = (seconds / 10) % 10;
	digits[3] =  seconds % 10;
}


void StartDefaultTask(void const * argument)
{
    uint32_t lastSampledTime = HAL_GetTick();

    for(;;)
    {
        uint32_t now = HAL_GetTick();

        switch(startFlag) {
        case 0: // idle
            break;

        case 1: // start or resume
            startTime       = now;          // remember momentul preluării
            lastRunTime     = now;          // for UART pacing in Task01
            lastSampledTime = now;          // for delta accumulation
            startFlag       = 2;            // transition to running
            break;

        case 2: // running
            totalTime += (now - lastSampledTime);
            lastSampledTime = now;
            break;

        case 3: // stop
            elapsedTime += now - startTime;
            totalTime    = elapsedTime;
            startFlag    = 0; // pause
            break;

        case 4: // reset
            elapsedTime = 0;
            totalTime   = 0;
            startFlag   = 0;
            break;
        }

        osDelay(1);
    }
}

void StartTask01(void const * argument)
{
    for(;;)
    {
        if (startFlag == 2)  // running
        {
            uint32_t currentTime = HAL_GetTick();
            if ((currentTime - lastRunTime) >= 500) // au trecut 500 ms
            {
                lastRunTime = currentTime;

                char buffer[50];
                int len = sprintf(buffer, "Timp: %lu ms\r\n", totalTime);
                HAL_UART_Transmit(&huart2, (uint8_t*)buffer, len, HAL_MAX_DELAY);
            }
        }
        else if (startFlag == 0)  // paused -> show final total
        {
            // optional: transmit once if needed
        }

        osDelay(1);
    }
}

void StartTask02(void const * argument)
{
	// Recepție UART activă permanent
	HAL_UART_Receive_IT(&huart2, &rxData, 1);

	for (;;)
	{
		uint32_t now = HAL_GetTick();

		if(debugActive)
		{
		    if(now - debugTimestamp < 2000)
		    {
		        // Keep showing debug values
		        Display_Update();
		    }
		    else
		    {
		        debugActive = 0; // Debug mode expired
		    }
		}
		else if(now - lastTimeDisplayed > 100)
		{
		    UpdateDisplayFromTime();
		    lastTimeDisplayed = HAL_GetTick();
		    Display_Update();
		}
		//osDelay(1);
	}

}

// Callback UART
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	if (huart->Instance == USART2)
	{
		// Debug: trimitem caracterul primit înapoi
		HAL_UART_Transmit(&huart2, &rxData, 1, HAL_MAX_DELAY);

		if (rxData == 'S')  // Start
		{
			HAL_GPIO_WritePin(GPIOA, LED_GREEN_Pin, GPIO_PIN_SET);
			startFlag = 1;
		}
		else if (rxData == 'T') // Stop
		{
			HAL_GPIO_WritePin(GPIOA, LED_GREEN_Pin, GPIO_PIN_RESET);
			startFlag = 3;
		}
		else if (rxData == 'R') // Reset (opțional)
		{
			startFlag = 4; // va reseta timerul
			HAL_GPIO_TogglePin(GPIOA, LED_GREEN_Pin);
		}
		else if (rxData == 'Z') // Debug: afișăm cifra 1
		{
			startFlag = 0;
			digits[0] = 1;
			digits[1] = 2;
			digits[2] = 3;
			digits[3] = 4;

			debugActive = 1;                 // enable debug
			debugTimestamp = HAL_GetTick();  // store the current time

			char dbgMsg[] = "DEBUG: \r\n";
			HAL_UART_Transmit(&huart2, (uint8_t*)dbgMsg, strlen(dbgMsg), HAL_MAX_DELAY);
		}
		// Re-armăm recepția
		HAL_UART_Receive_IT(&huart2, &rxData, 1);
	}
}
