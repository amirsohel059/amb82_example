#include <AmebaFatFS.h>

// Global filesystem instance
AmebaFatFS fs;

/**
 * FileTracker class manages tracking of files and their upload status
 */
class FileTracker {
private:
    const char* TRACKER_FILE = "file_track.txt";  // File to store tracking data
    
    // Structure to hold file information
    struct FileEntry {
        char filename[64];  // Filename (max 63 chars + null terminator)
        bool uploaded;      // Upload status flag
    };
    
    static const int MAX_FILES = 50;  // Maximum number of files to track
    FileEntry files[MAX_FILES];       // Array to store file entries
    int fileCount = 0;                // Current number of tracked files
    
public:
    /**
     * Initialize the file tracker
     * @return true if initialization succeeded, false otherwise
     */
    bool begin() {
        if (!fs.begin()) {
            Serial.println("Failed to initialize filesystem");
            return false;
        }
        loadTrackerFile();  // Load existing tracking data
        return true;
    }
    
    /**
     * Add a new file to track
     * @param filename Name of the file to track
     * @return true if file was added successfully, false otherwise
     */
    bool addFile(const char* filename) {
        if (fileCount >= MAX_FILES) {
            Serial.println("Tracker full");
            return false;
        }
        
        // Check for duplicates
        for (int i = 0; i < fileCount; i++) {
            if (strcmp(files[i].filename, filename) == 0) {
                return false;
            }
        }
        
        // Add new file entry
        strncpy(files[fileCount].filename, filename, sizeof(files[0].filename)-1);
        files[fileCount].uploaded = false;
        fileCount++;
        
        return saveTrackerFile();  // Save updated tracking data
    }
    
    /**
     * Mark a file as uploaded
     * @param filename Name of the file to mark
     * @return true if file was found and marked, false otherwise
     */
    bool markAsUploaded(const char* filename) {
        for (int i = 0; i < fileCount; i++) {
            if (strcmp(files[i].filename, filename) == 0 && !files[i].uploaded) {
                files[i].uploaded = true;
                return saveTrackerFile();  // Save updated status
            }
        }
        return false;
    }
    
    /**
     * Display all files pending upload
     */
    void showPending() {
        bool found = false;
        for (int i = 0; i < fileCount; i++) {
            if (!files[i].uploaded) {
                Serial.print("- ");
                Serial.println(files[i].filename);
                found = true;
            }
        }
        if (!found) Serial.println("No pending files");
    }
    
    /**
     * Display all uploaded files
     */
    void showUploaded() {
        bool found = false;
        for (int i = 0; i < fileCount; i++) {
            if (files[i].uploaded) {
                Serial.print("- ");
                Serial.println(files[i].filename);
                found = true;
            }
        }
        if (!found) Serial.println("No uploaded files");
    }
    
    /**
     * Display all tracked files with their status
     */
    void showAll() {
        if (fileCount == 0) {
            Serial.println("No files in tracker");
            return;
        }
        
        Serial.println("All tracked files:");
        for (int i = 0; i < fileCount; i++) {
            Serial.print("- ");
            Serial.print(files[i].filename);
            Serial.print(" | ");
            Serial.println(files[i].uploaded ? "Uploaded" : "Pending");
        }
    }

private:
    /**
     * Check if a file exists in the filesystem
     * @param path Path to the file
     * @return true if file exists, false otherwise
     */
    bool fileExists(const char* path) {
        File f = fs.open(path);
        if (f) {
            f.close();
            return true;
        }
        return false;
    }
    
    /**
     * Load tracking data from the tracker file
     */
    void loadTrackerFile() {
        File file = fs.open(TRACKER_FILE);
        if (!file) return;  // No existing tracker file
        
        fileCount = 0;
        while (file.available() && fileCount < MAX_FILES) {
            String line = file.readStringUntil('\n');
            line.trim();
            
            // Parse each line (format: filename|status)
            int separator = line.indexOf('|');
            if (separator > 0) {
                String fname = line.substring(0, separator);
                strncpy(files[fileCount].filename, fname.c_str(), sizeof(files[0].filename)-1);
                files[fileCount].uploaded = line.substring(separator+1).toInt();
                fileCount++;
            }
        }
        file.close();
    }
    
    /**
     * Save current tracking data to the tracker file
     * @return true if save succeeded, false otherwise
     */
    bool saveTrackerFile() {
        // Remove existing file if it exists
        if (fileExists(TRACKER_FILE)) {
            fs.remove(TRACKER_FILE);
        }
        
        // Create new tracker file
        File file = fs.open(TRACKER_FILE);
        if (!file) {
            Serial.println("Failed to create tracker file");
            return false;
        }
        
        // Write all entries to file
        for (int i = 0; i < fileCount; i++) {
            String line = String(files[i].filename) + "|" + (files[i].uploaded ? "1" : "0");
            file.println(line);
        }
        
        file.close();
        return true;
    }
};

// Global FileTracker instance
FileTracker fileTracker;

void setup() {
    Serial.begin(115200);
    delay(1000);  // Wait for serial to initialize
    
    Serial.println("Initializing file tracker...");
    if (!fileTracker.begin()) {
        Serial.println("Failed to initialize tracker");
        while(1);  // Halt if initialization fails
    }
    
    // Display available commands
    Serial.println("Available commands:");
    Serial.println("create <filename> - Add a test file");
    Serial.println("upload <filename> - Mark file as uploaded");
    Serial.println("showpending - Show pending uploads");
    Serial.println("showuploaded - Show uploaded files");
    Serial.println("showall - Show all tracked files");
}

void loop() {
    // Process serial commands
    if (Serial.available()) {
        String input = Serial.readStringUntil('\n');
        input.trim();
        
        if (input.startsWith("create ")) {
            String filename = input.substring(7);
            if (fileTracker.addFile(filename.c_str())) {
                Serial.print("Added file: ");
                Serial.println(filename);
            } else {
                Serial.println("Failed to add file (may already exist)");
            }
        }
        else if (input.startsWith("upload ")) {
            String filename = input.substring(7);
            if (fileTracker.markAsUploaded(filename.c_str())) {
                Serial.print("Marked as uploaded: ");
                Serial.println(filename);
            } else {
                Serial.println("File not found or already uploaded");
            }
        }
        else if (input == "showpending") {
            Serial.println("Pending uploads:");
            fileTracker.showPending();
        }
        else if (input == "showuploaded") {
            Serial.println("Uploaded files:");
            fileTracker.showUploaded();
        }
        else if (input == "showall") {
            fileTracker.showAll();
        }
        else {
            Serial.println("Unknown command");
        }
    }
    
    delay(100);  // Small delay to prevent busy waiting
}