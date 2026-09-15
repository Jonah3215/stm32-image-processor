#include <string.h>   // required for memset()
#include <stdint.h>   // required for uint8_t, uint16_t, uint32_t
#include <stdio.h>    // required for printf()
#include <stdlib.h>

// polyfill for ARM __USAT instruction when compiling for desktop
#ifdef __arm__
    // we are on ARM use the real hardware instruction
    #include "cmsis_compiler.h"
#else
    // we are on Desktop, use the software mock
    #define __USAT(val, sat) ((val > 255) ? 255 : ((val < 0) ? 0 : val))
#endif

void process_image_line(const uint8_t* restrict top, 
                        const uint8_t* restrict mid, 
                        const uint8_t* restrict bot, 
                        uint16_t* restrict out, 
                        uint8_t is_gr_row); 
void update_ISP_settings(void);
void multiply_CCM(float* r, float* g, float* b);
float get_sin(uint32_t degrees);
float get_cos(uint32_t degrees);

// give a list of the gamma LUTs; this takes up 1280 bytes of memory
const uint8_t GAMMA_LUTS[5][256] = {
    // Mode 0: Linear (y = x)
    {
          0,   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,  15,
         16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,
         32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,
         48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  62,  63,
         64,  65,  66,  67,  68,  69,  70,  71,  72,  73,  74,  75,  76,  77,  78,  79,
         80,  81,  82,  83,  84,  85,  86,  87,  88,  89,  90,  91,  92,  93,  94,  95,
         96,  97,  98,  99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
        112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127,
        128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143,
        144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159,
        160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175,
        176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191,
        192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207,
        208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223,
        224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239,
        240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255
    },
    // Mode 1: Gamma 1.8
    {
          0,   0,   0,   0,   0,   0,   0,   0,   1,   1,   1,   1,   1,   1,   1,   1,
          2,   2,   2,   2,   2,   2,   3,   3,   3,   3,   3,   4,   4,   4,   4,   4,
          5,   5,   5,   5,   6,   6,   6,   6,   7,   7,   7,   8,   8,   8,   9,   9,
          9,  10,  10,  10,  11,  11,  11,  12,  12,  13,  13,  13,  14,  14,  15,  15,
         16,  16,  17,  17,  18,  18,  19,  19,  20,  20,  21,  21,  22,  22,  23,  23,
         24,  25,  25,  26,  26,  27,  28,  28,  29,  29,  30,  31,  31,  32,  33,  33,
         34,  35,  35,  36,  37,  37,  38,  39,  39,  40,  41,  41,  42,  43,  44,  44,
         45,  46,  47,  47,  48,  49,  50,  50,  51,  52,  53,  53,  54,  55,  56,  57,
         57,  58,  59,  60,  61,  61,  62,  63,  64,  65,  66,  66,  67,  68,  69,  70,
         71,  71,  72,  73,  74,  75,  76,  77,  77,  78,  79,  80,  81,  82,  83,  84,
         85,  85,  86,  87,  88,  89,  90,  91,  92,  93,  94,  94,  95,  96,  97,  98,
         99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114,
        115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130,
        131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146,
        147, 148, 149, 150, 151, 152, 154, 155, 156, 157, 158, 159, 160, 161, 162, 164,
        165, 166, 167, 168, 169, 171, 172, 173, 174, 175, 177, 178, 179, 180, 182, 183
    },
    // Mode 2: Gamma 2.2
    {
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
          1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   2,   2,   2,   2,   2,   2,
          3,   3,   3,   3,   3,   3,   4,   4,   4,   4,   5,   5,   5,   5,   6,   6,
          6,   7,   7,   7,   8,   8,   8,   9,   9,   9,  10,  10,  11,  11,  11,  12,
         12,  13,  13,  14,  14,  15,  15,  16,  16,  17,  17,  18,  18,  19,  19,  20,
         21,  21,  22,  22,  23,  24,  24,  25,  26,  26,  27,  28,  28,  29,  30,  30,
         31,  32,  32,  33,  34,  34,  35,  36,  37,  37,  38,  39,  40,  40,  41,  42,
         43,  43,  44,  45,  46,  46,  47,  48,  49,  50,  50,  51,  52,  53,  54,  54,
         55,  56,  57,  58,  58,  59,  60,  61,  62,  63,  63,  64,  65,  66,  67,  68,
         68,  69,  70,  71,  72,  73,  74,  74,  75,  76,  77,  78,  79,  80,  81,  82,
         82,  83,  84,  85,  86,  87,  88,  89,  90,  91,  92,  92,  93,  94,  95,  96,
         97,  98,  99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112,
        113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128,
        129, 131, 132, 133, 134, 135, 136, 137, 138, 139, 141, 142, 143, 144, 145, 146,
        148, 149, 150, 151, 152, 154, 155, 156, 157, 159, 160, 161, 162, 164, 165, 166,
        168, 169, 170, 172, 173, 174, 176, 177, 178, 180, 181, 183, 184, 185, 187, 188
    },
    // Mode 3: Gamma 2.6
    {
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0,   1,   1,   1,   1,   1,   1,   1,   1,
          1,   1,   2,   2,   2,   2,   2,   2,   3,   3,   3,   3,   3,   4,   4,   4,
          5,   5,   5,   6,   6,   6,   7,   7,   8,   8,   8,   9,   9,  10,  10,  11,
         11,  12,  12,  13,  13,  14,  14,  15,  16,  16,  17,  18,  18,  19,  20,  20,
         21,  22,  22,  23,  24,  25,  25,  26,  27,  28,  28,  29,  30,  31,  31,  32,
         33,  34,  35,  35,  36,  37,  38,  39,  40,  40,  41,  42,  43,  44,  45,  46,
         47,  47,  48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,
         62,  63,  64,  65,  66,  67,  68,  69,  70,  71,  72,  73,  74,  75,  76,  77,
         78,  79,  81,  82,  83,  84,  85,  86,  87,  88,  90,  91,  92,  93,  94,  96,
         97,  98,  99, 101, 102, 103, 104, 106, 107, 108, 109, 111, 112, 113, 115, 116,
        117, 119, 120, 121, 123, 124, 125, 127, 128, 129, 131, 132, 133, 135, 136, 138,
        139, 140, 142, 143, 145, 146, 147, 149, 150, 152, 153, 155, 156, 158, 159, 161,
        162, 164, 165, 167, 168, 170, 171, 173, 174, 176, 177, 179, 180, 182, 183, 185,
        186, 188, 189, 191, 193, 194, 196, 197, 199, 201, 202, 204, 205, 207, 209, 210,
        212, 214, 215, 217, 219, 220, 222, 224, 225, 227, 229, 230, 232, 234, 236, 237
    },
    // Mode 4: Gamma 3.0
    {
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   1,
          1,   1,   1,   1,   1,   1,   1,   1,   1,   2,   2,   2,   2,   2,   2,   3,
          3,   3,   3,   4,   4,   4,   5,   5,   5,   6,   6,   7,   7,   8,   8,   9,
          9,  10,  10,  11,  12,  12,  13,  14,  14,  15,  16,  17,  17,  18,  19,  20,
         20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,  32,  33,  34,  35,
         36,  37,  38,  40,  41,  42,  43,  44,  46,  47,  48,  50,  51,  52,  54,  55,
         56,  58,  59,  61,  62,  64,  65,  67,  68,  70,  71,  73,  75,  76,  78,  79,
         81,  83,  84,  86,  88,  89,  91,  93,  94,  96,  98,  99, 101, 103, 105, 106,
        108, 110, 112, 114, 116, 117, 119, 121, 123, 125, 127, 129, 131, 133, 135, 137,
        139, 141, 143, 145, 147, 149, 151, 153, 155, 157, 159, 161, 163, 166, 168, 170,
        172, 174, 177, 179, 181, 183, 186, 188, 190, 193, 195, 197, 200, 202, 204, 207,
        209, 212, 214, 216, 219, 221, 224, 226, 229, 231, 234, 236, 239, 241, 244, 246,
        249, 251, 254, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255
    }
};

