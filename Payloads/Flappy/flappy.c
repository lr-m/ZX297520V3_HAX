#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <math.h>
#include <time.h>
#include <pthread.h>
#include <math.h>

// Constants
#define DISPLAY_WIDTH 128
#define DISPLAY_HEIGHT 128
#define BYTES_PER_PIXEL 2 // Assuming each pixel is 16 bits (2 bytes)
#define DEVICE_FILE "/dev/event0"
#define GRAVITY 0.5
#define JUMP_STRENGTH -5.0
#define PIPE_SPEED 2
#define PIPE_WIDTH 20
#define PIPE_GAP 45
#define CAP_HEIGHT 10
#define CAP_LIP_WIDTH 2

// Colors (BRG)
#define BLACK 0x0000
#define WHITE 0xFFFF
#define GREEN 0x0016
#define LIGHT_GREEN 0x001F
#define DARK_GREEN 0x000b
#define RED 0x07E0
#define BLUE 0xF800
#define YELLOW 0x07FF
#define ORANGE 0x07fb
#define DARK_YELLOW 0x07fe

// for digits
#define DIGIT_WIDTH 7
#define DIGIT_HEIGHT 10

// for flappy
#define FLAPPY_WIDTH 17
#define FLAPPY_HEIGHT 12

// Global variables
int framebuffer_fd = 0; // File descriptor for the framebuffer device
unsigned int framebuffer_size;
unsigned int output_buffer_size;
void *framebuffer_addr = NULL;
void *output_buffer = NULL;
char *error_message = NULL;
int wps_button_pressed = 0;

const unsigned char flappy_bitmap[204] = { // 12x17, 0x0 = black, 0x1 = white, 0x2 = yellow, 0x3 = dark yellow, 0x4 = orange, 0x5 = empty
    0x5, 0x5, 0x5, 0x5, 0x5, 0x5, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x5, 0x5, 0x5, 0x5, 0x5,
    0x5, 0x5, 0x5, 0x5, 0x0, 0x0, 0x2, 0x2, 0x2, 0x2, 0x0, 0x1, 0x0, 0x5, 0x5, 0x5, 0x5,
    0x5, 0x5, 0x5, 0x0, 0x2, 0x2, 0x2, 0x2, 0x2, 0x0, 0x1, 0x1, 0x1, 0x0, 0x5, 0x5, 0x5,
    0x5, 0x0, 0x0, 0x0, 0x0, 0x2, 0x2, 0x2, 0x2, 0x0, 0x1, 0x1, 0x0, 0x1, 0x0, 0x5, 0x5,
    0x0, 0x1, 0x1, 0x1, 0x1, 0x0, 0x2, 0x2, 0x2, 0x0, 0x1, 0x1, 0x0, 0x1, 0x0, 0x5, 0x5,
    0x0, 0x1, 0x1, 0x1, 0x1, 0x1, 0x0, 0x2, 0x2, 0x2, 0x0, 0x1, 0x1, 0x1, 0x0, 0x5, 0x5,
    0x0, 0x2, 0x1, 0x1, 0x1, 0x2, 0x0, 0x2, 0x2, 0x2, 0x2, 0x0, 0x0, 0x0, 0x0, 0x0, 0x5,
    0x5, 0x0, 0x2, 0x2, 0x2, 0x0, 0x2, 0x2, 0x2, 0x2, 0x0, 0x4, 0x4, 0x4, 0x4, 0x4, 0x0,
    0x5, 0x5, 0x0, 0x0, 0x0, 0x3, 0x3, 0x3, 0x3, 0x0, 0x4, 0x0, 0x0, 0x0, 0x0, 0x0, 0x5,
    0x5, 0x5, 0x5, 0x5, 0x0, 0x3, 0x3, 0x3, 0x3, 0x3, 0x0, 0x4, 0x4, 0x4, 0x4, 0x0, 0x5,
    0x5, 0x5, 0x5, 0x5, 0x5, 0x0, 0x0, 0x3, 0x3, 0x3, 0x3, 0x0, 0x0, 0x0, 0x0, 0x0, 0x5,
    0x5, 0x5, 0x5, 0x5, 0x5, 0x5, 0x5, 0x0, 0x0, 0x0, 0x0, 0x5, 0x5, 0x5, 0x5, 0x5, 0x5,
};

