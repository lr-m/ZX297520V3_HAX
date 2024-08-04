#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/mman.h>

// Constants
#define DISPLAY_WIDTH 128
#define DISPLAY_HEIGHT 128
#define BYTES_PER_PIXEL 2 // Assuming each pixel is 16 bits (2 bytes)

// Global variables
int framebuffer_fd = 0; // File descriptor for the framebuffer device
unsigned int framebuffer_size;
unsigned int output_buffer_size;
void *framebuffer_addr = NULL;
void *output_buffer = NULL;
char *error_message = NULL;

// Structure definitions
typedef struct ioctl_lcd_info {
    uint width;
    uint height;
    uint unknown1;
    uint unknown2;
    uint unknown3;
    uint unknown4;
    uint bpp;
    uint unknown5[64];
} IOCTL_LCD_INFO;

typedef struct lcd_info {
    ushort width;
    ushort height;
    uint always_1;
    ushort bpp;
    ushort width_something;
    unsigned char* framebuffer;
} LCD_INFO;

// Function declarations
unsigned int init_lcd(void);
unsigned int get_lcd_info(LCD_INFO *info);
unsigned int lcd_set_brightness(unsigned int brightness);
unsigned int lcd_backlight_enable(void);
unsigned int write_to_fb0(unsigned char* framebuffer, unsigned int size);
void set_pixels_pattern(unsigned char* framebuffer);
void load_rgb565_image_to_framebuffer(const char *filename, unsigned char *framebuffer, unsigned int framebuffer_size);

// Function definitions

unsigned int init_lcd(void)
{
    int fd = open("/dev/fb0", O_RDWR);
    if (fd < 0) {
        perror("Failed to open /dev/fb0");
        return 0xffffffff;
    }

    printf("Opened /dev/fb0 successfully.\n");
    close(fd);
    return 0;
}

unsigned int get_lcd_info(LCD_INFO *info)
{
    if (info == NULL) {
        return 0xffffffff;
    }

    framebuffer_fd = open("/dev/fb0", O_RDWR);
    if (framebuffer_fd < 0) {
        perror("Failed to open /dev/fb0");
        return 0xffffffff;
    }

    IOCTL_LCD_INFO ioctl_lcd_info;
    if (ioctl(framebuffer_fd, 0x4600, &ioctl_lcd_info) < 0) {
        perror("ioctl get_lcd_info failed");
        close(framebuffer_fd);
        framebuffer_fd = 0;
        return 0xffffffff;
    }

    info->width = (unsigned short)ioctl_lcd_info.width;
    info->height = (unsigned short)ioctl_lcd_info.height;
    info->bpp = (unsigned short)ioctl_lcd_info.bpp;
    info->always_1 = 1;
    info->width_something = (unsigned short)((ioctl_lcd_info.width & 0xffff) << 1);
    framebuffer_size = info->width * info->height * 2;
    output_buffer_size = info->bpp * info->width * info->height / 2;
    output_buffer = malloc(output_buffer_size * 4);

    if (output_buffer == NULL) {
        fprintf(stderr, "Failed to allocate output buffer.\n");
        close(framebuffer_fd);
        framebuffer_fd = 0;
        return 0xffffffff;
    }

    framebuffer_addr = mmap(NULL, framebuffer_size, PROT_READ | PROT_WRITE, MAP_SHARED, framebuffer_fd, 0);
    if (framebuffer_addr == MAP_FAILED) {
        perror("mmap failed");
        free(output_buffer);
        close(framebuffer_fd);
        framebuffer_fd = 0;
        return 0xffffffff;
    }

    info->framebuffer = framebuffer_addr;

    printf("LCD Info: width = %d | height = %d | bpp = %d | framebuffer address = %p | framebuffer size = %u\n",
           info->width, info->height, info->bpp, framebuffer_addr, output_buffer_size);

    return 0;
}

unsigned int lcd_set_brightness(unsigned int brightness)
{
    unsigned int local_c[3] = { brightness };

    if (ioctl(framebuffer_fd, 0x40044c03, local_c) < 0) {
        perror("Setting brightness failed");
        return 0xffffffff;
    }

    printf("Brightness set successfully.\n");
    return 0;
}

unsigned int lcd_backlight_enable(void)
{
    unsigned int local_c[3] = { 1 };

    if (ioctl(framebuffer_fd, 0x40044c02, local_c) < 0) {
        perror("Enabling backlight failed");
        return 0xffffffff;
    }

    printf("Backlight enabled successfully.\n");
    return 0;
}

unsigned int write_to_fb0(unsigned char* framebuffer, unsigned int size)
{
    int fd = open("/dev/fb0", O_RDWR);
    if (fd < 0) {
        perror("Failed to open /dev/fb0 for writing");
        return 0xffffffff;
    }

    write(fd, framebuffer, size);
    close(fd);
    return 0;
}

void set_pixels_pattern(unsigned char* framebuffer)
{
    unsigned char pattern[] = { 0b10000001, 0b01100111 }; // Example pattern

    for (int y = 0; y < DISPLAY_HEIGHT; ++y) {
        for (int x = 0; x < DISPLAY_WIDTH; ++x) {
            size_t pixel_index = (y * DISPLAY_WIDTH + x) * BYTES_PER_PIXEL;
            for (int byte = 0; byte < BYTES_PER_PIXEL; ++byte) {
                framebuffer[pixel_index + byte] = pattern[byte];
            }
        }
    }
}

void load_rgb565_image_to_framebuffer(const char *filename, unsigned char *framebuffer, unsigned int framebuffer_size)
{
    FILE *file = fopen(filename, "rb");
    if (!file) {
        perror("Failed to open image file");
        return;
    }

    fread(framebuffer, sizeof(unsigned char), framebuffer_size, file);

    printf("%s loaded to framebuffer\n", filename);

    fclose(file);
}

int main(void)
{
    unsigned int result = init_lcd();
    if (result != 0) {
        fprintf(stderr, "LCD initialization failed.\n");
        return 1;
    }

    LCD_INFO lcd_info;
    result = get_lcd_info(&lcd_info);
    if (result != 0) {
        fprintf(stderr, "Failed to retrieve LCD info.\n");
        return 1;
    }

    memset(lcd_info.framebuffer, 0x0, framebuffer_size); // Clear framebuffer

    lcd_backlight_enable();

    load_rgb565_image_to_framebuffer("/tmp/walter.rgb", lcd_info.framebuffer, framebuffer_size);

    write_to_fb0(lcd_info.framebuffer, framebuffer_size);

    return 0;
}