// give the sine LUT; this takes up 364 bytes
const float SIN_QONE_LUT[91] = {
    0.0000f, 0.0175f, 0.0349f, 0.0523f, 0.0698f, 0.0872f, 0.1045f, 0.1219f, 
    0.1392f, 0.1564f, 0.1736f, 0.1908f, 0.2079f, 0.2250f, 0.2419f, 0.2588f, 
    0.2756f, 0.2924f, 0.3090f, 0.3256f, 0.3420f, 0.3584f, 0.3746f, 0.3907f, 
    0.4067f, 0.4226f, 0.4384f, 0.4540f, 0.4695f, 0.4848f, 0.5000f, 0.5150f, 
    0.5299f, 0.5446f, 0.5592f, 0.5736f, 0.5878f, 0.6018f, 0.6157f, 0.6293f, 
    0.6428f, 0.6561f, 0.6691f, 0.6820f, 0.6947f, 0.7071f, 0.7193f, 0.7314f, 
    0.7431f, 0.7547f, 0.7660f, 0.7771f, 0.7880f, 0.7986f, 0.8090f, 0.8192f, 
    0.8290f, 0.8387f, 0.8480f, 0.8572f, 0.8660f, 0.8746f, 0.8829f, 0.8910f, 
    0.8988f, 0.9063f, 0.9135f, 0.9205f, 0.9272f, 0.9336f, 0.9397f, 0.9455f, 
    0.9511f, 0.9563f, 0.9613f, 0.9659f, 0.9703f, 0.9744f, 0.9781f, 0.9816f, 
    0.9848f, 0.9877f, 0.9903f, 0.9925f, 0.9945f, 0.9962f, 0.9976f, 0.9986f, 
    0.9994f, 0.9998f, 1.0000f
};

