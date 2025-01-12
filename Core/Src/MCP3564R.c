/*
 * MCP3564R.c
 *
 *  Created on: Nov 4, 2024
 *      Author: williamgomez
 */
#include "MCP3564R.h"

//define external variables so compiler doesn't throw errors
SPI_HandleTypeDef* MCP3564_hspi;
//GPIO_TypeDef* GPIOpinLetter;
//uint16_t GPIO_PIN_Number;

//todo: make all pin assignments generic (after testing, for simplicity's sake)
//Initializes MCP3564 on a particular SPI bus, returns 0 if successful, 1 if failed
int MCP3564_Init(SPI_HandleTypeDef* hspi/*, GPIO_TypeDef* GPIOpinLetter, uint16_t GPIO_PIN_Number*/){
	HAL_StatusTypeDef status;
	MCP3564_hspi = hspi;
	uint8_t RxData1 = 0;
	uint8_t RxData2 = 0;

	//01 = device address, 0001 = CONFIG0, 10 = incremental write
	uint8_t writeCommand = 0b01000110;
	//1 = default Vref, 1 = not partial shutdown, 00 = extern. digital clk, 00 = no current applied, 11 = conversion mode
	uint8_t configWrite = 0b11000011;

	//connects hspi to ADC/check that its connected
	status = MCP3564_CheckConnection();
	if(status != HAL_OK){ return 1; }

	//set CS low
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_4, GPIO_PIN_RESET);

	//send write command
	status = HAL_SPI_TransmitReceive(MCP3564_hspi, &writeCommand, &RxData1, 1, 1000);
	if(status == HAL_ERROR){
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_4, GPIO_PIN_SET);
		return 1;
	}

	//write to config register to enable conversion mode
	status = HAL_SPI_TransmitReceive(MCP3564_hspi, &configWrite, &RxData2, 1, 1000);
	if(status == HAL_ERROR){
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_4, GPIO_PIN_SET);
		return 1;
	}

	//set CS high
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_4, GPIO_PIN_SET);

	//return 0 if successfully reached this point
	return 0;

}

// Checks if the MCP3564 is connected to the SPI bus, returns 0 if successful, 1 if failed
int MCP3564_CheckConnection(){
	//Checks if connected by verifying the mode, accessed through register 0x1
	//am I able to read something from this?
	//TxData: CMD byte → CMD[7:6] = 01 (address of device)
	//		           → CMD[5:2] = 0x1
	//		           → CMD[1:0] = 01 (static, for now)
	HAL_StatusTypeDef status;

	//01 = device address, 0001 = CONFIG0, 01 = static read
	uint8_t TxData = 0b01000101;
	uint8_t RxData = 0; // create space for status byte

	//Pin C4 is our manual chip select line for the MCP3564R
	//Set ~CS low to begin reading and writing to chip
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_4, GPIO_PIN_RESET);
	status = HAL_SPI_TransmitReceive(MCP3564_hspi, &TxData, &RxData, 1, HAL_MAX_DELAY);
	if (status == HAL_ERROR) {
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_4, GPIO_PIN_SET);
		return status;
	}
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_4, GPIO_PIN_SET);

	//isolate device bits to check if STAT[4] is complement of STAT[3]
	uint8_t STAT4 = (RxData >> 4) & 0x01;
	uint8_t STAT3 = (RxData >> 3) & 0x01;
	if (STAT4 == STAT3) {
		return 1; // device not connected
	} else if (STAT4 != STAT3){
		return 0; //device connected
	}
	return status;
}

// Reads voltage calculated from MCP3564, returns 0 if successful, 1 if failed
int MCP3564_ReadChannel(int32_t *channelReading){

	HAL_StatusTypeDef status;
	uint8_t data[3];
	uint8_t command = 0b01000001;

	//CS low
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_4, GPIO_PIN_RESET);
	//returns 0 if no problem
	//01 = device address, 0000 = ADCDATA, 01 = static read
	status = HAL_SPI_Transmit (MCP3564_hspi, &command, 1, 1000);
	if(status == HAL_ERROR){
		return status;
	}
	status = HAL_SPI_Receive (MCP3564_hspi, data, 3, 1000);
	if(status == HAL_ERROR){
			return status;
	}
	//CS high
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_4, GPIO_PIN_SET);

	//stitches the 3 bytes of data together
	//todo: verify that these bits are being stitched together correctly
	*channelReading = ((int32_t)data[0] << 16 | (int32_t)data[1] << 8 | (int32_t)data[2]);

	return status;
}

