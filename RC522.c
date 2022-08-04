//khai bao thu vien
#include <wiringPi.h>
#include <wiringPiSPI.h>
#include <stdio.h>
#include <mysql.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
typedef enum {
	// the duoc quet dung
	MI_OK = 0,
	MI_NOTAGERR,
	MI_ERR
} TM_MFRC522_Status_t;
/* MFRC522 Commands */
#define PCD_IDLE						0x00   //NO action; Cancel the current command
#define PCD_AUTHENT						0x0E   //Authentication Key
#define PCD_RECEIVE						0x08   //Receive Data
#define PCD_TRANSMIT					0x04   //Transmit data
#define PCD_TRANSCEIVE					0x0C   //Transmit and receive data,
#define PCD_RESETPHASE					0x0F   //Reset
#define PCD_CALCCRC						0x03   //CRC Calculate

/* Mifare_One card command word */
#define PICC_REQIDL						0x26   // find the antenna area does not enter hibernation
#define PICC_REQALL						0x52   // find all the cards antenna area
#define PICC_ANTICOLL					0x93   // anti-collision
#define PICC_SElECTTAG					0x93   // election card
#define PICC_AUTHENT1A					0x60   // authentication key A
#define PICC_AUTHENT1B					0x61   // authentication key B
#define PICC_READ						0x30   // Read Block
#define PICC_WRITE						0xA0   // write block
#define PICC_DECREMENT					0xC0   // debit
#define PICC_INCREMENT					0xC1   // recharge
#define PICC_RESTORE					0xC2   // transfer block data to the buffer
#define PICC_TRANSFER					0xB0   // save the data in the buffer
#define PICC_HALT						0x50   // Sleep

/* MFRC522 Registers */
//Page 0: Command and Status
#define MFRC522_REG_RESERVED00			0x00    
#define MFRC522_REG_COMMAND				0x01    
#define MFRC522_REG_COMM_IE_N			0x02    
#define MFRC522_REG_DIV1_EN				0x03    
#define MFRC522_REG_COMM_IRQ			0x04    
#define MFRC522_REG_DIV_IRQ				0x05
#define MFRC522_REG_ERROR				0x06    
#define MFRC522_REG_STATUS1				0x07    
#define MFRC522_REG_STATUS2				0x08    
#define MFRC522_REG_FIFO_DATA			0x09
#define MFRC522_REG_FIFO_LEVEL			0x0A
#define MFRC522_REG_WATER_LEVEL			0x0B
#define MFRC522_REG_CONTROL				0x0C
#define MFRC522_REG_BIT_FRAMING			0x0D
#define MFRC522_REG_COLL				0x0E
#define MFRC522_REG_RESERVED01			0x0F
//Page 1: Command 
#define MFRC522_REG_RESERVED10			0x10
#define MFRC522_REG_MODE				0x11
#define MFRC522_REG_TX_MODE				0x12
#define MFRC522_REG_RX_MODE				0x13
#define MFRC522_REG_TX_CONTROL			0x14
#define MFRC522_REG_TX_AUTO				0x15
#define MFRC522_REG_TX_SELL				0x16
#define MFRC522_REG_RX_SELL				0x17
#define MFRC522_REG_RX_THRESHOLD		0x18
#define MFRC522_REG_DEMOD				0x19
#define MFRC522_REG_RESERVED11			0x1A
#define MFRC522_REG_RESERVED12			0x1B
#define MFRC522_REG_MIFARE				0x1C
#define MFRC522_REG_RESERVED13			0x1D
#define MFRC522_REG_RESERVED14			0x1E
#define MFRC522_REG_SERIALSPEED			0x1F
//Page 2: CFG    
#define MFRC522_REG_RESERVED20			0x20  
#define MFRC522_REG_CRC_RESULT_M		0x21
#define MFRC522_REG_CRC_RESULT_L		0x22
#define MFRC522_REG_RESERVED21			0x23
#define MFRC522_REG_MOD_WIDTH			0x24
#define MFRC522_REG_RESERVED22			0x25
#define MFRC522_REG_RF_CFG				0x26
#define MFRC522_REG_GS_N				0x27
#define MFRC522_REG_CWGS_PREG			0x28
#define MFRC522_REG__MODGS_PREG			0x29
#define MFRC522_REG_T_MODE				0x2A
#define MFRC522_REG_T_PRESCALER			0x2B
#define MFRC522_REG_T_RELOAD_H			0x2C
#define MFRC522_REG_T_RELOAD_L			0x2D
#define MFRC522_REG_T_COUNTER_VALUE_H	0x2E
#define MFRC522_REG_T_COUNTER_VALUE_L	0x2F
//Dummy byte
#define MFRC522_DUMMY					0x00

