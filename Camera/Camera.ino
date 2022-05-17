#include "Arduino.h"
#include "FS.h"                // SD Card ESP32
#include "SD_MMC.h"            // SD Card ESP32
#include "soc/soc.h"           // Disable brownour problems
#include "soc/rtc_cntl_reg.h"  // Disable brownour problems
#include "driver/rtc_io.h"
#include <painlessMesh.h>
#include <ArduinoJson.h>
//#include <esp_now.h>
#include <WiFi.h>
#define ONBOADLED 4
#include "esp_camera.h"

#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
// ===================
// Select camera model
// ===================
//#define CAMERA_MODEL_WROVER_KIT // Has PSRAM
//#define CAMERA_MODEL_ESP_EYE // Has PSRAM
//#define CAMERA_MODEL_ESP32S3_EYE // Has PSRAM
//#define CAMERA_MODEL_M5STACK_PSRAM // Has PSRAM
//#define CAMERA_MODEL_M5STACK_V2_PSRAM // M5Camera version B Has PSRAM
//#define CAMERA_MODEL_M5STACK_WIDE // Has PSRAM
//#define CAMERA_MODEL_M5STACK_ESP32CAM // No PSRAM
//#define CAMERA_MODEL_M5STACK_UNITCAM // No PSRAM
#define CAMERA_MODEL_AI_THINKER // Has PSRAM
//#define CAMERA_MODEL_TTGO_T_JOURNAL // No PSRAM
// ** Espressif Internal Boards **
//#define CAMERA_MODEL_ESP32_CAM_BOARD
//#define CAMERA_MODEL_ESP32S2_CAM_BOARD
//#define CAMERA_MODEL_ESP32S3_CAM_LCD

#include "camera_pins.h"

#define fileDatainMessage 240.0

#define shutterbutton 0

// for photo name
int pictureNumber = 1;
byte takeNextPhotoFlag = 0;

// for photo transmit
int currentTransmitCurrentPosition = 0;
int currentTransmitTotalPackages = 0;
byte sendNextPackageFlag = 0;
String fileName = "/pic.jpg";

//Define functions for use in platformIO
void initCamera();
void initSD();
void takePhoto();
void startTransmit();
void sendData();
void maakfoto();
void sendNextPackage();
void startTransmit();
void sendNextPackage();
void blinkIt();
void sendMessage();

Scheduler userScheduler; // to control your personal task
Task taskSendMessage( 30000 , TASK_FOREVER, &maakfoto );


/*  ============================================================================
                       Setup for ESP-NOW Mesh Network 
  ============================================================================= */
#define   MESH_PREFIX     "TopGeheimNetwerk"
#define   MESH_PASSWORD   "HeelMoeilijk1337"
#define   MESH_PORT       1337
painlessMesh  mesh;


