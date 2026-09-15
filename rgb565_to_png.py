import numpy as np
from PIL import Image

WIDTH, HEIGHT = 240, 240

# read the raw file (The C code output)
with open("test_images/simulated_display.raw", "rb") as f:
    raw_bytes = f.read()

# convert raw bytes into 16-bit integers (RGB565)
pixels_16 = np.frombuffer(raw_bytes, dtype=np.uint16).reshape((HEIGHT, WIDTH))

# unpack the RGB565 to standard 8-bit to write out as a png or whatever
# R: 5 bits, G: 6 bits, B: 5 bits
# idc about efficiency just get it
img_array = np.zeros((HEIGHT, WIDTH, 3), dtype=np.uint8)
img_array[:, :, 0] = ((pixels_16 >> 11) & 0x1F) * 255 // 31
img_array[:, :, 1] = ((pixels_16 >> 5)  & 0x3F) * 255 // 63
img_array[:, :, 2] = (pixels_16         & 0x1F) * 255 // 31

# save as png
img = Image.fromarray(img_array)
img.save("test_images/final_result.png")
print("Successfully converted raw data to test_images/final_result.png!")