#define MFRC522_MAX_LEN					16
unsigned char buffer[10];

void TM_MFRC522_WriteRegister(unsigned char addr, unsigned char val) {
    buffer[0]=(addr << 1) & 0x7E;
    buffer[1]=val;
    wiringPiSPIDataRW(0, buffer, 2);
}

unsigned char TM_MFRC522_ReadRegister(unsigned char addr) {
    unsigned char val;
    buffer[0]=((addr << 1) & 0x7E) | 0x80;
    //datasheet yeu cau phai gui vao 1 bien nhap ,sau do moi tra gia tri ve
    buffer[1]=MFRC522_DUMMY;
    wiringPiSPIDataRW(0, buffer, 2);
    val = buffer[1];
	return val;	
}

void TM_MFRC522_SetBitMask(unsigned char reg, unsigned char mask) {
	TM_MFRC522_WriteRegister(reg, TM_MFRC522_ReadRegister(reg) | mask);
}

void TM_MFRC522_ClearBitMask(unsigned char reg, unsigned char mask){
	TM_MFRC522_WriteRegister(reg, TM_MFRC522_ReadRegister(reg) & (~mask));
}

void TM_MFRC522_Reset(void) {
	TM_MFRC522_WriteRegister(MFRC522_REG_COMMAND, PCD_RESETPHASE);
} 

void TM_MFRC522_AntennaOn(void) {
	unsigned char temp;

	temp = TM_MFRC522_ReadRegister(MFRC522_REG_TX_CONTROL);
	if (!(temp & 0x03)) {
		TM_MFRC522_SetBitMask(MFRC522_REG_TX_CONTROL, 0x03);
	}
}

void TM_MFRC522_AntennaOff(void) {
	TM_MFRC522_ClearBitMask(MFRC522_REG_TX_CONTROL, 0x03);
}

void TM_MFRC522_Init(void) {
	wiringPiSPISetup(0,9000000);

	TM_MFRC522_Reset();

	TM_MFRC522_WriteRegister(MFRC522_REG_T_MODE, 0x8D);
	TM_MFRC522_WriteRegister(MFRC522_REG_T_PRESCALER, 0x3E);
	TM_MFRC522_WriteRegister(MFRC522_REG_T_RELOAD_L, 30);           
	TM_MFRC522_WriteRegister(MFRC522_REG_T_RELOAD_H, 0);

	/* 48dB gain */
	TM_MFRC522_WriteRegister(MFRC522_REG_RF_CFG, 0x70);
	
	TM_MFRC522_WriteRegister(MFRC522_REG_TX_AUTO, 0x40);
	TM_MFRC522_WriteRegister(MFRC522_REG_MODE, 0x3D);

	TM_MFRC522_AntennaOn();		//Open the antenna
}

TM_MFRC522_Status_t TM_MFRC522_ToCard(unsigned char command, unsigned char *sendData, unsigned char sendLen, unsigned char *backData, unsigned int *backLen) {
	TM_MFRC522_Status_t status = MI_ERR;
	unsigned char irqEn = 0x00;
	unsigned char waitIRq = 0x00;
	unsigned char lastBits;
	unsigned char n;
	unsigned int i;

	switch (command) {
		case PCD_AUTHENT: {
			irqEn = 0x12;
			waitIRq = 0x10;
			break;
		}
		case PCD_TRANSCEIVE: {
			irqEn = 0x77;
			waitIRq = 0x30;
			break;
		}
		default:
			break;
	}

	TM_MFRC522_WriteRegister(MFRC522_REG_COMM_IE_N, irqEn | 0x80);
	TM_MFRC522_ClearBitMask(MFRC522_REG_COMM_IRQ, 0x80);
	TM_MFRC522_SetBitMask(MFRC522_REG_FIFO_LEVEL, 0x80);

	TM_MFRC522_WriteRegister(MFRC522_REG_COMMAND, PCD_IDLE);

	//Writing data to the FIFO
	for (i = 0; i < sendLen; i++) {   
		TM_MFRC522_WriteRegister(MFRC522_REG_FIFO_DATA, sendData[i]);    
	}

	//Execute the command
	TM_MFRC522_WriteRegister(MFRC522_REG_COMMAND, command);
	if (command == PCD_TRANSCEIVE) {    
		TM_MFRC522_SetBitMask(MFRC522_REG_BIT_FRAMING, 0x80);		//StartSend=1,transmission of data starts  
	}   

	//Waiting to receive data to complete
	i = 2000;	//i according to the clock frequency adjustment, the operator M1 card maximum waiting time 25ms???
	do {
		//CommIrqReg[7..0]
		//Set1 TxIRq RxIRq IdleIRq HiAlerIRq LoAlertIRq ErrIRq TimerIRq
		n = TM_MFRC522_ReadRegister(MFRC522_REG_COMM_IRQ);
		i--;
	} while ((i!=0) && !(n&0x01) && !(n&waitIRq));

	TM_MFRC522_ClearBitMask(MFRC522_REG_BIT_FRAMING, 0x80);			//StartSend=0

	if (i != 0)  {
		if (!(TM_MFRC522_ReadRegister(MFRC522_REG_ERROR) & 0x1B)) {
			status = MI_OK;
			if (n & irqEn & 0x01) {   
				status = MI_NOTAGERR;			
			}

			if (command == PCD_TRANSCEIVE) {
				n = TM_MFRC522_ReadRegister(MFRC522_REG_FIFO_LEVEL);
				lastBits = TM_MFRC522_ReadRegister(MFRC522_REG_CONTROL) & 0x07;
				if (lastBits) {   
					*backLen = (n - 1) * 8 + lastBits;   
				} else {   
					*backLen = n * 8;   
				}

				if (n == 0) {   
					n = 1;    
				}
				if (n > MFRC522_MAX_LEN) {   
					n = MFRC522_MAX_LEN;   
				}

				//Reading the received data in FIFO
				for (i = 0; i < n; i++) {   
					backData[i] = TM_MFRC522_ReadRegister(MFRC522_REG_FIFO_DATA);    
				}
			}
		} else {   
			status = MI_ERR;  
		}
	}

	return status;
}

