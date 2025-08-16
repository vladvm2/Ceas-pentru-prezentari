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
uint16_t segmentPins[8]       = {A_Pin, B_Pin, C_Pin, D_Pin,
                                 E_Pin, F_Pin, G_Pin, DP_Pin};




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
int currentDigit = 6;
// global variable to track debug mode
uint32_t debugTimestamp = 0;
uint8_t debugActive = 0;



// Call this in StartTask01 every 250ms
/*
void Display_Update(void)
{

	// Turn off all digits
	for(int i=0; i<4; i++)
		HAL_GPIO_WritePin(digitPorts[i], digitPins[i], GPIO_PIN_RESET);

	// Set segments for current digit
	for(int i=0;i<8;i++)
	{
		HAL_GPIO_WritePin(segmentPorts[i], segmentPins[i],
				segmentMap[digits[currentDigit]][i] ? GPIO_PIN_SET : GPIO_PIN_RESET);
	}

	// Enable current digit
	HAL_GPIO_WritePin(digitPorts[currentDigit], digitPins[currentDigit], GPIO_PIN_SET);

	// Next digit
	currentDigit = (currentDigit + 1) % 4;
}*/

void Display_Update(void)
{
    for(int i=0; i<4; i++)
        HAL_GPIO_WritePin(digitPorts[i], digitPins[i], GPIO_PIN_RESET);
    HAL_GPIO_WritePin(digitPorts[currentDigit], digitPins[currentDigit], GPIO_PIN_SET);


    for(int i=0; i<8; i++)
    {
        HAL_GPIO_WritePin(segmentPorts[i], segmentPins[i],
        		segmentMap[digits[currentDigit]][i] ? GPIO_PIN_SET : GPIO_PIN_RESET);
    }

    // Small delay for visibility
    osDelay(1);  // 2 ms is usually enough

    // Move to the next digit
    currentDigit = (currentDigit + 1) % 4;
}


void UpdateDisplayFromTime(uint32_t elapsedTime)
{
	// elapsedTime is in ms → convert to total seconds
	uint32_t totalSeconds = elapsedTime / 1000;

	uint32_t minutes = totalSeconds / 60;
	uint32_t seconds = totalSeconds % 60;

	// Fill digits for MM:SS
	digits[0] = (minutes / 10) % 10; // tens of minutes
	digits[1] = minutes % 10;        // ones of minutes
	digits[2] = (seconds / 10) % 10; // tens of seconds
	digits[3] = seconds % 10;        // ones of seconds
}

void StartDefaultTask(void const * argument)
{
	for(;;)
	{
		osDelay(1);
	}
}

void StartTask01(void const * argument)
{
	for(;;)
	{
		if (startFlag == 1)  // Start primit sau reluare
		{
			startTime = HAL_GetTick(); // momentul reluării
			lastRunTime = startTime;
			startFlag = 2; // semnal că timerul rulează
		}

		if (startFlag == 2)  // Timerul rulează
		{
			uint32_t currentTime = HAL_GetTick();
			if ((currentTime - lastRunTime) >= 500) // au trecut 500 ms
			{
				lastRunTime = currentTime;

				// Calculăm timpul total până acum
				totalTime = elapsedTime + (currentTime - startTime);

				// Trimitem timpul prin UART
				char buffer[50];
				int len = sprintf(buffer, "Timp: %lu ms\r\n", totalTime);
				HAL_UART_Transmit(&huart2, (uint8_t*)buffer, len, HAL_MAX_DELAY);

				// HAL_GPIO_TogglePin(GPIOA, LED_GREEN_Pin); // optional
			}
		}
		else if (startFlag == 3)  // Stop primit
		{
			// Acumulăm timpul până la oprire
			elapsedTime += HAL_GetTick() - startTime;
			startFlag = 0; // pauză

			// Trimitem timpul total acumulat
			char buffer[50];
			int len = sprintf(buffer, "Timp total: %lu ms\r\n", elapsedTime);
			HAL_UART_Transmit(&huart2, (uint8_t*)buffer, len, HAL_MAX_DELAY);
		}
		else if (startFlag == 4)  // Reset primit
		{
			elapsedTime = 0;
			startFlag = 0; // reset complet
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

	    // Check if debug mode is active and less than 2 sec has passed
	    if(debugActive && (now - debugTimestamp < 2000))
	    {
	        // Do not update time, just refresh the debug display
	        Display_Update();
	    }
	    else
	    {
	        debugActive = 0;               // disable debug mode after 2 sec
	        UpdateDisplayFromTime(totalTime); // normal time update
	        Display_Update();
	    }
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
		    digits[0] = 10;
		    digits[1] = 10;
		    digits[2] = 10;
		    digits[3] = 10;

		    debugActive = 1;                 // enable debug
		    debugTimestamp = HAL_GetTick();  // store the current time

		    char dbgMsg[] = "DEBUG: \r\n";
		    HAL_UART_Transmit(&huart2, (uint8_t*)dbgMsg, strlen(dbgMsg), HAL_MAX_DELAY);
		}


		// Re-armăm recepția
		HAL_UART_Receive_IT(&huart2, &rxData, 1);
	}
}
