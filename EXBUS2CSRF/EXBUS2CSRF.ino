

#include "JetiExBusProtocol.h"
#include "ExbusSensor.h"
#include <stdint.h> 
#include "xfire.h"
#include <limits.h>
#include "RTClib.h"
#include <Time.h>
#include <inttypes.h>
#include "TeensyTimerTool.h"




using namespace TeensyTimerTool;
PeriodicTimer csrfTimer;


JetiExBusProtocol exBus;
ExbusSensor        exbusSensor; 


enum
{
	ID_VOLTAGE = 1,
	ID_ALTITUDE,
	ID_TEMP,
	ID_CLIMB,
	ID_FUEL,
	ID_RPM,
	ID_GPSLON,
	ID_GPSLAT,
	ID_DATE,
	ID_TIME,
	ID_VAL11, ID_VAL12, ID_VAL13, ID_VAL14, ID_VAL15, ID_VAL16, ID_VAL17, ID_VAL18,
};


//csrf vars



// sensor definition (max. 31 for DC/DS-16)
// name plus unit must be < 20 characters
// precision = 0 --> 0, precision = 1 --> 0.0, precision = 2 --> 0.00

JETISENSOR_CONST sensors[] PROGMEM =
{
	// id             name          unit         data type             precision 
	{ ID_VOLTAGE,    "Voltage",    "V",         JetiSensor::TYPE_14b, 1 },
	{ ID_ALTITUDE,   "Altitude",   "m",         JetiSensor::TYPE_14b, 0 },
	{ ID_TEMP,       "Temp",       "\xB0\x43",  JetiSensor::TYPE_14b, 0 }, // �C
	{ ID_CLIMB,      "Climb",      "m/s",       JetiSensor::TYPE_14b, 2 },
	{ ID_FUEL,       "Fuel",       "%",         JetiSensor::TYPE_14b, 0 },
	{ ID_RPM,        "RPM x 1000", "/min",      JetiSensor::TYPE_14b, 1 },

	{ ID_GPSLON,     "Longitude",  " ",         JetiSensor::TYPE_GPS, 0 },
	{ ID_GPSLAT,     "Latitude",   " ",         JetiSensor::TYPE_GPS, 0 },
	{ ID_DATE,       "Date",       " ",         JetiSensor::TYPE_DT,  0 },
	{ ID_TIME,       "Time",       " ",         JetiSensor::TYPE_DT,  0 },

	{ ID_VAL11,      "V11",        "U11",       JetiSensor::TYPE_14b, 0 },
	{ ID_VAL12,      "V12",        "U12",       JetiSensor::TYPE_14b, 0 },
	{ ID_VAL13,      "V13",        "U13",       JetiSensor::TYPE_14b, 0 },
	{ ID_VAL14,      "V14",        "U14",       JetiSensor::TYPE_14b, 0 },
	{ ID_VAL15,      "V15",        "U15",       JetiSensor::TYPE_14b, 0 },
	{ ID_VAL16,      "V16",        "U16",       JetiSensor::TYPE_14b, 0 },
	{ ID_VAL17,      "V17",        "U17",       JetiSensor::TYPE_14b, 0 },
	{ ID_VAL18,      "V18",        "U18",       JetiSensor::TYPE_14b, 0 },
	{ 0 } // end of array
};



void setup()
{
  Serial.begin(9600);

  startCrossfire();
  csrfTimer.begin(setupPulsesCrossfire, (REFRESH_INTERVAL*1000)); 
  
	while (!Serial)
		;

	exBus.SetDeviceId(0x76, 0x32); // 0x3276
	exBus.Start("EX Bus", sensors, 2 ); // com port: 1..3 for Teeny, 0 or 1 for AtMega328PB UART0/UART1, others: not used 

	exBus.SetJetiboxText(0, "EXBUS2CSRF");
	exBus.SetJetiboxText(1, "TRANSCODER");

  

}

