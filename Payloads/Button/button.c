#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#define DEVICE_FILE "/dev/event0"

// Define the structure to hold the parsed data
struct EventData {
    unsigned long long timestamp;  // 8 bytes for timestamp
    unsigned short type;           // 2 bytes for type
    unsigned short code;           // 2 bytes for code
    unsigned int value;            // 4 bytes for value
};

int main() {
    int fd;
    struct EventData event_data;
    ssize_t bytes_read;

    // Open the device file
    fd = open(DEVICE_FILE, O_RDONLY);
    if (fd == -1) {
        perror("Failed to open the device file");
        return 1;
    }

    while(1){
        // Read 16 bytes
        bytes_read = read(fd, &event_data, sizeof(struct EventData));
        if (bytes_read != sizeof(struct EventData)) {
            perror("Failed to read struct EventData");
            close(fd);
            return 1;
        }

        if (event_data.type == 1 && event_data.value == 1){
            if (event_data.code == 117){
                printf("[*] WPS button down\n");
            } else if (event_data.code == 116){
                printf("[*] Power button down\n");
            }
        }

        if (event_data.type == 1 && event_data.value == 0){
            if (event_data.code == 117){
                printf("[*] WPS button up\n");
            } else if (event_data.code == 116){
                printf("[*] Power button up\n");
            }
        }
    }

    // Close the device file
    close(fd);

    return 0;
}
