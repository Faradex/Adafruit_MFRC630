#include <Wire.h>
#include <Adafruit_MFRC630.h>

/* Indicate the pin number where PDOWN is connected. */
#define PDOWN_PIN         (12)

/* Show me a mojic trick, dad! */
#define MOJIC_TRICK       (0)

/* Use the default I2C address */
Adafruit_MFRC630 rfid = Adafruit_MFRC630(PDOWN_PIN);

/* Prints out 16 bytes of hex data in table format. */
static void print_buf_hex16(uint8_t *buf, size_t len)
{
  for (uint8_t i = 0; i < len; i++)
  {
    Serial.print("0x");
    if (buf[i] < 16)
    {
      Serial.print("0");
    }
    Serial.print(buf[i], HEX);
    Serial.print(" ");
  }
  Serial.println(" ");
}

/* This functions sends a command to fill the FIFO with random numbers and */
/* then reads the entire FIFO contents in 16 byte chunks */
void fifo_read_test(void)
{
  Serial.println("Reading randomly generated numbers from FIFO buffer");

  /* Generate random numbers into the FIFO */
  rfid.writeCommand(MFRC630_CMD_READRNR);
  /* Note, this command requires a 10ms delay to fill the buffer! */
  delay(10);

  /* Dump the FIFO 16 bytes at a time to stay within 32 byte I2C limit */
  uint8_t buff[16];
  int16_t len = rfid.readFIFOLen();
  while (len)
  {
    memset(buff, 0, sizeof(buff));
    int16_t readlen = rfid.readFIFO(len > 16 ? 16 : len, buff);
    len -= readlen;
    /* Display the buffer in 16 byte chunks. */
    print_buf_hex16(buff, readlen);
  }
}

/* This function writes data to the FIFO and then tries to read it back */
void fifo_write_test(void)
{
  uint8_t buff[16] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 };

  /* Write data into the FIFO buffer */
  Serial.println("Writing 16 bytes into FIFO buffer");
  int16_t writelen = rfid.writeFIFO(sizeof(buff), buff);
  (void)writelen;

  /* Read data back and display it*/
  memset(buff, 0, sizeof(buff));
  int16_t readlen = rfid.readFIFO(sizeof(buff), buff);
  print_buf_hex16(buff, readlen);
}

/* This function clears the FIFO contents */
void fifo_clear_test(void)
{
  uint8_t buff[16] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 };

  /* Write data into the FIFO buffer */
  Serial.println("Writing 16 bytes into FIFO buffer");
  int16_t writelen = rfid.writeFIFO(sizeof(buff), buff);
  (void)writelen;
  int16_t len = rfid.readFIFOLen();
  Serial.print("FIFO len = "),
  Serial.println(len);

  /* Clear the FIFO */
  Serial.println("Clearing FIFO");
  rfid.clearFIFO();
  len = rfid.readFIFOLen();
  Serial.print("FIFO len = "),
  Serial.println(len);
}

/* This function displays the status of the MFRC's state machine */
void status_test(void)
{
  Serial.println("Reading COM Status");
  uint8_t stat = rfid.getComStatus();

  switch (stat)
  {
    case MFRC630_COMSTAT_IDLE:
      Serial.println("IDLE");
      break;
    case MFRC630_COMSTAT_TXWAIT:
      Serial.println("TX WAIT");
      break;
    case MFRC630_COMSTAT_TRANSMITTING:
      Serial.println("TRANSMITTING");
      break;
    case MFRC630_COMSTAT_RXWAIT:
      Serial.println("RX WAIT");
      break;
    case MFRC630_COMSTAT_WAITFORDATA :
      Serial.println("WAITING FOR DATA");
      break;
    case MFRC630_COMSTAT_RECEIVING:
      Serial.println("RECEIVING");
      break;
    default:
      Serial.println("UKNOWN!");
      break;
  }
}

void radio_mifare_dump_sector(uint8_t sector_num)
{
  uint8_t readbuf[16] = { 0 };
  /* Try to read four blocks inside the sector. */
  for (uint8_t b = 0; b < 4 ; b++) {
    uint8_t len = 0;
    len = rfid.mifareReadBlock(sector_num * 4 + b, readbuf);
    if (len == 0) {
      /* No data returned! */
      Serial.print("What!?! No data returned for block ");
      Serial.print(sector_num * 4 + b);
      Serial.println("!");
      #if MOJIC_TRICK
      Serial.println("(ノ ゜Д゜)ノ ︵ ┻━┻");
      #endif
      return;
    } else {
      /* Display the block contents. */
      Serial.print(b); Serial.print(": ");
      print_buf_hex16(readbuf, len);
    }
  }
}

