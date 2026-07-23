import os
from PIL import Image

frames_dir = r'anim_frames'
out_h = r'anim_frames.h'
out_c = r'anim_frames.c'

frames = sorted([f for f in os.listdir(frames_dir) if f.endswith('.png')])
print(f'Found {len(frames)} frames')

with open(out_h, 'w') as h, open(out_c, 'w') as c:
    h.write('#ifndef ANIM_FRAMES_H\n')
    h.write('#define ANIM_FRAMES_H\n')
    h.write('#include "lvgl.h"\n\n')
    h.write('#define ANIM_FRAME_COUNT %d\n' % len(frames))
    h.write('extern const lv_image_dsc_t anim_frames[%d];\n' % len(frames))
    h.write('extern const uint8_t *anim_frame_data[%d];\n' % len(frames))
    h.write('#endif\n')

    c.write('#include "anim_frames.h"\n\n')

    for idx, fname in enumerate(frames):
        img = Image.open(os.path.join(frames_dir, fname)).convert('RGB')
        w, h_img = img.size
        pixels = list(img.getdata())

        # Convert to RGB565 little-endian bytes
        data = bytearray()
        for r, g, b in pixels:
            val = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)
            data.append(val & 0xFF)        # low byte
            data.append((val >> 8) & 0xFF)  # high byte

        c.write('static const uint8_t frame_%d_data[%d] = {\n' % (idx, len(data)))
        for i in range(0, len(data), 16):
            chunk = data[i:i+16]
            c.write('    ' + ', '.join('0x%02X' % b for b in chunk) + ',\n')
        c.write('};\n\n')

    c.write('const uint8_t *anim_frame_data[%d] = {\n' % len(frames))
    for idx in range(len(frames)):
        c.write('    frame_%d_data,\n' % idx)
    c.write('};\n')

    c.write('\nconst lv_image_dsc_t anim_frames[%d] = {\n' % len(frames))
    for idx, fname in enumerate(frames):
        img = Image.open(os.path.join(frames_dir, fname))
        w, h_img = img.size
        data_size = w * h_img * 2
        c.write('    { .header = { .cf = LV_COLOR_FORMAT_RGB565, .w = %d, .h = %d }, .data_size = %d, .data = frame_%d_data },\n' % (w, h_img, data_size, idx))
    c.write('};\n')

for f in [out_h, out_c]:
    size = os.path.getsize(f)
    print('%s: %.1f KB' % (f, size / 1024.0))