TM_MFRC522_Status_t TM_MFRC522_Request(unsigned char reqMode, unsigned char *TagType) {
	TM_MFRC522_Status_t status;  
	unsigned int backBits;			//The received data bits

	TM_MFRC522_WriteRegister(MFRC522_REG_BIT_FRAMING, 0x07);		//TxLastBists = BitFramingReg[2..0]	???

	TagType[0] = reqMode;
	status = TM_MFRC522_ToCard(PCD_TRANSCEIVE, TagType, 1, TagType, &backBits);

	if ((status != MI_OK) || (backBits != 0x10)) {    
		status = MI_ERR;
	}

	return status;
}

TM_MFRC522_Status_t TM_MFRC522_Anticoll(unsigned char *serNum) {
	TM_MFRC522_Status_t status;
	unsigned char i;
	unsigned char serNumCheck = 0;
	unsigned int unLen;

	TM_MFRC522_WriteRegister(MFRC522_REG_BIT_FRAMING, 0x00);		//TxLastBists = BitFramingReg[2..0]

	serNum[0] = PICC_ANTICOLL;
	serNum[1] = 0x20;
	status = TM_MFRC522_ToCard(PCD_TRANSCEIVE, serNum, 2, serNum, &unLen);

	if (status == MI_OK) {
		//Check card serial number
		for (i = 0; i < 4; i++) {   
			serNumCheck ^= serNum[i];
		}
		if (serNumCheck != serNum[i]) {   
			status = MI_ERR;    
		}
	}
	return status;
}

void TM_MFRC522_CalculateCRC(unsigned char *pIndata, unsigned char len, unsigned char *pOutData) {
	unsigned char i, n;

	TM_MFRC522_ClearBitMask(MFRC522_REG_DIV_IRQ, 0x04);			//CRCIrq = 0
	TM_MFRC522_SetBitMask(MFRC522_REG_FIFO_LEVEL, 0x80);			//Clear the FIFO pointer
	//Write_MFRC522(CommandReg, PCD_IDLE);

	//Writing data to the FIFO	
	for (i = 0; i < len; i++) {   
		TM_MFRC522_WriteRegister(MFRC522_REG_FIFO_DATA, *(pIndata+i));   
	}
	TM_MFRC522_WriteRegister(MFRC522_REG_COMMAND, PCD_CALCCRC);

	//Wait CRC calculation is complete
	i = 0xFF;
	do {
		n = TM_MFRC522_ReadRegister(MFRC522_REG_DIV_IRQ);
		i--;
	} while ((i!=0) && !(n&0x04));			//CRCIrq = 1

	//Read CRC calculation result
	pOutData[0] = TM_MFRC522_ReadRegister(MFRC522_REG_CRC_RESULT_L);
	pOutData[1] = TM_MFRC522_ReadRegister(MFRC522_REG_CRC_RESULT_M);
}