/* Read the tag contents (assumes scan has been successfully called!). */
void radio_mifare_read(uint8_t *uid, uint8_t uidlen)
{
  if (uidlen != 4) {
    Serial.println("Not a Mifare tag!");
    #if MOJIC_TRICK
    Serial.println("(ノ ゜Д゜)ノ ︵ ┻━┻");
    #endif
    return;
  }

  /* Use the default key for fresh Mifare cards. */
  // uint8_t keyb[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

  /* Set the key. */
  rfid.mifareLoadKey(rfid.mifareKeyGlobal);

  /* Try to authenticate sectors 0..15 (all Mifare cards should have these!). */
  for (uint8_t s = 0; s < 16; s++) {
    Serial.print("Trying to authenticate sector "); Serial.print(s);
    Serial.println(" with KEYB.");

    /* Authenticate. */
    if(rfid.mifareAuth(MIFARE_CMD_AUTH_B, s*4, uid)) {
        #if MOJIC_TRICK
        Serial.println(" ᕙ(`▽´)ᕗ");
        #endif
        /* Dump the first sector. */
        radio_mifare_dump_sector(s);
        /* Deauth! */
        rfid.mifareDeauth();
    } else {
        Serial.print("AUTH_A failed with ");
        for (uint8_t k = 0; k < 6; k++){
          Serial.print("0x");
          if (HEX < 0x10) {
            Serial.print("0");
          }
          Serial.print(rfid.mifareKeyGlobal[k], HEX);
          Serial.print(" ");
        }
        Serial.println("");
        #if MOJIC_TRICK
        Serial.println("(ノ ゜Д゜)ノ ︵ ┻━┻");
        #endif
    }
  }
}

/* Configure the radio for ISO14443A-106 type tags and scan for the UID. */
void radio_iso1443A_106_scan()
{
  Serial.println("Configuring radio for ISO14443A-106 tags");
  rfid.configRadio(MFRC630_RADIOCFG_ISO1443A_106);

  /* Request a tag */
  Serial.println("Checking for ISO14443A-106 tags");
  #if MOJIC_TRICK
  Serial.println("┬─┬ ノ( ゜-゜ノ)");
  #endif
  uint16_t atqa = rfid.iso14443aRequest();

  /* Select the tag if we found something */
  if (atqa)
  {
    uint8_t uid[10] = { 0 };
    uint8_t uidlen;
    uint8_t sak;

    Serial.print("Found ISO14443A-106 tag with ATQA 0x");
    Serial.println(atqa, HEX);
    #if MOJIC_TRICK
    Serial.println("ᕙ(`▽´)ᕗ");
    #endif

    Serial.println("Selecting the tag");
    #if MOJIC_TRICK
    Serial.println("(ﾉ◕ヮ◕)ﾉ*:・ﾟ✧");
    #endif
    uidlen = rfid.iso14443aSelect(uid, &sak);
    if (uidlen) {
      Serial.print("Got a UID! --> ");
      for (uint8_t i=0; i<uidlen; i++) {
        Serial.print(uid[i], HEX);
        Serial.print(" ");
      }
      Serial.println("");
      #if MOJIC_TRICK
      Serial.println("ᕙ(`▽´)ᕗ");
      #endif
      /* Try to read and dump the card's contents. */
      radio_mifare_read(uid, uidlen);
    } else {
      Serial.println("What, no UID found! Did someone steal my tag!!");
      #if MOJIC_TRICK
      Serial.println("(ノ ゜Д゜)ノ ︵ ┻━┻");
      #endif
    }
  }
  else
  {
    Serial.println("No ISO14443A-106 tag found!");
    #if MOJIC_TRICK
    Serial.println("(ノ ゜Д゜)ノ ︵ ┻━┻");
    #endif
  }
}

void setup() {
  Serial.begin(115200);

  while (!Serial) {
    delay(1);
  }
  
  Serial.println("");
  Serial.println("-----------------------------");
  Serial.println("Adafruit MFRC630 RFID HW Test");
  Serial.println("-----------------------------");

  pinMode(LED_BUILTIN, OUTPUT);
  //pinMode(PIN_LED2, OUTPUT);

  /* Try to initialize the IC */
  if (!(rfid.begin())) {
    Serial.println("Unable to initialize the MFRC630. Check wiring?");
    while(1) {
      digitalWrite(LED_BUILTIN, HIGH);
      delay(50);
      digitalWrite(LED_BUILTIN, LOW);
      delay(50);
    }
  }

  /* FIFO tests */
  // fifo_read_test();
  // fifo_write_test();
  // fifo_clear_test();

  /* General HW tests */
  // status_test();

  /* Radio tests */
  radio_iso1443A_106_scan();
}

void loop() {
  for (int i = 0; i < 2; i++)
  {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(500);
    digitalWrite(LED_BUILTIN, LOW);
    delay(500);
  }
}
