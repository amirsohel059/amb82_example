/*
 Combined RTSP Streaming and MP4 Recording Control
 Commands:
   - "start stream": Begin RTSP streaming
   - "stop stream": Stop RTSP streaming
   - "start record": Begin MP4 recording
   - "stop record": Stop MP4 recording
*/

#include "WiFi.h"
#include "StreamIO.h"
#include "VideoStream.h"
#include "AudioStream.h"
#include "AudioEncoder.h"
#include "RTSP.h"
#include "MP4Recording.h"

#define CHANNEL 0

VideoSetting configV(CHANNEL);
AudioSetting configA(0);
Audio audio;
AAC aac;
RTSP rtsp;
MP4Recording mp4;

// Create separate StreamIO objects for different functions
StreamIO audioStreamer(1, 1);      // Audio -> AAC
StreamIO streamMixer(2, 1);        // Video + AAC -> RTSP (for streaming)
StreamIO recordMixer(2, 1);        // Video + AAC -> MP4 (for recording)

char ssid[] = "SSID";      // your network SSID
char pass[] = "password";          // your network password

int status = WL_IDLE_STATUS;

// State tracking variables
bool streamingActive = false;
bool recordingActive = false;
bool cameraActive = false;
bool audioActive = false;

void setup() {
    Serial.begin(115200);
    while (!Serial); // Wait for serial connection

    // Connect to WiFi
    while (status != WL_CONNECTED) {
        Serial.print("Connecting to: ");
        Serial.println(ssid);
        status = WiFi.begin(ssid, pass);
        delay(2000);
    }
    Serial.println("WiFi connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    // Initialize hardware components
    Camera.configVideoChannel(CHANNEL, configV);
    Camera.videoInit();
    
    audio.configAudio(configA);
    audio.begin();
    
    aac.configAudio(configA);
    aac.begin();
    
    rtsp.configVideo(configV);
    rtsp.configAudio(configA, CODEC_AAC);
    rtsp.begin();
    
    // Configure MP4 recording settings
    mp4.configVideo(configV);
    mp4.configAudio(configA, CODEC_AAC);
    mp4.setRecordingDuration(300); // 5 minutes max recording
    mp4.setRecordingFileCount(100);
    mp4.setRecordingFileName("Recording");

    // Set up audio pipeline
    audioStreamer.registerInput(audio);
    audioStreamer.registerOutput(aac);

    // Set up streaming pipeline
    streamMixer.registerInput1(Camera.getStream(CHANNEL));
    streamMixer.registerInput2(aac);
    streamMixer.registerOutput(rtsp);

    // Set up recording pipeline
    recordMixer.registerInput1(Camera.getStream(CHANNEL));
    recordMixer.registerInput2(aac);
    recordMixer.registerOutput(mp4);

    Serial.println("\nCommand Menu:");
    Serial.println("1. 'start stream' - Begin RTSP streaming");
    Serial.println("2. 'stop stream'  - Stop RTSP streaming");
    Serial.println("3. 'start record' - Begin MP4 recording");
    Serial.println("4. 'stop record'  - Stop MP4 recording");
    Serial.println("5. 'info'         - Show current status");
}

void loop() {
    if (Serial.available() > 0) {
        String command = Serial.readStringUntil('\n');
        command.trim();
        command.toLowerCase();

        if (command == "start stream") {
            startStreaming();
        } 
        else if (command == "stop stream") {
            stopStreaming();
        } 
        else if (command == "start record") {
            startRecording();
        } 
        else if (command == "stop record") {
            stopRecording();
        } 
        else if (command == "info") {
            printInfo();
        }
        else {
            Serial.println("Invalid command. Valid commands:");
            Serial.println("start stream, stop stream, start record, stop record, info");
        }
    }

    // Add small delay to prevent serial buffer overflow
    delay(50);
}

void startStreaming() {
    if (streamingActive) {
        Serial.println("Streaming is already active");
        return;
    }
    
    Serial.println("Starting streaming...");
    
    // Activate camera if not already active
    if (!cameraActive) {
        Camera.channelBegin(CHANNEL);
        cameraActive = true;
        Serial.println("Camera activated");
    }
    
    // Activate audio pipeline if not active
    if (!audioActive) {
        if (audioStreamer.begin() != 0) {
            Serial.println("Failed to start audio pipeline!");
            return;
        }
        audioActive = true;
        Serial.println("Audio pipeline started");
    }
    
    // Start streaming pipeline
    if (streamMixer.begin() != 0) {
        Serial.println("Failed to start streaming pipeline!");
        return;
    }
    
    streamingActive = true;
    Serial.println("Streaming started successfully!");
    Serial.print("RTSP URL: rtsp://");
    Serial.print(WiFi.localIP());
    Serial.print(":");
    rtsp.printInfo();
}

void stopStreaming() {
    if (!streamingActive) {
        Serial.println("Streaming is not active");
        return;
    }
    
    Serial.println("Stopping streaming...");
    streamMixer.end();
    streamingActive = false;
    Serial.println("Streaming stopped");
    
    // Deactivate resources if not used by recording
    shutdownIdleResources();
}

void startRecording() {
    if (recordingActive) {
        Serial.println("Recording is already active");
        return;
    }
    
    Serial.println("Starting recording...");
    
    // Activate camera if not already active
    if (!cameraActive) {
        Camera.channelBegin(CHANNEL);
        cameraActive = true;
        Serial.println("Camera activated");
    }
    
    // Activate audio pipeline if not active
    if (!audioActive) {
        if (audioStreamer.begin() != 0) {
            Serial.println("Failed to start audio pipeline!");
            return;
        }
        audioActive = true;
        Serial.println("Audio pipeline started");
    }

    mp4.begin();
    
    // Start recording pipeline
    if (recordMixer.begin() != 0) {
        Serial.println("Failed to start recording pipeline!");
        mp4.end();
        return;
    }
    
    recordingActive = true;
    Serial.println("Recording started successfully!");
    Serial.println("Saving to: " + String(mp4.getRecordingFileName()) + ".mp4");
}

void stopRecording() {
    if (!recordingActive) {
        Serial.println("Recording is not active");
        return;
    }
    
    Serial.println("Stopping recording...");
    recordMixer.end();
    mp4.end();
    recordingActive = false;
    Serial.println("Recording stopped");
    
    // Deactivate resources if not used by streaming
    shutdownIdleResources();
}

void shutdownIdleResources() {
    // Stop audio pipeline if neither streaming nor recording need it
    if (!streamingActive && !recordingActive && audioActive) {
        audioStreamer.end();
        audioActive = false;
        Serial.println("Audio pipeline stopped");
    }
    
    // Stop camera if no activities need it
    if (!streamingActive && !recordingActive && cameraActive) {
        Camera.channelEnd(CHANNEL);
        cameraActive = false;
        Serial.println("Camera deactivated");
    }
}

void printInfo() {
    Serial.println("\n===== System Status =====");
    Serial.println("Streaming: " + String(streamingActive ? "ACTIVE" : "INACTIVE"));
    Serial.println("Recording: " + String(recordingActive ? "ACTIVE" : "INACTIVE"));
    
    if (streamingActive) {
        Serial.print("RTSP URL: rtsp://");
        Serial.print(WiFi.localIP());
        Serial.print(":");
        rtsp.printInfo();
    }
    
    if (recordingActive) {
        Serial.println("Recording in progress: " + String(mp4.getRecordingFileName()) + ".mp4");
    }
    
    Serial.println("Camera: " + String(cameraActive ? "ACTIVE" : "INACTIVE"));
    Serial.println("Audio Pipeline: " + String(audioActive ? "ACTIVE" : "INACTIVE"));
    Serial.println("========================\n");
}