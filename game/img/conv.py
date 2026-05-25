#!/usr/bin/env python3
"""
Конвертер PNG -> RGB565 raw (big-endian) для TFT_eSPI.
Обрабатывает все PNG в текущей папке.
"""

import os
from PIL import Image

def rgb565(r, g, b):
    """Конвертация 8-битных RGB в 16-битный RGB565 (big-endian)."""
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | ((b & 0xF8) >> 3)

def convert_png_to_img(png_path):
    try:
        img = Image.open(png_path).convert('RGB')
    except Exception as e:
        print(f"Ошибка открытия {png_path}: {e}")
        return False

    w, h = img.size
    data = bytearray()
    for y in range(h):
        for x in range(w):
            r, g, b = img.getpixel((x, y))
            pixel = rgb565(r, g, b)
            # big-endian: старший байт первым
            data.append((pixel >> 8) & 0xFF)
            data.append(pixel & 0xFF)

    out_path = os.path.splitext(png_path)[0] + '.img'
    with open(out_path, 'wb') as f:
        f.write(data)

    print(f"Конвертирован: {png_path} -> {out_path} ({w}x{h}, {len(data)} байт)")
    return True

def main():
    png_files = [f for f in os.listdir('.') if f.lower().endswith('.png')]
    if not png_files:
        print("Не найдено PNG файлов в текущей папке.")
        return
    print(f"Найдено {len(png_files)} PNG файлов. Конвертация...")
    for png in png_files:
        convert_png_to_img(png)
    print("Готово.")

if __name__ == "__main__":
    main()