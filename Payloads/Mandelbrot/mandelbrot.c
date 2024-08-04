#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <math.h>

// Constants
#define DISPLAY_WIDTH 128
#define DISPLAY_HEIGHT 128
#define BYTES_PER_PIXEL 2 // Assuming each pixel is 16 bits (2 bytes)

// Colors
#define BLACK 0x0000
#define RED 0xF800

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
void fill_screen(unsigned char* framebuffer, unsigned short color);
void draw_pixel(unsigned char* framebuffer, int x, int y, unsigned short color);
unsigned short calculate_mandelbrot_color(int x, int y);
void draw_mandelbrot(unsigned char* framebuffer);

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

unsigned short calculate_mandelbrot_color(int x, int y)
{
    const int max_iter = 250;
    const double min_re = -2.25;
    const double max_re = 0.75;
    const double min_im = -1.5;
    const double max_im = min_im + (max_re - min_re) * DISPLAY_HEIGHT / DISPLAY_WIDTH;

    double re_factor = (max_re - min_re) / (DISPLAY_WIDTH - 1);
    double im_factor = (max_im - min_im) / (DISPLAY_HEIGHT - 1);
    double c_re = min_re + x * re_factor;
    double c_im = max_im - y * im_factor;
    double Z_re = c_re, Z_im = c_im;
    int iter;

    for (iter = 0; iter < max_iter; ++iter) {
        double Z_re2 = Z_re * Z_re, Z_im2 = Z_im * Z_im;
        if (Z_re2 + Z_im2 > 4.0)
            break;

        Z_im = 2 * Z_re * Z_im + c_im;
        Z_re = Z_re2 - Z_im2 + c_re;
    }

    if (iter == max_iter)
        return BLACK;

    // Create a red color gradient based on the iteration count
    unsigned char red = (unsigned char)(0x3f * iter / max_iter);
    return (red << 5); // Pack the red value into the 16-bit RGB565 format
}

void draw_mandelbrot(unsigned char* framebuffer)
{
    for (int y = 0; y < DISPLAY_HEIGHT; ++y) {
        for (int x = 0; x < DISPLAY_WIDTH; ++x) {
            unsigned short color = calculate_mandelbrot_color(x, y);
            draw_pixel(framebuffer, x, y, color);
        }
    }
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

    fill_screen(lcd_info.framebuffer, BLACK); // Clear framebuffer to black

    lcd_backlight_enable();

    draw_mandelbrot(lcd_info.framebuffer);

    write_to_fb0(lcd_info.framebuffer, framebuffer_size);

    return 0;
}