void loop()
{




    if ( exBus.HasNewChannelData() )
     {
     int i; 
     //for (i = 0; i < exBus.GetNumChannels(); i++){
     for (i = 0; i < 16; i++){
      Serial.println(exBus.GetChannel(i));
     }
  }


  
	// get JETI buttons
	uint8_t bt = exBus.GetJetiboxKey();
	if( bt )
	{
		Serial.print( "bt - "); Serial.println(bt);
    }

	if (exBus.IsBusReleased())
	{
		// exBus is currently sending an ex packet
		// do time consuming stuff here (20-30ms)
		delay( 30 );
	}
  
 


  exBus.SetSensorValue(ID_VOLTAGE, exbusSensor.GetVoltage());
	exBus.SetSensorValue(ID_ALTITUDE, exbusSensor.GetAltitude());
	exBus.SetSensorValue(ID_TEMP, exbusSensor.GetTemp());
	exBus.SetSensorValue(ID_CLIMB, exbusSensor.GetClimb());
	exBus.SetSensorValue(ID_FUEL, exbusSensor.GetFuel());
	exBus.SetSensorValue(ID_RPM, exbusSensor.GetRpm());

	exBus.SetSensorValueGPS(ID_GPSLON, true, 11.55616f); // E 11� 33' 22.176"
	exBus.SetSensorValueGPS(ID_GPSLAT, false, 48.24570f); // N 48� 14' 44.520"
	exBus.SetSensorValueDate(ID_DATE, 29, 12, 2015);
	exBus.SetSensorValueTime(ID_TIME, 19, 16, 37);

	exBus.SetSensorValue(ID_VAL11, exbusSensor.GetVal(4));
	exBus.SetSensorValue(ID_VAL12, exbusSensor.GetVal(5));
	exBus.SetSensorValue(ID_VAL13, exbusSensor.GetVal(6));
	exBus.SetSensorValue(ID_VAL14, exbusSensor.GetVal(7));
	exBus.SetSensorValue(ID_VAL15, exbusSensor.GetVal(8));
	exBus.SetSensorValue(ID_VAL16, exbusSensor.GetVal(9));
	exBus.SetSensorValue(ID_VAL17, exbusSensor.GetVal(10));
	exBus.SetSensorValue(ID_VAL18, exbusSensor.GetVal(11));


	// run protocol state machine
  //noInterrupts(); 
   
	exBus.DoJetiExBus();
// interrupts();
  
}


/* crossfire stuff */

uint32_t lastRefreshTime;
uint8_t frame[CROSSFIRE_FRAME_MAXLEN];


uint8_t startCrossfire(){
    //CROSSFIRE_SERIAL.begin(CROSSFIRE_BAUD_RATE, SERIAL_8N1_TXINV | SERIAL_8N1_RXINV);
     CROSSFIRE_SERIAL.begin(CROSSFIRE_BAUD_RATE,SERIAL_8N1_RXINV_TXINV);

    return true;
}


uint8_t runCrossfire(){

         setupPulsesCrossfire();

      return 0;
}




void setupPulsesCrossfire()
{
       // uint8_t crossfire[CROSSFIRE_FRAME_MAXLEN];
       // memset(crossfire, 0, sizeof(crossfire));
        //uint8_t frame[CROSSFIRE_FRAME_MAXLEN];
        memset(frame, 0, sizeof(frame));

        uint8_t length = createCrossfireChannelsFrame(frame);
        
        CROSSFIRE_SERIAL.write(frame, length);
   
}

//uint8_t createCrossfireChannelsFrame(uint8_t * frame, int16_t * pulses)
uint8_t createCrossfireChannelsFrame(uint8_t * frame)
{
  uint8_t * buf = frame;
  *buf++ = MODULE_ADDRESS;
  *buf++ = 24; // 1(ID) + 22 + 1(CRC)
  uint8_t * crc_start = buf;
  *buf++ = CHANNELS_ID;
  uint32_t bits = 0;
  uint8_t bitsavailable = 0;


  for (int i=0; i<CROSSFIRE_CHANNELS_COUNT; i++) {

    
    uint32_t val = map(exBus.GetChannel(i),8000,16000,173,1815);

    
    bits |= val << bitsavailable;
    bitsavailable += CROSSFIRE_CH_BITS;
    while (bitsavailable >= 8) {
      *buf++ = bits;
      bits >>= 8;
      bitsavailable -= 8;
    }
  }
  *buf++ = crc8(crc_start, 23);
  return buf - frame;
}