float get_sin (uint32_t degrees) {
    // normalize to 0-359 range; arguement is hue rotation, which is positive
    degrees = degrees % 360;

    // use symmetry of the unit circle and trig indentities to map all quadrants to the 0-90 range
    if (degrees <= 90) {
        return SIN_QONE_LUT[degrees];
    } 
    else if (degrees <= 180) {
        return SIN_QONE_LUT[180 - degrees];
    } 
    else if (degrees <= 270) {
        return -SIN_QONE_LUT[degrees - 180];
    } 
    else {
        return -SIN_QONE_LUT[360 - degrees];
    }
}
/*
These parameters can't be used in this desktop implementation of the ISP, because
they write directly to camera registers via I2C and "preprocess" parts of the data that we otherwise can't do. 

uint32_t brightness = 128;      // [0, 255]
uint32_t luminance_range = 16;  // [1, 32]
uint32_t step_size = 1;         // [1, 16]

// Manual White Balance (AWB) Initializations (12-bit registers 0x3400-0x3405)
uint32_t red_gain = 1024;      //  [0, 4095] ; [x000, xFFF]
uint32_t blue_gain = 1024;     //  [0, 4095] ; [x000, xFFF]

*/
const uint8_t* current_gamma_lut = GAMMA_LUTS[0];
float ccm[3][3];

const float red_weight = 0.299f;
const float green_weight = 0.587f;
const float blue_weight = 0.114f;      

const uint8_t WIDTH = 240;
const uint8_t HEIGHT = 240;

// Color Matrix (CMX) Initializations
uint32_t saturation = 128;     //  [0, 255]
uint32_t hue_rotation = 0;     //  [0, 360]
uint32_t contrast = 128;

// Gamma Correction & Effects Initializations
uint32_t gamma_mode = 2;        // [0, 4]
uint32_t night_mode = 0;        // [0, 1]
uint32_t banding_filter = 0;    // [0, 1]
uint32_t monochrome = 0;        // [0, 1]

