# SD Card File Tracker for AMB82 Mini IoT

This project provides a file tracking system for the AMB82 Mini IoT board, allowing you to manage files on an SD card and maintain a simple database of file upload statuses.

## Features

- Track files on SD card with upload status (uploaded/pending)
- Add new files to the tracking system
- Mark files as uploaded
- View pending, uploaded, or all tracked files
- Persistent storage of tracking information
- Simple serial command interface

## Hardware Requirements

- AMB82 Mini IoT board
- SD card (properly formatted)
- Serial terminal for command input

## Installation

1. Copy the code to your Arduino IDE
2. Ensure you have the AMB82 board support installed
3. Connect your AMB82 Mini IoT board
4. Insert a properly formatted SD card
5. Upload the sketch

## Usage

After uploading the sketch, open the Serial Monitor (baud rate 115200) to interact with the file tracker. Available commands:

- `create <filename>` - Add a new file to track
- `upload <filename>` - Mark a file as uploaded
- `showpending` - Show all files pending upload
- `showuploaded` - Show all uploaded files
- `showall` - Show all tracked files with their status


## Applications

This code can be used as a foundation for:

1. IoT data logging systems
2. File synchronization tools
3. Remote data collection systems
4. Any application needing to track file states
5. Simple database functionality for small datasets

## Limitations

- Maximum of 50 tracked files (can be adjusted by changing MAX_FILES)
- Filenames limited to 63 characters
- Simple text-based storage format
