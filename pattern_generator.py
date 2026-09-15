import numpy as np

WIDTH, HEIGHT = 240, 240

# Use float32 for accurate gradient math, we will cast to uint8 later
canvas = np.zeros((HEIGHT, WIDTH, 3), dtype=np.float32)

# standard color bars, forms the top half
# tests: hue rotation, saturation, monochrome
bar_width = WIDTH // 6
colors = [
    [255, 0, 0],     # red
    [255, 255, 0],   # yellow
    [0, 255, 0],     # green
    [0, 255, 255],   # cyan
    [0, 0, 255],     # blue
    [255, 0, 255]    # magenta
]
for i, color in enumerate(colors):
    canvas[0:120, i * bar_width:(i + 1)*bar_width] = color

# create grayscale gradient
# Tests: contrast, banding filter
# creates a 1D array from 0 to 255, broadcasting it across all rows in this quadrant
gradient = np.linspace(0, 255, WIDTH // 2)
for i in range(3): # Apply equally to R, G, B channels
    canvas[120:240, 0:120, i] = gradient

# create low-light gradient
# Tests: night mode (gain), banding filter
dark_gradient = np.linspace(0, 40, WIDTH // 2)
for i in range(3):
    canvas[120:240, 120:240, i] = dark_gradient

# cast the final RGB canvas back to uint8 for packing
canvas = np.clip(canvas, 0, 255).astype(np.uint8)

# bayer encode
bayer = np.zeros((HEIGHT, WIDTH), dtype=np.uint8)

# encode into GRBG
for y in range(HEIGHT):
    for x in range(WIDTH):
        if y % 2 == 0:
            # even row: G-R-G-R pattern
            if x % 2 == 0:
                bayer[y, x] = canvas[y, x, 1] # Green
            else:
                bayer[y, x] = canvas[y, x, 0] # Red
        else:
            # odd row: B-G-B-G pattern
            if x % 2 == 0:
                bayer[y, x] = canvas[y, x, 2] # Blue
            else:
                bayer[y, x] = canvas[y, x, 1] # Green

# save as raw binary bytes
with open("test_images/test_image.raw", "wb") as f:
    f.write(bayer.tobytes())

print("Generated test_image.raw successfully!")