const unsigned char digit_bitmaps[10][70] = { // 0 = white, 1 = black, 2 = empty
    {
        0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1,
        0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1,
        0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1,
        0x1, 0x0, 0x0, 0x1, 0x0, 0x0, 0x1,
        0x1, 0x0, 0x0, 0x1, 0x0, 0x0, 0x1,
        0x1, 0x0, 0x0, 0x1, 0x0, 0x0, 0x1,
        0x1, 0x0, 0x0, 0x1, 0x0, 0x0, 0x1,
        0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1,
        0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1,
        0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1,
    }, // 0
    {
        0x2, 0x1, 0x1, 0x1, 0x1, 0x1, 0x2,
        0x2, 0x1, 0x0, 0x0, 0x0, 0x1, 0x2,
        0x2, 0x1, 0x0, 0x0, 0x0, 0x1, 0x2,
        0x2, 0x1, 0x1, 0x0, 0x0, 0x1, 0x2,
        0x2, 0x2, 0x1, 0x0, 0x0, 0x1, 0x2,
        0x2, 0x2, 0x1, 0x0, 0x0, 0x1, 0x2,
        0x2, 0x2, 0x1, 0x0, 0x0, 0x1, 0x2,
        0x2, 0x2, 0x1, 0x0, 0x0, 0x1, 0x2,
        0x2, 0x2, 0x1, 0x0, 0x0, 0x1, 0x2,
        0x2, 0x2, 0x1, 0x1, 0x1, 0x1, 0x2,
    }, // 1
    {
        0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1,
        0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1,
        0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1,
        0x1, 0x1, 0x1, 0x1, 0x0, 0x0, 0x1,
        0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1,
        0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1,
        0x1, 0x0, 0x0, 0x1, 0x1, 0x1, 0x1,
        0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1,
        0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1,
        0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1,
    }, // 2
    {
        0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1,
        0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1,
        0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1,
        0x1, 0x1, 0x1, 0x1, 0x0, 0x0, 0x1,
        0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1,
        0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1,
        0x1, 0x1, 0x1, 0x1, 0x0, 0x0, 0x1,
        0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1,
        0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1,
        0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1,
    }, // 3
    {
        0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1,
        0x1, 0x0, 0x0, 0x1, 0x0, 0x0, 0x1,
        0x1, 0x0, 0x0, 0x1, 0x0, 0x0, 0x1,
        0x1, 0x0, 0x0, 0x1, 0x0, 0x0, 0x1,
        0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1,
        0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1,
        0x1, 0x1, 0x1, 0x1, 0x0, 0x0, 0x1,
        0x2, 0x2, 0x2, 0x1, 0x0, 0x0, 0x1,
        0x2, 0x2, 0x2, 0x1, 0x0, 0x0, 0x1,
        0x2, 0x2, 0x2, 0x1, 0x1, 0x1, 0x1,
    }, // 4
    {
        0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1,
        0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1,
        0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1,
        0x1, 0x0, 0x0, 0x1, 0x1, 0x1, 0x1,
        0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1,
        0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1,
        0x1, 0x1, 0x1, 0x1, 0x0, 0x0, 0x1,
        0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1,
        0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1,
        0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1,
    }, // 5
    {
        0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1,
        0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1,
        0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1,
        0x1, 0x0, 0x0, 0x1, 0x1, 0x1, 0x1,
        0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1,
        0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1,
        0x1, 0x0, 0x0, 0x1, 0x0, 0x0, 0x1,
        0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1,
        0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1,
        0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1,
    }, // 6
    {
        0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1,
        0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1,
        0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1,
        0x1, 0x1, 0x1, 0x1, 0x0, 0x0, 0x1,
        0x2, 0x2, 0x2, 0x1, 0x0, 0x0, 0x1,
        0x2, 0x2, 0x2, 0x1, 0x0, 0x0, 0x1,
        0x2, 0x2, 0x2, 0x1, 0x0, 0x0, 0x1,
        0x2, 0x2, 0x2, 0x1, 0x0, 0x0, 0x1,
        0x2, 0x2, 0x2, 0x1, 0x0, 0x0, 0x1,
        0x2, 0x2, 0x2, 0x1, 0x1, 0x1, 0x1,
    }, // 7
    {
        0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1,
        0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1,
        0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1,
        0x1, 0x0, 0x0, 0x1, 0x0, 0x0, 0x1,
        0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1,
        0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1,
        0x1, 0x0, 0x0, 0x1, 0x0, 0x0, 0x1,
        0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1,
        0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1,
        0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1,
    }, // 8
    {
        0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1,
        0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1,
        0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1,
        0x1, 0x0, 0x0, 0x1, 0x0, 0x0, 0x1,
        0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1,
        0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1,
        0x1, 0x1, 0x1, 0x1, 0x0, 0x0, 0x1,
        0x2, 0x2, 0x2, 0x1, 0x0, 0x0, 0x1,
        0x2, 0x2, 0x2, 0x1, 0x0, 0x0, 0x1,
        0x2, 0x2, 0x2, 0x1, 0x1, 0x1, 0x1,
    }, // 9
};

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
unsigned int lcd_set_backlight(unsigned int setting);
unsigned int lcd_set_sleep(unsigned int setting);
unsigned int write_to_fb0(unsigned char* framebuffer, unsigned int size);
void fill_screen(unsigned char* framebuffer, unsigned short color);
void draw_pixel(unsigned char* framebuffer, int x, int y, unsigned short color);
void draw_circle(unsigned char* framebuffer, int xc, int yc, int r, unsigned short color);
void fill_circle(unsigned char* framebuffer, int xc, int yc, int r, unsigned short color);
void draw_rect(unsigned char* framebuffer, int x, int y, int w, int h, unsigned short color);
void fill_rect(unsigned char* framebuffer, int x, int y, int w, int h, unsigned short color);
void handle_wps_button_press(void);
void draw_score(unsigned char* framebuffer, int score);


