import sys
import math
import random
import numpy as np
from PIL import Image

class Pixel:
    def __init__(self, r, g, b):
        self.r = r
        self.g = g
        self.b = b

def read_img(filename):
    image = Image.open(filename)
    pixels = list(image.getdata())
    global w, h 
    w, h = image.size[0], image.size[1]
    print(image.size)
    return [[Pixel(r, g, b) for (r, g, b) in pixels[i * w:(i + 1) * w]] for i in range(h)]

def write_img(image, filename):
    pixels = [tuple([pixel.r, pixel.g, pixel.b]) for row in image for pixel in row]
    img = Image.new("RGB", (w, h))
    img.putdata(pixels)
    img.save(filename, quality=100)
    

def compute_displacement_field(amp, sigma):
    d_x = [[0.0 for _ in range(w)] for _ in range(h)]
    d_y = [[0.0 for _ in range(w)] for _ in range(h)]

    da_x = [[0.0 for _ in range(w)] for _ in range(h)]
    da_y = [[0.0 for _ in range(w)] for _ in range(h)]

    kws = int(2.0 * sigma)
    ker = [math.exp(-float(k * k) / (sigma * sigma)) for k in range(-kws, kws + 1)]

    for i in range(h):
        for j in range(w):
            d_x[i][j] = -1.0 + 2.0 * random.random()
            d_y[i][j] = -1.0 + 2.0 * random.random()

    for i in range(h):
        for j in range(w):
            sum_x = 0.0
            sum_y = 0.0
            for k in range(-kws, kws + 1):
                v = j + k
                if v < 0:
                    v = -v
                if v >= w:
                    v = 2 * w - v - 1
                sum_x += d_x[i][v] * ker[abs(k)]
                sum_y += d_y[i][v] * ker[abs(k)]
            da_x[i][j] = sum_x
            da_y[i][j] = sum_y

    for j in range(w):
        for i in range(h):
            sum_x = 0.0
            sum_y = 0.0
            for k in range(-kws, kws + 1):
                u = i + k
                if u < 0:
                    u = -u
                if u >= h:
                    u = 2 * h - u - 1
                sum_x += da_x[u][j] * ker[abs(k)]
                sum_y += da_y[u][j] * ker[abs(k)]
            d_x[i][j] = sum_x
            d_y[i][j] = sum_y

    avg = sum(math.sqrt(d_x[i][j] ** 2 + d_y[i][j] ** 2) for i in range(h) for j in range(w)) / (h * w)

    for i in range(h):
        for j in range(w):
            d_x[i][j] = amp * d_x[i][j] / avg
            d_y[i][j] = amp * d_y[i][j] / avg

    return d_x, d_y

def apply_displacement_field(input, d_x, d_y):
    output = [[Pixel(0, 0, 0) for _ in range(w)] for _ in range(h)]

    for i in range(h):
        for j in range(w):
            p1 = i + d_y[i][j]
            p2 = j + d_x[i][j]

            u0 = int(math.floor(p1))
            v0 = int(math.floor(p2))

            f1 = p1 - u0
            f2 = p2 - v0

            sumr, sumg, sumb = 0.0, 0.0, 0.0
            for idx in range(4):
                if idx == 0:
                    u, v = u0, v0
                    f = (1.0 - f1) * (1.0 - f2)
                elif idx == 1:
                    u, v = u0 + 1, v0
                    f = f1 * (1.0 - f2)
                elif idx == 2:
                    u, v = u0, v0 + 1
                    f = (1.0 - f1) * f2
                else:
                    u, v = u0 + 1, v0 + 1
                    f = f1 * f2

                u = max(0, min(u, h - 1))
                v = max(0, min(v, w - 1))

                val = input[u][v].r
                sumr += f * val

                val = input[u][v].g
                sumg += f * val

                val = input[u][v].b
                sumb += f * val

            output[i][j].r = int(sumr)
            output[i][j].g = int(sumg)
            output[i][j].b = int(sumb)

    return output

def rubbersheet(input, amp, sigma):
    if sigma > h / 2.5 or sigma > w / 2.5:
        sys.stderr.write("- Warning: Gaussian smoothing kernel too large for the input image.\n")
        return

    if sigma < 1E-5:
        sys.stderr.write("- Warning: Gaussian smoothing kernel with negative/zero spread.\n")
        return

    d_x, d_y = compute_displacement_field(amp, sigma)
    output = apply_displacement_field(input, d_x, d_y)

    return output

if __name__ == "__main__":
    # Process command line arguments
    # Example run: python3 imagemorph.py test.jpg 2.0 50.0 output.jpg
    if len(sys.argv) != 5:
        sys.stderr.write(f"Program usage: {sys.argv[0]} [path_to_image] [displacement] [smoothing radius] [output_file]\n")
        sys.exit(1)

    path_to_image = sys.argv[1]
    amp = float(sys.argv[2])
    sigma = float(sys.argv[3])
    output_file = sys.argv[4]

    input_image = read_img(path_to_image)
    output = rubbersheet(input_image, amp, sigma)
    write_img(output, 'output.jpg')

    print(f'Output image saved to {output_file}')