// CRC8 implementation with polynom = x^8+x^7+x^6+x^4+x^2+1 (0xD5)
unsigned char crc8tab[256] = {
  0x00, 0xD5, 0x7F, 0xAA, 0xFE, 0x2B, 0x81, 0x54,
  0x29, 0xFC, 0x56, 0x83, 0xD7, 0x02, 0xA8, 0x7D,
  0x52, 0x87, 0x2D, 0xF8, 0xAC, 0x79, 0xD3, 0x06,
  0x7B, 0xAE, 0x04, 0xD1, 0x85, 0x50, 0xFA, 0x2F,
  0xA4, 0x71, 0xDB, 0x0E, 0x5A, 0x8F, 0x25, 0xF0,
  0x8D, 0x58, 0xF2, 0x27, 0x73, 0xA6, 0x0C, 0xD9,
  0xF6, 0x23, 0x89, 0x5C, 0x08, 0xDD, 0x77, 0xA2,
  0xDF, 0x0A, 0xA0, 0x75, 0x21, 0xF4, 0x5E, 0x8B,
  0x9D, 0x48, 0xE2, 0x37, 0x63, 0xB6, 0x1C, 0xC9,
  0xB4, 0x61, 0xCB, 0x1E, 0x4A, 0x9F, 0x35, 0xE0,
  0xCF, 0x1A, 0xB0, 0x65, 0x31, 0xE4, 0x4E, 0x9B,
  0xE6, 0x33, 0x99, 0x4C, 0x18, 0xCD, 0x67, 0xB2,
  0x39, 0xEC, 0x46, 0x93, 0xC7, 0x12, 0xB8, 0x6D,
  0x10, 0xC5, 0x6F, 0xBA, 0xEE, 0x3B, 0x91, 0x44,
  0x6B, 0xBE, 0x14, 0xC1, 0x95, 0x40, 0xEA, 0x3F,
  0x42, 0x97, 0x3D, 0xE8, 0xBC, 0x69, 0xC3, 0x16,
  0xEF, 0x3A, 0x90, 0x45, 0x11, 0xC4, 0x6E, 0xBB,
  0xC6, 0x13, 0xB9, 0x6C, 0x38, 0xED, 0x47, 0x92,
  0xBD, 0x68, 0xC2, 0x17, 0x43, 0x96, 0x3C, 0xE9,
  0x94, 0x41, 0xEB, 0x3E, 0x6A, 0xBF, 0x15, 0xC0,
  0x4B, 0x9E, 0x34, 0xE1, 0xB5, 0x60, 0xCA, 0x1F,
  0x62, 0xB7, 0x1D, 0xC8, 0x9C, 0x49, 0xE3, 0x36,
  0x19, 0xCC, 0x66, 0xB3, 0xE7, 0x32, 0x98, 0x4D,
  0x30, 0xE5, 0x4F, 0x9A, 0xCE, 0x1B, 0xB1, 0x64,
  0x72, 0xA7, 0x0D, 0xD8, 0x8C, 0x59, 0xF3, 0x26,
  0x5B, 0x8E, 0x24, 0xF1, 0xA5, 0x70, 0xDA, 0x0F,
  0x20, 0xF5, 0x5F, 0x8A, 0xDE, 0x0B, 0xA1, 0x74,
  0x09, 0xDC, 0x76, 0xA3, 0xF7, 0x22, 0x88, 0x5D,
  0xD6, 0x03, 0xA9, 0x7C, 0x28, 0xFD, 0x57, 0x82,
  0xFF, 0x2A, 0x80, 0x55, 0x01, 0xD4, 0x7E, 0xAB,
  0x84, 0x51, 0xFB, 0x2E, 0x7A, 0xAF, 0x05, 0xD0,
  0xAD, 0x78, 0xD2, 0x07, 0x53, 0x86, 0x2C, 0xF9
};

uint8_t crc8(const uint8_t * ptr, uint32_t len)
{
  uint8_t crc = 0;
  for ( uint32_t i=0 ; i<len ; i += 1 )
  {
    crc = crc8tab[crc ^ *ptr++] ;
  }
  return crc;
}
