#include "Arduino.h"
#include "FS.h"                // SD Card ESP32
#include "SD_MMC.h"            // SD Card ESP32
#include "soc/soc.h"           // Disable brownour problems
#include "soc/rtc_cntl_reg.h"  // Disable brownour problems
#include "driver/rtc_io.h"
#include <painlessMesh.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#define ONBOADLED 4
#include "esp_camera.h"

#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
// ===================
// Select camera model
// ===================

#define CAMERA_MODEL_AI_THINKER // Has PSRAM

#include "camera_pins.h"

#define shutterbutton 0

//Define functions for use in platformIO
void initCamera();
void initSD();
void takePhoto();
void startTransmit();
void sendData();
void maakfoto();
void sendNextPackage();
void blinkIt();
void sendMessage();

// for photo name
int pictureNumber = 1;
byte takeNextPhotoFlag = 0;

// for photo transmit
int currentTransmitCurrentPosition = 0;
int currentTransmitTotalPackages = 0;
byte sendNextPackageFlag = 0;
String fileName = "/pic.jpg";

Scheduler userScheduler; // to control your personal task
Task taskSendMessage( 60000 , TASK_FOREVER, &startTransmit );

/*  ============================================================================
                       Setup for ESP-NOW Mesh Network 
  ============================================================================= */
#define   MESH_PREFIX     "TopGeheimNetwerk"
#define   MESH_PASSWORD   "HeelMoeilijk1337"
#define   MESH_PORT       1337
painlessMesh  mesh;


void sendMessage(uint8_t * dataArray, uint8_t dataArrayLength) {
Serial.println("Starting send message functie");
Serial.println(dataArrayLength);

  unsigned char * msgtemp = dataArray;
  const char * msg = (const char *)msgtemp;
  Serial.println("Starting Versturen naar master node");
  mesh.sendSingle(1571683145, msg);//Send data to MasterNode.
  Serial.println(msg);

}

// Needed for painless library
void receivedCallback( uint32_t from, String &msg ) {
  Serial.printf("startHere: Received from %u msg=%s\n", from, msg.c_str());
}

void newConnectionCallback(uint32_t nodeId) {
    Serial.printf("--> startHere: New Connection, nodeId = %u\n", nodeId);
}

void changedConnectionCallback() {
  Serial.printf("Changed connections\n");
}

void nodeTimeAdjustedCallback(int32_t offset) {
    Serial.printf("Adjusted time %u. Offset = %d\n", mesh.getNodeTime(),offset);
}


/* ***************************************************************** */
/* INIT SD                                                           */
/* ***************************************************************** */
void initSD()
{
  Serial.println("Starting SD Card");
  if (!SD_MMC.begin("/sdcard", true)) {
    Serial.println("SD Card Mount Failed");
    return;
  }

  uint8_t cardType = SD_MMC.cardType();
  if (cardType == CARD_NONE) {
    Serial.println("No SD Card attached");
    return;
  }
}


/* ***************************************************************** */
/* INIT CAMERA                                                       */
/* ***************************************************************** */
void initCamera()
{
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  Serial.println("psramFound() = " + String(psramFound()));

    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;

  // Init Camera
    esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }
  
}


void stuffPixels(const uint8_t * pixelBuff, const uint16_t &bufStartIndex, const uint16_t &txStartIndex, const uint16_t &len){
  uint16_t txi = txStartIndex;
  //Serial.println(pixelBuff);
  Serial.println(bufStartIndex);
  Serial.println(txStartIndex);
  Serial.println(len);
  for (uint16_t i=bufStartIndex; i<(bufStartIndex + len); i++)  {
  
  Serial.println("Starting Versturen naar master node");
  Serial.println(txi);
unsigned char * msg;  
  msg.getBytes(pixelBuff, i)
//unsigned char * msgtemp;
  //msgtemp[txi] = pixelBuff[i];
  //const char * msg = (const char *)msgtemp;
  mesh.sendSingle(1571683145, msg);//Send data to MasterNode.
  //Serial.println(msg);
    txi++;
  }
}

/* ***************************************************************** */
/* START TRASMIT                                                     */
/* ***************************************************************** */
struct img_meta_data{
  uint16_t counter;
  uint16_t imSize;
  uint16_t numLoops;
  uint16_t sizeLastLoop;
} ImgMetaData;
const uint16_t MAX_PACKET_SIZE = 250;
const uint16_t PIXELS_PER_PACKET = MAX_PACKET_SIZE - sizeof(ImgMetaData);

void startTransmit()
{
 Serial.println("Startup Camera"); 
  void initCamera();

 Serial.println("Starting Take Photo Function"); 
  Serial.println("Zet de LED aan"); 
  digitalWrite(4, HIGH);
  Serial.println("Wacht 50ms"); 
  delay(50);
  Serial.println("Initieer camera buffer"); 
  camera_fb_t * fb = NULL;
  Serial.println("Vul de  camera buffer"); 
  fb = esp_camera_fb_get();
  Serial.println("Controleer camera buffer"); 
  if (!fb) {
    Serial.println("Camera capture failed");
    return;
  }
  Serial.println("Zet de LED uit"); 
  digitalWrite(4, LOW);

uint16_t startIndex = 0;
  ImgMetaData.imSize       = fb->len;                             //sizeof(myFb);
  ImgMetaData.numLoops     = (fb->len / PIXELS_PER_PACKET) + 1;   //(sizeof(myFb)/PIXELS_PER_PACKET) + 1; 
  ImgMetaData.sizeLastLoop = fb->len % PIXELS_PER_PACKET;         //(sizeof(myFb)%PIXELS_PER_PACKET);

Serial.println("Plaatje grootte: ");
Serial.println(ImgMetaData.imSize );
Serial.println("Aantal te versturen pakketten: ");
Serial.println(ImgMetaData.numLoops );
Serial.println("SizeLastLoop: ");
Serial.println(ImgMetaData.sizeLastLoop );

for(ImgMetaData.counter=1; ImgMetaData.counter<=ImgMetaData.numLoops; ImgMetaData.counter++){
    
    stuffPixels(fb->buf, startIndex, sizeof(ImgMetaData), PIXELS_PER_PACKET);
  
    startIndex += PIXELS_PER_PACKET;    
    delay(100);
   
  }
    esp_camera_fb_return(fb);   //clear camera memory

}


void setup() {
  // NEEDED ????
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable brownout detector
  // start serial
  Serial.begin(115200);
  Serial.println("CAMERA MASTER STARTED");
  // init onboad led
  pinMode(ONBOADLED, OUTPUT);
  digitalWrite(ONBOADLED, LOW);
Serial.println("Start Setup Mesh Network");
  // Setup Mesh Network
  mesh.setDebugMsgTypes( DEBUG | ERROR | STARTUP );  // set before init() so that you can see startup messages

  mesh.init( MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT );
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);
  mesh.setContainsRoot(true);

Serial.println("Finished Seting up Mesh Network");

  // init camera
  initCamera();
  Serial.println("Camera Init Finished");
  // init sd
  initSD();
Serial.println("SD Card  Init Finished");


  userScheduler.addTask( taskSendMessage );
Serial.println("Scheduler Task toegevoegd");
Serial.println("Scheduler Task activeren");
  taskSendMessage.enable();
  Serial.println("Scheduler Task geactiveerd");

}

void loop() {
 mesh.update(); // Start Mesh Network

}