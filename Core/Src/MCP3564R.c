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
	uint8_t RxData;

	//checks that ADC is connected
	status = MCP3564_CheckConnection();
	if(status != HAL_OK){ return 1; }

	/* --- INSTRUCTION TO CONFIG0 --- */

	//01 = device address, 0001 = CONFIG0, 10 = incremental write
	uint8_t writeCommand1 = 0b01000110;
	//11 = not shutdown, 11 = internal clk (analog), 00 = no current applied, 11 = conversion mode
	uint8_t writeCONFIG0 = 0b11110011;

	//set CS low
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_4, GPIO_PIN_RESET);

	//send write command
	status = HAL_SPI_TransmitReceive(MCP3564_hspi, &writeCommand1, &RxData, 1, 1000);
	if(status == HAL_ERROR){
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_4, GPIO_PIN_SET);
		return 1;
	}

	//write to config0 register to enable conversion mode
	status = HAL_SPI_TransmitReceive(MCP3564_hspi, &writeCONFIG0, &RxData, 1, 1000);
	if(status == HAL_ERROR){
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_4, GPIO_PIN_SET);
		return 1;
	}

	//set CS high
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_4, GPIO_PIN_SET);

	/* --- INSTRUCTION TO CONFIG3 --- */

	//01 = device address, 0100 = CONFIG3, 10 = incremental write
	uint8_t writeCommand2 = 0b01010010;
	//11 = Cont. Conversion mode, 00 = 24 bit ADC data 0000 = default other settings
	uint8_t writeCONFIG3 = 0b11000000;

	//set CS low
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_4, GPIO_PIN_RESET);

	//send write command
	status = HAL_SPI_TransmitReceive(MCP3564_hspi, &writeCommand2, &RxData, 1, 1000);
	if(status == HAL_ERROR){
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_4, GPIO_PIN_SET);
		return 1;
	}

	//write to config register to enable conversion mode
	status = HAL_SPI_TransmitReceive(MCP3564_hspi, &writeCONFIG3, &RxData, 1, 1000);
	if(status == HAL_ERROR){
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_4, GPIO_PIN_SET);
		return 1;
	}

	//set CS high
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_4, GPIO_PIN_SET);

	/* --- INSTRUCTION TO MUX --- */

		//01 = device address, 0110 = MUX, 10 = incremental write
		uint8_t writeCommand3 = 0b01011010;
		//0000 = CH0, 1000 = Agnd
		uint8_t writeMUX = 0b00001000;

		//set CS low
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_4, GPIO_PIN_RESET);

		//send write command
		status = HAL_SPI_TransmitReceive(MCP3564_hspi, &writeCommand3, &RxData, 1, 1000);
		if(status == HAL_ERROR){
			HAL_GPIO_WritePin(GPIOC, GPIO_PIN_4, GPIO_PIN_SET);
			return 1;
		}

		//write to config register to enable conversion mode
		status = HAL_SPI_TransmitReceive(MCP3564_hspi, &writeMUX, &RxData, 1, 1000);
		if(status == HAL_ERROR){
			HAL_GPIO_WritePin(GPIOC, GPIO_PIN_4, GPIO_PIN_SET);
			return 1;
		}

		//set CS high
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_4, GPIO_PIN_SET);

		/* --- READING IRQ --- */

		//01 = device address, 0101 = IRQ, 01 = static read
		uint8_t readCommand1 = 0b01010101;
		uint8_t READdata = 0;
		//set CS low
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_4, GPIO_PIN_RESET);

		//send read command
		status = HAL_SPI_TransmitReceive(MCP3564_hspi, &readCommand1, &RxData, 1, 1000);
		if(status == HAL_ERROR){
			HAL_GPIO_WritePin(GPIOC, GPIO_PIN_4, GPIO_PIN_SET);
			return 1;
		}

		//read IRQ data
		status = HAL_SPI_Receive(MCP3564_hspi, &READdata, 1, 1000);
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
	//Checks if connected by reading register 0x1 and analyzing the STATUS byte sent over
	HAL_StatusTypeDef status;

	//01 = device address, 0001 = CONFIG0, 01 = static read
	uint8_t TxData = 0b01000101;
	uint8_t RxData = 0; // create space for status byte

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
	uint8_t data[3] = {0,0,0};

	//01 = device address, 0000 = ADCDATA, 01 = static read
	uint8_t command = 0b01000001;
	uint8_t RxData = 0; //for comparison to validate connection

	//CS low
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_4, GPIO_PIN_RESET);

	status = HAL_SPI_TransmitReceive(MCP3564_hspi, &command, &RxData, 1, 1000);
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
	*channelReading |= (uint32_t)data[0] << 16;
	*channelReading |= (uint32_t)data[1] << 8;
	*channelReading |= (uint32_t)data[2];

	return status;
}