void TM_MFRC522_Halt(void) {
	unsigned int unLen;
	unsigned char buff[4]; 

	buff[0] = PICC_HALT;
	buff[1] = 0;
	TM_MFRC522_CalculateCRC(buff, 2, &buff[2]);

	TM_MFRC522_ToCard(PCD_TRANSCEIVE, buff, 4, buff, &unLen);
} 
//check ma the
TM_MFRC522_Status_t TM_MFRC522_Check(unsigned char* id) {
	TM_MFRC522_Status_t status;
	//Find cards, return card type
	status = TM_MFRC522_Request(PICC_REQIDL, id);	
	if (status == MI_OK) {
		//Card detected
		//Anti-collision, return card serial number 4 bytes
		status = TM_MFRC522_Anticoll(id);	
	}
	TM_MFRC522_Halt();			//Command card into hibernation 

	return status;
}

void main()
{
    TM_MFRC522_Init();
    unsigned char CardID[5];
    char szBuff[100];
    wiringPiSetup();
    
    pinMode(7,OUTPUT);
    digitalWrite(7,1);
    
    MYSQL *conn;
    MYSQL_RES *res; //bien dung de luu tru du lieu database
    MYSQL_ROW row;
    
    char *server = "localhost";
    char *user = "root";
    char *password = "123456"; /* set me first */
    char *database = "rfid";
    
    conn=mysql_init(NULL);
    unsigned int num_field;
    unsigned char mathe[10];
    int kiem_tra=0;
    char cmd[200];
    //khai bao gipo21 OUTPUT
    pinMode(29,OUTPUT);
    digitalWrite(29,0);
    //gui ma the xuong database
    int tg_hien_tai=0;
    int tg_truoc=0;
    int sv=0;
    while(1)
    {
        if(TM_MFRC522_Check(CardID) == MI_OK) {
	//in ma doc tu the
            printf("ID: 0x%02X%02X%02X%02X%02X\n", CardID[0], CardID[1], CardID[2], CardID[3], CardID[4]);
	//gop cac ma the lai voi nhau
            sprintf(mathe,"%02X%02X%02X%02X%02X",CardID[0], CardID[1], CardID[2], CardID[3], CardID[4]);
	    
	//ket noi toi database
            conn=mysql_init(NULL);
            if(!mysql_real_connect(conn,server,user,password,database,0,NULL,0))
            {
                fprintf(stderr,"%s\n",mysql_error(conn));
            }
	    
	//lay cot id tu bang id card
            mysql_query(conn,"select id from id_card");
            res=mysql_use_result(conn);
	//dem so cot co trong bang
	//ham dem : mysql_num_fields();
            num_field = mysql_num_fields(res);
            while ((row = mysql_fetch_row(res))) //mysql_fetch_row :truy cap tung hang
            { 
                for (int i=0; i<num_field;i++)
                {	
                    if(strcmp(mathe,row[i])==0)
                    {
			kiem_tra=1;
		    }
                    //printf("%s \t",row[i]);
                }
            }
	    if(kiem_tra==1)
            {
		    // lay thoi gian 
                time_t mytime = time(NULL);
                char * time_str = ctime(&mytime);
                time_str[strlen(time_str)-1] = '\0';
		//dien vao bang diem danh
                sprintf(cmd,"insert into diem_danh(id,thoi_gian) values ('%s','%s')",mathe,time_str);
                mysql_query(conn,cmd);
		sprintf(cmd,"insert into trang_thai(tt) values (%d)",1);
                mysql_query(conn,cmd);
		kiem_tra=0;
		//servo cho 2s roi mo
		delay(2000);
		digitalWrite(29,1);
		delay(2);
		digitalWrite(29,0);
		delay(18);
		
		//servo cho 3s roi dong
		delay(3000);
	        digitalWrite(29,1);
	        delay(1);
	        digitalWrite(29,0);
	        delay(19);
		
		
            }
            else
            {
                sprintf(cmd,"insert into dang_ky(id) values ('%s')",mathe);
                mysql_query(conn,cmd);
		sprintf(cmd,"insert into trang_thai(tt) values (%d)",0);
                mysql_query(conn,cmd);
            }
            // send SQL query 
            mysql_close(conn);
            delay(1000);
        }
	tg_hien_tai=millis();
	if(tg_hien_tai-tg_truoc>1000)
	{
		conn=mysql_init(NULL);
		if(!mysql_real_connect(conn,server,user,password,database,0,NULL,0))
		{
			fprintf(stderr,"%s\n",mysql_error(conn));
		}
		mysql_query(conn,"select tt from servo where stt=1");
		res=mysql_use_result(conn);
		row = mysql_fetch_row(res);
		sv=atoi(row[0]);
		if(sv==1)
		{	
			digitalWrite(29,1);
			delay(2);
			digitalWrite(29,0);
			delay(18);
		}
		else
		{	
			digitalWrite(29,1);
			delay(1);
			digitalWrite(29,0);
			delay(19);
		}
		tg_truoc=tg_hien_tai;
		mysql_close(conn);
		//delay(1000);	
	}
    }
}