float ccm[3][3];
float ccm_offset;

int main() {
    // allocate a buffer to put the entire raw data of our frame into memory
    // this is purely for storage that we'll read from to simulate DCMI streaming data in
    uint8_t* raw_frame = (uint8_t*) malloc(WIDTH * HEIGHT);

    const char* input_path = "test_images/test_image.raw";
    const char* output_path = "test_images/simulated_display.raw"; 

    FILE* f = fopen(input_path, "rb");

    if (f == NULL) {
        printf("Error: Could not find test_image.raw!\n");
        free(raw_frame);
        return 1;
    }

    size_t bytes_read = fread(raw_frame, 1, WIDTH * HEIGHT, f);
    fclose(f);

    if (bytes_read != (WIDTH * HEIGHT)) {
        printf("Error: File size does not match expected dimensions.\n");
        free(raw_frame); 
        return 1;
    }

    // create the 4 line buffer each containing a row for circular DMA
    uint8_t circular_buffer[4][WIDTH];

    // the display buffer; what we output final calculated values too
    uint16_t display_buffer[WIDTH];

    FILE* out_file = fopen(output_path, "wb");
    if (out_file == NULL) {
        printf("Error: Could not create output file!\n");
        free(raw_frame);
        return 1;
    }

    printf("Enter Saturation (0-255): ");
    scanf("%u", &saturation);
    saturation = (saturation > 255) ? 255 : saturation;

    printf("Enter Hue Rotation (0-360): ");
    scanf("%u", &hue_rotation);
    hue_rotation = (hue_rotation > 360) ? 360 : hue_rotation;

    printf("Enter Contrast (0-255): ");
    scanf("%u", &contrast);
    contrast = (contrast > 255) ? 255 : contrast;

    printf("Enter Gamma Mode (0-4): ");
    scanf("%u", &gamma_mode);

    printf("Enter Night Mode (0 or 1): ");
    scanf("%u", &night_mode);

    printf("Enter Banding Filter (0 or 1): ");
    scanf("%u", &banding_filter);

    printf("Enter Monochrome (0 or 1): ");
    scanf("%u", &monochrome);

    printf("\nConfiguration loaded. Starting ISP...\n");

    // Apply configuration before starting the processing loop
    update_ISP_settings();

    // DMA fills the first 3 rows before the CPU can do anything
    for (int i = 0; i < 3; i++) {
        for (int col = 0; col < WIDTH; col++) {
            circular_buffer[i][col] = raw_frame[(i * WIDTH) + col];
        }
    }
    
    // start at row 3, and go to HEIGHT (inclusive) to flush the final row
    for (int row = 3; row <= HEIGHT; row++) {
        int dma_write_idx = row % 4;        // where DMA is writing
        int top_read_idx  = (row - 3) % 4;  // oldest row
        int mid_read_idx  = (row - 2) % 4;  // center row (target)
        int bot_read_idx  = (row - 1) % 4;  // newest complete row

        // zero out the display buffer so our uncomputed edges default to black
        memset(display_buffer, 0, sizeof(display_buffer));

        // only try to read new data if we haven't hit the bottom of the image
        if (row < HEIGHT) {
            // DMA Fills the next line
            for (int col = 0; col < WIDTH; col++) {
                circular_buffer[dma_write_idx][col] = raw_frame[(row * WIDTH) + col];
            }
        }

        // determine if the middle row is a G-R or B-G row
        uint8_t is_gr_row = (((row - 2) & 1) == 0) ? 1 : 0;

        // we process this image line with our function
        process_image_line(
            circular_buffer[top_read_idx], 
            circular_buffer[mid_read_idx], 
            circular_buffer[bot_read_idx], 
            display_buffer, 
            is_gr_row
        );

        // --- EDGE PADDING / HARDWARE CHEAT ---
        if (row == 3) {
            // First valid row (Target 1). Output it twice to cover Row 0 and Row 1.
            fwrite(display_buffer, sizeof(uint16_t), WIDTH, out_file); 
        }
        
        // Output the currently computed row
        fwrite(display_buffer, sizeof(uint16_t), WIDTH, out_file);
    }

    // Loop ends after processing Target 238. 
    // The buffer still holds Target 238. Output it one last time to cover Row 239.
    fwrite(display_buffer, sizeof(uint16_t), WIDTH, out_file);

    fclose(out_file);
    free(raw_frame);
    printf("Simulation Complete!\n");
    return 0;
}

