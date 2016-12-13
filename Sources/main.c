/**
* File name : main.c
* Project name: BootLoader
*
* Author :  Manuel Madrigal Valenzuela
*           Efraín Duarte López
*           Daniela Becerra
*/


#include <hidef.h> /* for EnableInterrupts macro */
#include "derivative.h" /* include peripheral declarations */
#include "MCUinit.h"
#include "types.h"

/*
 * In ANSI-C it is normally not allowed to cast an object pointer to a function pointer or a 
 * function pointer to an object pointer. Since this operation is needed in the code the Warning
 * message generated is being disabled
 */
#pragma MESSAGE DISABLE C1805


#define Page_Erase   PGM[21]=0x40; temp = ((unsigned char(*)(unsigned int))(PGM))
#define Program_Byte PGM[21]=0x20; temp = ((unsigned char(*)(unsigned int, unsigned char))(PGM))

//Array of opcode instructions of the Erase/Program function in the HCS08 family
volatile unsigned char PGM[59]  = {  
0x87,0xC6,0x18,0x25,0xA5,0x10,0x27,0x08,0xC6,0x18,0x25,0xAA,0x10,0xC7,0x18,0x25,
0x9E,0xE6,0x01,0xF7,0xA6,0x20,0xC7,0x18,0x26,0x45,0x18,0x25,0xF6,0xAA,0x80,0xF7,
0x9D,0x9D,0x9D,0x9D,0x45,0x18,0x25,0xF6,0xF7,0xF6,0xA5,0x30,0x27,0x04,0xA6,0xFF,
0x20,0x07,0xC6,0x18,0x25,0xA5,0x40,0x27,0xF9,0x8A,0x81};
/*  The opcode above represents this set of instructions  
    if (FSTAT&0x10){                     //Check to see if FACCERR is set
        FSTAT = FSTAT | 0x10;            //write a 1 to FACCERR to clear
    }
    (*((volatile unsigned char *)(Address))) = data;  //write to somewhere in flash
    FSTAT = FSTAT | 0x80;                //Put FCBEF at 1.
    _asm NOP;                            //Wait 4 cycles
    _asm NOP;
    _asm NOP;
    _asm NOP;
    if (FSTAT&0x30){                     //check to see if FACCERR or FVIOL are set
    return 0xFF;                         //if so, error.
    }
    while ((FSTAT&0x40)==0){             //else wait for command to complete
        ;
    }*/

unsigned char temp;
u8 NumVectores;
u16 index;
unsigned char NewCode[200];
bool ReceivedDataFlag;
u16 wait_time_ms;
u16 i;
u16 posSize;
u16 posDir;
u8 size;
u16* dir;
u8 count;
u16 testDir;
u8 testData;
bool SCILoaded;
u8*  interruptVectors;

interrupt VectorNumber_Vscirx void SCI_Rx_IRS()
{
   (void) SCIS1;
   NewCode[index++]=SCID;
   ReceivedDataFlag=TRUE;
   wait_time_ms=1000;
}

void main(void) {
  EnableInterrupts;
  MCU_init();
  
  count=0;
  size=0;
  index=0;
  ReceivedDataFlag=FALSE;
  SCILoaded=FALSE;
  
  //Backup of the interrupt vectors----------------------------------------------
  interruptVectors=(u8*)0xFFD0;
  for(i=0;i<48;i++)
  {
     NewCode[i]=*(interruptVectors+i);
  }
  
  //Load the direction of the RESET and SCI interruption of the Bootloader------
  temp = Page_Erase(0xFE00);
  Program_Byte(0xFFFE,0xFA);
  Program_Byte(0xFFFF,0x7B);
  Program_Byte(0xFFE0,0xFA);
  Program_Byte(0xFFE1,0xCB);
       
  //Configure the Serial Communication Interface----------------------------------
  SCIBD=26;//Configure BR
  SCIC2_RE=1;//Receiver enable
  SCIC2_RIE=1;//receiver interrupt enable
  
  //Configure the MTIM Timer to 5 secs--------------------------------------------
  wait_time_ms=5000;// 5 segundos
  MTIMSC_TSTP=0;
  MTIMCLK=0b00000100;   //Bus clock y PS de /16
  MTIMMOD=250;          //250 cuentas de 4 us equivalente a 1ms
  
  //Wait for the timer to expire or to receive a new program----------------------
  do{
     __RESET_WATCHDOG();
     if(MTIMSC_TOF==1)
     {
        (void) MTIMSC;
        MTIMSC_TOF=0;
        wait_time_ms--;
     }
  }while(wait_time_ms!=0);//Wait for 5 seconds to receive an upgrade

  SCIC2_RIE=0;//receiver interrupt disable
  
  
  //If no new program is received reload the interrupt vectors and run the program
  //already loaded------------------------------------------------------------------
  if(ReceivedDataFlag==FALSE)
  {
     temp = Page_Erase(0xFE00);
     for(i=0;i<48;i++)
     {
        Program_Byte((int)interruptVectors+i,NewCode[i]);
     }
     asm("JMP 0xE000");
  }
  
  //Erase Flash to upload the new program-------------------------------------------
  for(i=0;i<13;i++)           
  {
     __RESET_WATCHDOG();
     temp = Page_Erase(0xE000+(i*0x200));
  }
  temp = Page_Erase(0xFE00);

  wait_time_ms=260;     //wait for the flash to finish erasing the block memories
  do{
     __RESET_WATCHDOG();
     if(MTIMSC_TOF==1)
     {
        (void) MTIMSC;
        MTIMSC_TOF=0;
        wait_time_ms--;
     }
  }while(wait_time_ms!=0);
  
  //Decode the array and upload the new program to FLASH------------------------------
  NumVectores=NewCode[0];
  for(i=1;i<index;i++){
     __RESET_WATCHDOG();
     if(size==count)
     {
        count=0;
        posSize=i;
        posDir=i+1;
        size=NewCode[posSize];
        dir=(u16*)&NewCode[posDir];
        if(*dir==0xFFFE)
        {
           NewCode[posDir+2]=0xFA;
           NewCode[posDir+3]=0x7B;
        }
     }
     while(i==posSize || i==posDir || i==(posDir+1))
     {
        i++;
        size--;
     }
     testDir=((int)*dir)+(count++);
     testData=NewCode[i];
     Program_Byte(testDir,testData);
     
  }
  

  //------------------------------------------------------------------------------------
  
  asm("JMP 0xE000");//Run the new program

  
}
