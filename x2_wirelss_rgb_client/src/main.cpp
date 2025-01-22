/**
 * The controller acts as a client, expecting (pixel) data from the serial port.
 *
 * The SmartMatrix library offers many tools (and examples) to display graphics,
 * animations and texts.
 * Dependencies (and docs):
 * https://github.com/pixelmatix/SmartMatrix
 *
 * Fork of the library that allows control of the special 32x32 matrix
 * https://github.com/Kameeno/SmartMatrix
 */

// Pinout configuration for the PicoDriver v.5.0
#include "common/pico_driver_v5_pinout.h"

#include <Arduino.h>

#include <WiFi.h>
#include <WiFiUdp.h>


/* WiFi network name and password */
const char * ssid = "FabulousNet";
const char * pwd = "25jan2022";

#define UDP_PORT 44444

// IP address to send UDP data to.
// it can be ip address of the server or 
// a network broadcast address
// here is broadcast address
// const char * udpAddress = "192.168.1.100";
// const int udpPort = 44444;

//create UDP instance
WiFiUDP udp;


#include <SmartMatrix.h>

#define COLOR_DEPTH 24   // valid: 24, 48
#define TOTAL_WIDTH 32   // Size of the total (chained) with of the matrix/matrices
#define TOTAL_HEIGHT 32  // Size of the total (chained) height of the matrix/matrices

#define kRefreshDepth 24 // Valid: 24, 36, 48
#define kDmaBufferRows 4 // Valid: 2-4
#define kPanelType SM_PANELTYPE_HUB75_32ROW_32COL_MOD8SCAN // custom
#define kMatrixOptions (SM_HUB75_OPTIONS_NONE)
#define kbgOptions (SM_BACKGROUND_OPTIONS_NONE)

// SmartMatrix setup & buffer alloction
SMARTMATRIX_ALLOCATE_BUFFERS(matrix, TOTAL_WIDTH, TOTAL_HEIGHT, kRefreshDepth, kDmaBufferRows, kPanelType, kMatrixOptions);

// A single background layer "bg"
SMARTMATRIX_ALLOCATE_BACKGROUND_LAYER(bg, TOTAL_WIDTH, TOTAL_HEIGHT, COLOR_DEPTH, kbgOptions);


// RGB32 or RGB24 (RRRRRGGG GGGBBBBB)?
const uint8_t INCOMING_COLOR_DEPTH = 16;

const uint16_t NUM_LEDS = TOTAL_WIDTH * TOTAL_HEIGHT;
const uint16_t BUFFER_SIZE = NUM_LEDS * (INCOMING_COLOR_DEPTH / 8);
uint8_t buf[BUFFER_SIZE]; // A buffer for the incoming color data

const uint16_t CHUNK_SIZE = 1024;
const uint16_t HEADER_SIZE = 2;   // [currentChunk, totalChunks]

uint8_t receivedChunks[8] = {0}; // Track received chunks (max 32 chunks)
bool isReceivingFrame = false;

void setup() {
	// Serial.begin(921600);
	// Serial.setTimeout(1); 

	pinMode(PICO_LED_PIN, OUTPUT);
	digitalWrite(PICO_LED_PIN, 1);

	// Serial.begin(115200);

	//Connect to the WiFi network
	WiFi.begin(ssid, pwd);
	// Serial.println("");

	// Wait for connection
	while (WiFi.status() != WL_CONNECTED) {
		delay(500);
		// Serial.print(".");
	}
	udp.begin(UDP_PORT);

	// Serial.println("");
	// Serial.print("Connected to ");
	// Serial.println(ssid);
	// Serial.print("IP address: ");
	// Serial.println(WiFi.localIP());
	// Serial.print("Port: ");
	// Serial.println(UDP_PORT);
	// Serial.print("Buffer size: ");
	// Serial.println(BUFFER_SIZE);  // This will show 3072 for 32x32x3

	bg.enableColorCorrection(true);
	matrix.addLayer(&bg);
	matrix.setBrightness(255);
	matrix.begin();
}

void loop() {

	static uint32_t frame = 0;

 // Check if there's a UDP packet available
    int packetSize = udp.parsePacket();
    if (packetSize) {
        uint8_t chunkIndex, totalChunks;
        
        // Read header
        udp.read(&chunkIndex, 1);
        udp.read(&totalChunks, 1);
        // totalChunksExpected = totalChunks;

        // Read chunk data
        int dataSize = packetSize - HEADER_SIZE; // Subtract header size
        if (dataSize > 0) {
            udp.read(&buf[chunkIndex * CHUNK_SIZE], dataSize);
			// udp.flush();
            receivedChunks[chunkIndex] = 1; // Mark chunk as received
            
            //Serial.printf("Received chunk %d/%d (size: %d)\n", chunkIndex + 1, totalChunks, dataSize);

            // Check if we have all chunks
            bool complete = true;
            for (int i = 0; i < totalChunks; i++) {
                if (!receivedChunks[i]) {
                    complete = false;
                    break;
                }
            }

            if (complete) {
				// Serial.print(frame);				
                // Serial.println(" complete frame received!");
                isReceivingFrame = false;
				memset(receivedChunks, 0, sizeof(receivedChunks));
                
                rgb24 *buffer = bg.backBuffer();
				rgb24 *col;
				uint16_t idx = 0;

				if (INCOMING_COLOR_DEPTH == 24) {
					for (uint16_t i = 0; i < NUM_LEDS; i++) {
						col = &buffer[i];
						col->red   = buf[idx++];
						col->green = buf[idx++];
						col->blue  = buf[idx++];
					}
				} else if (INCOMING_COLOR_DEPTH == 16) {
					for (uint16_t i = 0; i < NUM_LEDS; i++) {
						col = &buffer[i];

						uint8_t  high  = buf[idx++];
						uint8_t  low   = buf[idx++];
						uint16_t rgb16 = ((uint16_t)high << 8) | low;

						uint8_t r5 = (rgb16 >> 11) & 0x1F;
						uint8_t g6 = (rgb16 >> 5)  & 0x3F;
						uint8_t b5 =  rgb16        & 0x1F;

						col->red   = r5 << 3;
						col->green = g6 << 2;
						col->blue  = b5 << 3;
					}
				}
				bg.drawString(0, 0, {255,0,0}, ("F:" + String(frame)).c_str());	
				bg.drawString(0, 7, {255,0,0}, ("P:" + String(packetSize)).c_str());

				static unsigned long lastFrameTime = millis();
				unsigned long currentTime = millis();
				float fps = 1000.0 / (currentTime - lastFrameTime);
				lastFrameTime = currentTime;
				
				bg.drawString(0, 14, {255,0,0}, ("FPS:" + String(fps, 1)).c_str());

				bg.swapBuffers();
				frame++;
				digitalWrite(PICO_LED_PIN, frame / 20 % 2);   // Let's animate the built-in LED as well	
            }
        }
    }
	delay(5);
}