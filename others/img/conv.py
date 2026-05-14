#!/usr/bin/env python3
"""
Конвертер PNG -> RGB565 raw для загрузчика MyDuck.
Запрашивает у пользователя файл, размеры и выходной файл.
"""

import sys
import os
from PIL import Image

def rgb565(r, g, b):
    """Конвертирует 8-битные RGB в 16-битное RGB565 (little-endian порядок)."""
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | ((b & 0xF8) >> 3)

def convert_png_to_rgb565(input_path, output_path, width, height):
    """Загружает PNG, изменяет размер, переводит в RGB565 и сохраняет как raw."""
    try:
        img = Image.open(input_path).convert('RGB')
    except Exception as e:
        print(f"Ошибка открытия файла: {e}")
        return False

    img = img.resize((width, height), Image.Resampling.LANCZOS)

    data = bytearray()
    for y in range(height):
        for x in range(width):
            r, g, b = img.getpixel((x, y))
            pixel = rgb565(r, g, b)
            # Сохраняем в little-endian: сначала младший байт
            data.append(pixel & 0xFF)
            data.append((pixel >> 8) & 0xFF)

    with open(output_path, 'wb') as f:
        f.write(data)

    print(f"✅ {output_path} (размер {width}x{height}, {len(data)} байт).")
    return True

def main():
    print("Конвертер PNG -> .img (RGB565) для загрузчика MyDuck\n")

    # 1. Входной файл
    input_file = input("📁 Путь к PNG файлу: ").strip()
    if not os.path.exists(input_file):
        print("❌ Файл не найден.")
        return

    # 2. Размеры
    try:
        width = int(input("📐 Ширина (например, 80 для лого, 64 для обложки): "))
        height = int(input("📐 Высота: "))
    except ValueError:
        print("❌ Некорректный размер.")
        return

    # 3. Выходной файл (по умолчанию имя от входного)
    default_out = os.path.splitext(input_file)[0] + '.img'
    output_file = input(f"💾 Выходной файл (Enter → {default_out}): ").strip()
    if not output_file:
        output_file = default_out

    # Конвертация
    convert_png_to_rgb565(input_file, output_file, width, height)

if __name__ == "__main__":
    main()