// since we output within this RGB 565 raw format, we need to use another
// script to write it out to a png if we want to visually test and confirm
// that our ISP design works.

float get_cos (uint32_t degrees) {
    // use trig indentitiy cos(x) = sin(x + 90)
    return get_sin(degrees + 90);
}

void process_image_line (const uint8_t* restrict top, 
                         const uint8_t* restrict mid, 
                         const uint8_t* restrict bot, 
                         uint16_t* restrict output, 
                         uint8_t is_gr_row) {
    
    // cache the gamma pointer locally
    const uint8_t* gamma = current_gamma_lut;
    /* GRBG Bayer              
        [G, R] G-R-G-R ...   
        [B, G] B-G-B-G ...  */
    // use conditioan logic outside the loop
    if (is_gr_row == 1) {
        
        // left boundary (i = 0) where the center is G
        float r_edge, g_edge, b_edge;
        uint32_t r_f, g_f, b_f;
        
        r_edge = (float) mid[1];
        g_edge = (float) mid[0];
        b_edge = (float) ((top[0] + bot[0]) >> 1);
        
        multiply_CCM(&r_edge, &g_edge, &b_edge);
        r_f = gamma[__USAT((int32_t)(r_edge + ccm_offset), 8)];
        g_f = gamma[__USAT((int32_t)(g_edge + ccm_offset), 8)];
        b_f = gamma[__USAT((int32_t)(b_edge + ccm_offset), 8)];
        output[0] = ((r_f & 0xF8) << 8) | ((g_f & 0xFC) << 3) | (b_f >> 3);

        // process column pairs (odd and even) in one pass
        for (int i = 1; i < WIDTH - 1; i += 2) {
            float r0, g0, b0; // odd column (center is r)
            float r1, g1, b1; // even column (center is g)

            // pixel i: center is r, corners are b, cross is g
            r0 = (float) mid[i];
            g0 = (float) ((mid[i - 1] + mid[i + 1] + top[i] + bot[i]) >> 2);
            b0 = (float) ((top[i - 1] + top[i + 1] + bot[i - 1] + bot[i + 1]) >> 2);

            // pixel i+1: center is g, left/right are r, top/bot are b
            // note: indexing shifts right by 1 for the next pixel
            r1 = (float) ((mid[i] + mid[i + 2]) >> 1);
            g1 = (float) mid[i + 1];
            b1 = (float) ((top[i + 1] + bot[i + 1]) >> 1);

            // apply the color correction matrix via pointer references
            multiply_CCM(&r0, &g0, &b0);
            multiply_CCM(&r1, &g1, &b1);

            // apply offset, 1-cycle branchless clamp to 255, and gamma correct pixel i
            // do this all in one line
            uint32_t r0_f = gamma[__USAT((int32_t) (r0 + ccm_offset), 8)];
            uint32_t g0_f = gamma[__USAT((int32_t) (g0 + ccm_offset), 8)];
            uint32_t b0_f = gamma[__USAT((int32_t) (b0 + ccm_offset), 8)];

            // pack and store pixel i
            output[i] = ((r0_f & 0xF8) << 8) | ((g0_f & 0xFC) << 3) | (b0_f >> 3);

            // apply offset, 1-cycle branchless clamp to 255, and gamma correct pixel i+1
            uint32_t r1_f = gamma[__USAT((int32_t) (r1 + ccm_offset), 8)];
            uint32_t g1_f = gamma[__USAT((int32_t) (g1 + ccm_offset), 8)];
            uint32_t b1_f = gamma[__USAT((int32_t) (b1 + ccm_offset), 8)];

            // pack and store pixel i+1
            output[i + 1] = ((r1_f & 0xF8) << 8) | ((g1_f & 0xFC) << 3) | (b1_f >> 3);
        }

        // right boundary (i = WIDTH - 1) where center is R
        r_edge = (float) mid[WIDTH - 1];                          
        g_edge = (float) ((top[WIDTH - 1] + bot[WIDTH - 1] + (mid[WIDTH - 2] << 1)) >> 2); 
        b_edge = (float) ((top[WIDTH - 2] + bot[WIDTH - 2]) >> 1); 
        
        multiply_CCM(&r_edge, &g_edge, &b_edge);
        r_f = gamma[__USAT((int32_t)(r_edge + ccm_offset), 8)];
        g_f = gamma[__USAT((int32_t)(g_edge + ccm_offset), 8)];
        b_f = gamma[__USAT((int32_t)(b_edge + ccm_offset), 8)];
        output[WIDTH - 1] = ((r_f & 0xF8) << 8) | ((g_f & 0xFC) << 3) | (b_f >> 3);

    } else {
        
        // left boundary (i = 0) where the center is B
        float r_edge, g_edge, b_edge;
        uint32_t r_f, g_f, b_f;
        
        r_edge = (float) ((top[0] + bot[0]) >> 1);                
        g_edge = (float) ((top[0] + bot[0] + (mid[1] << 1)) >> 2); 
        b_edge = (float) mid[0];                                  
        
        multiply_CCM(&r_edge, &g_edge, &b_edge);
        r_f = gamma[__USAT((int32_t)(r_edge + ccm_offset), 8)];
        g_f = gamma[__USAT((int32_t)(g_edge + ccm_offset), 8)];
        b_f = gamma[__USAT((int32_t)(b_edge + ccm_offset), 8)];
        output[0] = ((r_f & 0xF8) << 8) | ((g_f & 0xFC) << 3) | (b_f >> 3);

        // process column pairs (odd and even) in one pass
        for (int i = 1; i < WIDTH - 1; i += 2) {
            float r0, g0, b0; // odd column (center is g)
            float r1, g1, b1; // even column (center is b)

            // pixel i: center is g, left/right are b, top/bot are r
            r0 = (float) ((top[i] + bot[i]) >> 1);
            g0 = (float) mid[i];
            b0 = (float) ((mid[i - 1] + mid[i + 1]) >> 1);

            // pixel i+1: center is b, corners are r, cross is g
            // note: indexing shifts right by 1 for the next pixel
            r1 = (float) ((top[i] + top[i + 2] + bot[i] + bot[i + 2]) >> 2);
            g1 = (float) ((mid[i] + mid[i + 2] + top[i + 1] + bot[i + 1]) >> 2);
            b1 = (float) mid[i + 1];

            // apply the color correction matrix via pointer references
            multiply_CCM(&r0, &g0, &b0);
            multiply_CCM(&r1, &g1, &b1);

            // apply offset, 1-cycle branchless clamp to 255, and gamma correct pixel i
            uint32_t r0_f = gamma[__USAT((int32_t) (r0 + ccm_offset), 8)];
            uint32_t g0_f = gamma[__USAT((int32_t) (g0 + ccm_offset), 8)];
            uint32_t b0_f = gamma[__USAT((int32_t) (b0 + ccm_offset), 8)];

            // pack and store pixel i in output (display buffer)
            output[i] = ((r0_f & 0xF8) << 8) | ((g0_f & 0xFC) << 3) | (b0_f >> 3);

            // apply offset, 1-cycle branchless clamp to 255, and gamma correct pixel i+1
            uint32_t r1_f = gamma[__USAT((int32_t) (r1 + ccm_offset), 8)];
            uint32_t g1_f = gamma[__USAT((int32_t) (g1 + ccm_offset), 8)];
            uint32_t b1_f = gamma[__USAT((int32_t) (b1 + ccm_offset), 8)];

            // pack and store pixel i+1 in output (display buffer)
            output[i + 1] = ((r1_f & 0xF8) << 8) | ((g1_f & 0xFC) << 3) | (b1_f >> 3);        
        }

        // right boundary (i = WIDTH - 1) where center is G
        r_edge = (float) ((top[WIDTH - 1] + bot[WIDTH - 1]) >> 1); 
        g_edge = (float) mid[WIDTH - 1];                           
        b_edge = (float) mid[WIDTH - 2];                           
        
        multiply_CCM(&r_edge, &g_edge, &b_edge);
        r_f = gamma[__USAT((int32_t)(r_edge + ccm_offset), 8)];
        g_f = gamma[__USAT((int32_t)(g_edge + ccm_offset), 8)];
        b_f = gamma[__USAT((int32_t)(b_edge + ccm_offset), 8)];
        output[WIDTH - 1] = ((r_f & 0xF8) << 8) | ((g_f & 0xFC) << 3) | (b_f >> 3);
    }
}