void sendMessage(uint8_t * dataArray, uint8_t dataArrayLength) {
Serial.println("Starting send message functie");
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


void maakfoto()
{
Serial.println("Starting Maak Foto Functie");
  if (!currentTransmitTotalPackages && !sendNextPackageFlag )
    takeNextPhotoFlag = 1;

  // if the sendNextPackageFlag is set
  if (sendNextPackageFlag)
    sendNextPackage();
Serial.println("Maak de foto");
  // if takeNextPhotoFlag is set
  if (takeNextPhotoFlag)
    takePhoto();

}

/* ***************************************************************** */
/* TAKE PHOTO                                                        */
/* ***************************************************************** */
void takePhoto()
{
 Serial.println("Starting Take Photo Function"); 
  takeNextPhotoFlag = 0;
  Serial.println("Zet de LED aan"); 
  digitalWrite(4, HIGH);
  Serial.println("Wacht 50ms"); 
  delay(50);
  Serial.println("Initieer camera buffer"); 
  camera_fb_t * fb = NULL;

  // Take Picture with Camera
  Serial.println("Vul de  camera buffer"); 
  fb = esp_camera_fb_get();
  Serial.println("Controleer camera buffer"); 
  if (!fb) {
    Serial.println("Camera capture failed");
    return;
  }
  Serial.println("Zet de LED uit"); 
  digitalWrite(4, LOW);
  // Path where new picture will be saved in SD Card
  String path = "/picture" + String(pictureNumber) + ".jpg";

  fs::FS &fs = SD_MMC;
  Serial.printf("Picture file name: %s\n", path.c_str());

  fs.remove(path.c_str());

  File file = fs.open(path.c_str(), FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file in writing mode");
  }
  else {
    file.write(fb->buf, fb->len); // payload (image), payload length
    Serial.printf("Saved file to path: %s\n", path.c_str());
  }
  file.close();

  esp_camera_fb_return(fb);

  fileName = path;
  
  startTransmit();

  pictureNumber++;

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

  if (!psramFound()) {
    config.frame_size = FRAMESIZE_QVGA; //FRAMESIZE_UXGA; // FRAMESIZE_ + QVGA|CIF|VGA|SVGA|XGA|SXGA|UXGA //FRAMESIZE_QVGA
    config.jpeg_quality = 2;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  // Init Camera
    esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }
  
}

/* ***************************************************************** */
/* START TRASMIT                                                     */
/* ***************************************************************** */

void startTransmit()
{
  Serial.println("Starting transmit");
  fs::FS &fs = SD_MMC;
  File file = fs.open(fileName.c_str(), FILE_READ);
  if (!file) {
    Serial.println("Failed to open file in writing mode");
    return;
  }
  Serial.println(file.size());
  int fileSize = file.size();
  file.close();
  currentTransmitCurrentPosition = 0;
  currentTransmitTotalPackages = ceil(fileSize / fileDatainMessage);
  Serial.println(currentTransmitTotalPackages);
  uint8_t message[] = {0x01, currentTransmitTotalPackages >> 8, (byte) currentTransmitTotalPackages};
  sendMessage(message, sizeof(message));
}

/* ***************************************************************** */
/* SEND NEXT PACKAGE                                                 */
/* ***************************************************************** */

void sendNextPackage()
{
  // claer the flag
  sendNextPackageFlag = 0;

  // if got to AFTER the last package
  if (currentTransmitCurrentPosition == currentTransmitTotalPackages)
  {
    currentTransmitCurrentPosition = 0;
    currentTransmitTotalPackages = 0;
    Serial.println("Done submiting files");
    //takeNextPhotoFlag = 1;
    return;
  } //end if

  //first read the data.
  fs::FS &fs = SD_MMC;
  File file = fs.open(fileName.c_str(), FILE_READ);
  if (!file) {
    Serial.println("Failed to open file in writing mode");
    return;
  }

  // set array size.
  int fileDataSize = fileDatainMessage;
  // if its the last package - we adjust the size !!!
  if (currentTransmitCurrentPosition == currentTransmitTotalPackages - 1)
  {
    Serial.println("*************************");
    Serial.println(file.size());
    Serial.println(currentTransmitTotalPackages - 1);
    Serial.println((currentTransmitTotalPackages - 1)*fileDatainMessage);
    fileDataSize = file.size() - ((currentTransmitTotalPackages - 1) * fileDatainMessage);
  }

  // define message array
  uint8_t messageArray[fileDataSize + 3];
  messageArray[0] = 0x02;


  file.seek(currentTransmitCurrentPosition * fileDatainMessage);
  currentTransmitCurrentPosition++; // set to current (after seek!!!)

  messageArray[1] = currentTransmitCurrentPosition >> 8;
  messageArray[2] = (byte) currentTransmitCurrentPosition;
  for (int i = 0; i < fileDataSize; i++)
  {
    if (file.available())
    {
      messageArray[3 + i] = file.read();
    } //end if available
    else
    {
      Serial.println("END !!!");
      break;
    }
  } //end for

  sendMessage(messageArray, sizeof(messageArray));
  file.close();

}

void blinkIt(int delayTime, int times)
{
  for (int i = 0; i < times; i++)
  {
    digitalWrite(ONBOADLED, HIGH);
    delay(delayTime);
    digitalWrite(ONBOADLED, LOW);
    delay(delayTime);
  }
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