// Swap function definition
void swap(int *a, int *b) {
    int temp = *a;
    *a = *b;
    *b = temp;
}

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

unsigned int lcd_set_backlight(unsigned int setting)
{
    unsigned int local_c[3] = { setting };

    if (ioctl(framebuffer_fd, 0x40044c02, local_c) < 0) {
        perror("Enabling backlight failed");
        return 0xffffffff;
    }

    printf("Backlight enabled successfully.\n");
    return 0;
}

unsigned int lcd_set_sleep(unsigned int setting)
{
    unsigned int local_c[3] = { setting };

    if (ioctl(framebuffer_fd, 0x40044c01, local_c) < 0) {
        perror("Setting brightness failed");
        return 0xffffffff;
    }

    printf("Brightness set successfully.\n");
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

void fill_screen(unsigned char* framebuffer, unsigned short color)
{
    for (int y = 0; y < DISPLAY_HEIGHT; ++y) {
        for (int x = 0; x < DISPLAY_WIDTH; ++x) {
            draw_pixel(framebuffer, x, y, color);
        }
    }
}

void draw_pixel(unsigned char* framebuffer, int x, int y, unsigned short color)
{
    if (x >= 0 && x < DISPLAY_WIDTH && y >= 0 && y < DISPLAY_HEIGHT) {
        size_t pixel_index = (y * DISPLAY_WIDTH + x) * BYTES_PER_PIXEL;
        framebuffer[pixel_index] = color & 0xFF;
        framebuffer[pixel_index + 1] = (color >> 8) & 0xFF;
    }
}

void draw_circle(unsigned char* framebuffer, int xc, int yc, int r, unsigned short color)
{
    int x = r, y = 0;
    int err = 0;

    while (x >= y) {
        draw_pixel(framebuffer, xc + x, yc + y, color);
        draw_pixel(framebuffer, xc + y, yc + x, color);
        draw_pixel(framebuffer, xc - y, yc + x, color);
        draw_pixel(framebuffer, xc - x, yc + y, color);
        draw_pixel(framebuffer, xc - x, yc - y, color);
        draw_pixel(framebuffer, xc - y, yc - x, color);
        draw_pixel(framebuffer, xc + y, yc - x, color);
        draw_pixel(framebuffer, xc + x, yc - y, color);

        if (err <= 0) {
            y += 1;
            err += 2 * y + 1;
        }
        if (err > 0) {
            x -= 1;
            err -= 2 * x + 1;
        }
    }
}

void fill_circle(unsigned char* framebuffer, int xc, int yc, int r, unsigned short color)
{
    int x = r, y = 0;
    int err = 0;

    while (x >= y) {
        for (int i = -x; i <= x; ++i) {
            draw_pixel(framebuffer, xc + i, yc + y, color);
            draw_pixel(framebuffer, xc + i, yc - y, color);
        }
        for (int i = -y; i <= y; ++i) {
            draw_pixel(framebuffer, xc + i, yc + x, color);
            draw_pixel(framebuffer, xc + i, yc - x, color);
        }

        if (err <= 0) {
            y += 1;
            err += 2 * y + 1;
        }
        if (err > 0) {
            x -= 1;
            err -= 2 * x + 1;
        }
    }
}

void draw_rect(unsigned char* framebuffer, int x, int y, int w, int h, unsigned short color)
{
    for (int i = x; i < x + w; ++i) {
        draw_pixel(framebuffer, i, y, color);
        draw_pixel(framebuffer, i, y + h - 1, color);
    }
    for (int i = y; i < y + h; ++i) {
        draw_pixel(framebuffer, x, i, color);
        draw_pixel(framebuffer, x + w - 1, i, color);
    }
}

void fill_rect(unsigned char* framebuffer, int x, int y, int w, int h, unsigned short color)
{
    for (int i = y; i < y + h; ++i) {
        for (int j = x; j < x + w; ++j) {
            draw_pixel(framebuffer, j, i, color);
        }
    }
}

// Event handling for WPS button
void handle_wps_button_press(void) {
    int fd;
    struct EventData {
        unsigned long long timestamp;  // 8 bytes for timestamp
        unsigned short type;           // 2 bytes for type
        unsigned short code;           // 2 bytes for code
        unsigned int value;            // 4 bytes for value
    } event_data;
    ssize_t bytes_read;

    // Open the device file
    fd = open(DEVICE_FILE, O_RDONLY);
    if (fd == -1) {
        perror("Failed to open the device file");
        return;
    }

    while (1) {
        // Read 16 bytes
        bytes_read = read(fd, &event_data, sizeof(event_data));
        if (bytes_read != sizeof(event_data)) {
            perror("Failed to read event_data");
            close(fd);
            return;
        }

        if (event_data.type == 1 && event_data.value == 1 && event_data.code == 117) {
            wps_button_pressed = 1;
        }

        usleep(1000); // Small delay to prevent CPU overload
    }

    // Close the device file
    close(fd);
}

void draw_digit(unsigned char* framebuffer, int x, int y, int digit) {
    const unsigned char* bitmap = digit_bitmaps[digit];
    for (int i = 0; i < DIGIT_HEIGHT; ++i) {
        for (int j = 0; j < DIGIT_WIDTH; ++j) {
            int pixel = bitmap[i * DIGIT_WIDTH + j];
            int color;
            if (pixel == 1) {
                color = BLACK;
            } else if (pixel == 0) {
                color = WHITE;
            } else {
                continue; // Skip EMPTY pixels
            }
            draw_pixel(framebuffer, x + j, y + i, color);
        }
    }
}

void draw_score(unsigned char* framebuffer, int score) {
    char score_str[10];
    snprintf(score_str, sizeof(score_str), "%d", score);
    int num_digits = strlen(score_str);
    int total_width = num_digits * DIGIT_WIDTH;
    int start_x = (DISPLAY_WIDTH - total_width) / 2;
    int start_y = DIGIT_HEIGHT;

    for (int i = 0; i < num_digits; ++i) {
        int digit = score_str[i] - '0';
        draw_digit(framebuffer, start_x + i * DIGIT_WIDTH, start_y, digit);
    }
}

void draw_flappy_bitmap_rotated(unsigned char* framebuffer, int x, int y, float angle) {
    float rad = angle * M_PI / 180.0;
    float cos_theta = cos(rad);
    float sin_theta = sin(rad);

    int cx = FLAPPY_WIDTH / 2;
    int cy = FLAPPY_HEIGHT / 2;

    for (int i = 0; i < FLAPPY_HEIGHT; ++i) {
        for (int j = 0; j < FLAPPY_WIDTH; ++j) {
            int pixel = flappy_bitmap[i * FLAPPY_WIDTH + j];
            if (pixel == 0x5) continue; // Skip EMPTY pixels

            // Translate pixel to origin
            int tx = j - cx;
            int ty = i - cy;

            // Rotate pixel
            int rx = cos_theta * tx - sin_theta * ty;
            int ry = sin_theta * tx + cos_theta * ty;

            // Translate back to original position
            int nx = rx + cx;
            int ny = ry + cy;

            int color;
            switch (pixel) {
                case 0x0:
                    color = BLACK;
                    break;
                case 0x1:
                    color = WHITE;
                    break;
                case 0x2:
                    color = YELLOW;
                    break;
                case 0x3:
                    color = DARK_YELLOW;
                    break;
                case 0x4:
                    color = ORANGE;
                    break;
                default:
                    continue;
            }
            draw_pixel(framebuffer, x + nx, y + ny, color);
        }
    }
}
   

int main(void) {
    unsigned int result = init_lcd();
    if (result != 0) {
        fprintf(stderr, "LCD initialization failed.\n");
        return 1;
    }

    // open background image file
    FILE *background_file = fopen("/tmp/flappy_bg.rgb", "rb");
    if (!background_file) {
        perror("Failed to open image file");
        return 1;
    }

    LCD_INFO lcd_info;
    result = get_lcd_info(&lcd_info);
    if (result != 0) {
        fprintf(stderr, "Failed to retrieve LCD info.\n");
        return 1;
    }

    fill_screen(lcd_info.framebuffer, BLUE); // Clear framebuffer to black

    lcd_set_sleep(0);
    lcd_set_backlight(1);

    // Start WPS button event handler in a new thread
    pthread_t wps_thread;
    pthread_create(&wps_thread, NULL, (void *)handle_wps_button_press, NULL);

    int bird_x = 30, bird_y = DISPLAY_HEIGHT / 2, bird_radius = 6;
    float bird_velocity = 0;
    int pipe_x = DISPLAY_WIDTH, pipe_y = rand() % (DISPLAY_HEIGHT - PIPE_GAP);
    int pipe_height_top = pipe_y;
    int pipe_height_bottom = DISPLAY_HEIGHT - (pipe_y + PIPE_GAP);
    int score = 0;
    int bird_passed_pipe = 0;
    int cap_width = PIPE_WIDTH + (2 * CAP_LIP_WIDTH);

    int game_running = 1;
    double angle = 0;

    // draw intro and wait for WPS press to start game
    fread(lcd_info.framebuffer, sizeof(unsigned char), framebuffer_size, background_file); // draw background onto screen first
    fseek(background_file, 0, SEEK_SET); // go back to beginning

    draw_score(lcd_info.framebuffer, score);

    draw_flappy_bitmap_rotated(lcd_info.framebuffer, bird_x - FLAPPY_WIDTH/2, bird_y - FLAPPY_HEIGHT/2, 0);

    write_to_fb0(lcd_info.framebuffer, framebuffer_size);

    while (!wps_button_pressed) {
        usleep(30000); // Small delay to control the animation speed
    }

    while(1){
        while (game_running) {
            if (wps_button_pressed) {
                bird_velocity = JUMP_STRENGTH;
                wps_button_pressed = 0;
            }

            bird_velocity += GRAVITY;
            bird_y += bird_velocity;

            if (bird_y + bird_radius >= DISPLAY_HEIGHT || bird_y - bird_radius <= 0) {
                game_running = 0;
            }

            pipe_x -= PIPE_SPEED;
            if (pipe_x + PIPE_WIDTH < 0) {
                pipe_x = DISPLAY_WIDTH;
                pipe_y = rand() % (DISPLAY_HEIGHT - PIPE_GAP);
                pipe_height_top = pipe_y;
                pipe_height_bottom = DISPLAY_HEIGHT - (pipe_y + PIPE_GAP);
                bird_passed_pipe = 0;
            }

            // Collision detection
            if (bird_x + bird_radius > pipe_x && bird_x - bird_radius < pipe_x + PIPE_WIDTH) {
                if (bird_y - bird_radius < pipe_height_top || bird_y + bird_radius > DISPLAY_HEIGHT - pipe_height_bottom) {
                    game_running = 0;
                }
            }

            // Score updating
            if (!bird_passed_pipe && pipe_x + PIPE_WIDTH < bird_x - bird_radius) {
                score++;
                bird_passed_pipe = 1;
            }

            fread(lcd_info.framebuffer, sizeof(unsigned char), framebuffer_size, background_file); // draw background onto screen first
            fseek(background_file, 0, SEEK_SET); // go back to beginning

            // draw main part of pipe
            fill_rect(lcd_info.framebuffer, pipe_x, 0, PIPE_WIDTH/4, pipe_height_top - (CAP_HEIGHT - 1), LIGHT_GREEN);
            fill_rect(lcd_info.framebuffer, pipe_x + PIPE_WIDTH/4, 0, PIPE_WIDTH/2, pipe_height_top - (CAP_HEIGHT - 1), GREEN);
            fill_rect(lcd_info.framebuffer, pipe_x + 3*PIPE_WIDTH/4, 0, PIPE_WIDTH/4, pipe_height_top - (CAP_HEIGHT - 1), DARK_GREEN);

            fill_rect(lcd_info.framebuffer, pipe_x, DISPLAY_HEIGHT - pipe_height_bottom + (CAP_HEIGHT - 1), PIPE_WIDTH/4, pipe_height_bottom, LIGHT_GREEN);
            fill_rect(lcd_info.framebuffer, pipe_x + PIPE_WIDTH/4, DISPLAY_HEIGHT - pipe_height_bottom + (CAP_HEIGHT - 1), PIPE_WIDTH/2, pipe_height_bottom, GREEN);
            fill_rect(lcd_info.framebuffer, pipe_x + 3*PIPE_WIDTH/4, DISPLAY_HEIGHT - pipe_height_bottom + (CAP_HEIGHT - 1), PIPE_WIDTH/4, pipe_height_bottom, DARK_GREEN);

            draw_rect(lcd_info.framebuffer, pipe_x, 0, PIPE_WIDTH, pipe_height_top - (CAP_HEIGHT - 1), BLACK);
            draw_rect(lcd_info.framebuffer, pipe_x, DISPLAY_HEIGHT - pipe_height_bottom + (CAP_HEIGHT - 1), PIPE_WIDTH, pipe_height_bottom, BLACK);

            // draw caps
            fill_rect(lcd_info.framebuffer, pipe_x - CAP_LIP_WIDTH, pipe_height_top - CAP_HEIGHT, cap_width/4, CAP_HEIGHT, LIGHT_GREEN);
            fill_rect(lcd_info.framebuffer, (pipe_x - CAP_LIP_WIDTH) + cap_width/4, pipe_height_top - CAP_HEIGHT, cap_width/2, CAP_HEIGHT, GREEN);
            fill_rect(lcd_info.framebuffer, (pipe_x - CAP_LIP_WIDTH) + 3*cap_width/4, pipe_height_top - CAP_HEIGHT, cap_width/4, CAP_HEIGHT, DARK_GREEN);

            fill_rect(lcd_info.framebuffer, pipe_x - CAP_LIP_WIDTH, DISPLAY_HEIGHT - pipe_height_bottom, cap_width/4, CAP_HEIGHT, LIGHT_GREEN);
            fill_rect(lcd_info.framebuffer, (pipe_x - CAP_LIP_WIDTH) + cap_width/4, DISPLAY_HEIGHT - pipe_height_bottom, cap_width/2, CAP_HEIGHT, GREEN);
            fill_rect(lcd_info.framebuffer, (pipe_x - CAP_LIP_WIDTH) + 3*cap_width/4, DISPLAY_HEIGHT - pipe_height_bottom, cap_width/4, CAP_HEIGHT, DARK_GREEN);

            draw_rect(lcd_info.framebuffer, pipe_x - CAP_LIP_WIDTH, pipe_height_top - CAP_HEIGHT, PIPE_WIDTH + (2 * CAP_LIP_WIDTH), CAP_HEIGHT, BLACK);
            draw_rect(lcd_info.framebuffer, pipe_x - CAP_LIP_WIDTH, DISPLAY_HEIGHT - pipe_height_bottom, PIPE_WIDTH + (2 * CAP_LIP_WIDTH), CAP_HEIGHT, BLACK);

            // draw the bird, calculate angle with current velocity (cap at -45 and 45)
            angle = bird_velocity * 4.0;
            if (angle < -45.0) {
                angle = -45.0;
            } else if (angle > 45.0) {
                angle = 45.0;
            }

            draw_flappy_bitmap_rotated(lcd_info.framebuffer, bird_x - FLAPPY_WIDTH/2, bird_y - FLAPPY_HEIGHT/2, angle);

            // draw the current score
            draw_score(lcd_info.framebuffer, score);

            write_to_fb0(lcd_info.framebuffer, framebuffer_size);

            usleep(30000); // Small delay to control the game speed
        }

        while(bird_y < DISPLAY_HEIGHT - FLAPPY_WIDTH/2){
            fread(lcd_info.framebuffer, sizeof(unsigned char), framebuffer_size, background_file); // draw background onto screen first
            fseek(background_file, 0, SEEK_SET); // go back to beginning

            // draw main part of pipe
            fill_rect(lcd_info.framebuffer, pipe_x, 0, PIPE_WIDTH/4, pipe_height_top - (CAP_HEIGHT - 1), LIGHT_GREEN);
            fill_rect(lcd_info.framebuffer, pipe_x + PIPE_WIDTH/4, 0, PIPE_WIDTH/2, pipe_height_top - (CAP_HEIGHT - 1), GREEN);
            fill_rect(lcd_info.framebuffer, pipe_x + 3*PIPE_WIDTH/4, 0, PIPE_WIDTH/4, pipe_height_top - (CAP_HEIGHT - 1), DARK_GREEN);

            fill_rect(lcd_info.framebuffer, pipe_x, DISPLAY_HEIGHT - pipe_height_bottom + (CAP_HEIGHT - 1), PIPE_WIDTH/4, pipe_height_bottom, LIGHT_GREEN);
            fill_rect(lcd_info.framebuffer, pipe_x + PIPE_WIDTH/4, DISPLAY_HEIGHT - pipe_height_bottom + (CAP_HEIGHT - 1), PIPE_WIDTH/2, pipe_height_bottom, GREEN);
            fill_rect(lcd_info.framebuffer, pipe_x + 3*PIPE_WIDTH/4, DISPLAY_HEIGHT - pipe_height_bottom + (CAP_HEIGHT - 1), PIPE_WIDTH/4, pipe_height_bottom, DARK_GREEN);

            draw_rect(lcd_info.framebuffer, pipe_x, 0, PIPE_WIDTH, pipe_height_top - (CAP_HEIGHT - 1), BLACK);
            draw_rect(lcd_info.framebuffer, pipe_x, DISPLAY_HEIGHT - pipe_height_bottom + (CAP_HEIGHT - 1), PIPE_WIDTH, pipe_height_bottom, BLACK);

            // draw caps
            fill_rect(lcd_info.framebuffer, pipe_x - CAP_LIP_WIDTH, pipe_height_top - CAP_HEIGHT, cap_width/4, CAP_HEIGHT, LIGHT_GREEN);
            fill_rect(lcd_info.framebuffer, (pipe_x - CAP_LIP_WIDTH) + cap_width/4, pipe_height_top - CAP_HEIGHT, cap_width/2, CAP_HEIGHT, GREEN);
            fill_rect(lcd_info.framebuffer, (pipe_x - CAP_LIP_WIDTH) + 3*cap_width/4, pipe_height_top - CAP_HEIGHT, cap_width/4, CAP_HEIGHT, DARK_GREEN);

            fill_rect(lcd_info.framebuffer, pipe_x - CAP_LIP_WIDTH, DISPLAY_HEIGHT - pipe_height_bottom, cap_width/4, CAP_HEIGHT, LIGHT_GREEN);
            fill_rect(lcd_info.framebuffer, (pipe_x - CAP_LIP_WIDTH) + cap_width/4, DISPLAY_HEIGHT - pipe_height_bottom, cap_width/2, CAP_HEIGHT, GREEN);
            fill_rect(lcd_info.framebuffer, (pipe_x - CAP_LIP_WIDTH) + 3*cap_width/4, DISPLAY_HEIGHT - pipe_height_bottom, cap_width/4, CAP_HEIGHT, DARK_GREEN);

            draw_rect(lcd_info.framebuffer, pipe_x - CAP_LIP_WIDTH, pipe_height_top - CAP_HEIGHT, PIPE_WIDTH + (2 * CAP_LIP_WIDTH), CAP_HEIGHT, BLACK);
            draw_rect(lcd_info.framebuffer, pipe_x - CAP_LIP_WIDTH, DISPLAY_HEIGHT - pipe_height_bottom, PIPE_WIDTH + (2 * CAP_LIP_WIDTH), CAP_HEIGHT, BLACK);

            // draw the current score
            draw_score(lcd_info.framebuffer, score);

            // draw the bird, calculate angle with current velocity (cap at -45 and 45)
            draw_flappy_bitmap_rotated(lcd_info.framebuffer, bird_x - FLAPPY_WIDTH/2, bird_y - FLAPPY_HEIGHT/2, 90);

            write_to_fb0(lcd_info.framebuffer, framebuffer_size);

            bird_y++;
        }

        while (!wps_button_pressed) {
            usleep(30000); // Small delay to control the animation speed
        }

        // now reset everything
        game_running = 1;
        wps_button_pressed = 0;

        bird_y = DISPLAY_HEIGHT / 2;
        bird_velocity = 0;
        pipe_x = DISPLAY_WIDTH;
        pipe_y = rand() % (DISPLAY_HEIGHT - PIPE_GAP);
        pipe_height_top = pipe_y;
        pipe_height_bottom = DISPLAY_HEIGHT - (pipe_y + PIPE_GAP);
        score = 0;
        bird_passed_pipe = 0;
    }

    pthread_cancel(wps_thread);
    pthread_join(wps_thread, NULL);

    return 0;
}