void update_ISP_settings (void) {
    // set gamma_lut by updating the pointer 
    current_gamma_lut = GAMMA_LUTS[gamma_mode];
    
    // get sin(hue_rotation) and cos(hue_rotation)
    float cos_t = get_cos(hue_rotation);
    float sin_t = get_sin(hue_rotation);

    // declare these floats now and handle specific mode conditions right after
    float lum[3] = {red_weight, green_weight, blue_weight};
    float saturation_normalized = (float) saturation / 128.0f;
    float contrast_normalized = (float) contrast / 128.0f;
    float digital_gain = 1.0f;

    // if we're in monochrome mode, just use luminance calculation 
    if (monochrome) {
        saturation_normalized = 0.0f; 
    }

    if (night_mode) {
        digital_gain = 2.0f;
        contrast_normalized *= 0.8f;
    }

    // matrix generated with rotated values (paramtrized by the hue rotation)
    float rot[3][3] = {
        { red_weight + cos_t * (1.0f - red_weight) - sin_t * red_weight, 
          green_weight - cos_t * green_weight - sin_t * green_weight, 
          blue_weight - cos_t * blue_weight + sin_t * (1.0f - blue_weight) },

        { red_weight - cos_t * red_weight + sin_t * 0.168f, 
          green_weight + cos_t * (1.0f - green_weight) + sin_t * 0.330f, 
          blue_weight - cos_t * blue_weight - sin_t * 0.497f },

        { red_weight - cos_t * red_weight - sin_t * 0.328f, 
          green_weight - cos_t * green_weight + sin_t * 0.125f, 
          blue_weight + cos_t * (1.0f - blue_weight) + sin_t * blue_weight }
    };

    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            // blend between grayscale values and the distance from our post hue rotated values
            // creates that dulling effect or boosting effect based on saturation values
            // multiply by contrast and digital gain
            ccm[i][j] = digital_gain * contrast_normalized * (lum[j] + saturation_normalized * (rot[i][j] - lum[j]));
        }
    }
    // set ccm offset to be used in processing line
    ccm_offset = 128.0f * (1.0f - contrast_normalized);
}

void multiply_CCM (float* r, float* g, float* b) {
    float old_r = *r;
    float old_g = *g;
    float old_b = *b;

    // [r, g, b] = [old_r, old_g, old_b] x CCM
    *r = (ccm[0][0] * old_r) + (ccm[0][1] * old_g) + (ccm[0][2] * old_b);
    *g = (ccm[1][0] * old_r) + (ccm[1][1] * old_g) + (ccm[1][2] * old_b);
    *b = (ccm[2][0] * old_r) + (ccm[2][1] * old_g) + (ccm[2][2] * old_b);
}