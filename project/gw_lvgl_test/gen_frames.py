import os, cv2
from PIL import Image

video_path = r'C:\SW code\source code\ITE_LVGL_auto\NMGW2601_Circuit_Signal_Animation.mp4'
out_dir = 'anim_frames'

# Extract 5 frames at 240x240
TARGET_W, TARGET_H = 240, 240
os.makedirs(out_dir, exist_ok=True)
cap = cv2.VideoCapture(video_path)
for i in range(5):
    cap.set(cv2.CAP_PROP_POS_FRAMES, i * 24)
    ret, frame = cap.read()
    if ret:
        rgb = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
        # IT2D will hardware-scale to 480x480 on display
        small = cv2.resize(rgb, (TARGET_W, TARGET_H))
        bgr = cv2.cvtColor(small, cv2.COLOR_RGB2BGR)
        cv2.imwrite(os.path.join(out_dir, 'frame_%d.png' % i), bgr)
cap.release()
print('Extracted 5 frames at %dx%d' % (TARGET_W, TARGET_H))

# Generate C array per frame
pngs = sorted([f for f in os.listdir(out_dir) if f.endswith('.png')])
for idx, fname in enumerate(pngs):
    img = Image.open(os.path.join(out_dir, fname)).convert('RGB')
    w, h = img.size
    pixels = list(img.getdata())
    data = bytearray()
    for r, g, b in pixels:
        val = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)
        data.append(val & 0xFF)
        data.append((val >> 8) & 0xFF)

    cname = 'anim_frame_%d.c' % idx
    with open(cname, 'w') as f:
        f.write('#include "lvgl.h"\n')
        f.write('#include "anim_frames.h"\n\n')
        f.write('const uint8_t anim_frame_%d_data[%d] = {\n' % (idx, len(data)))
        for i in range(0, len(data), 16):
            chunk = data[i:i+16]
            f.write('    ' + ', '.join('0x%02X' % b for b in chunk) + ',\n')
        f.write('};\n\n')
        f.write('const lv_image_dsc_t anim_frame_%d = {\n' % idx)
        f.write('    .header = { .cf = LV_COLOR_FORMAT_RGB565, .w = %d, .h = %d },\n' % (w, h))
        f.write('    .data_size = %d,\n' % len(data))
        f.write('    .data = anim_frame_%d_data,\n' % idx)
        f.write('};\n')
    print('  %s: %.0f KB' % (cname, os.path.getsize(cname) / 1024.0))

# Generate header
header = 'anim_frames.h'
with open(header, 'w') as f:
    f.write('#ifndef ANIM_FRAMES_H\n')
    f.write('#define ANIM_FRAMES_H\n')
    f.write('#include "lvgl.h"\n\n')
    f.write('#define ANIM_FRAME_COUNT %d\n' % len(pngs))
    for idx in range(len(pngs)):
        f.write('extern const lv_image_dsc_t anim_frame_%d;\n' % idx)
    f.write('\nextern const lv_image_dsc_t* anim_frames[%d];\n' % len(pngs))
    f.write('#endif\n')

# Generate frame array
with open('anim_frames.c', 'w') as f:
    f.write('#include "anim_frames.h"\n\n')
    f.write('const lv_image_dsc_t* anim_frames[%d] = {\n' % len(pngs))
    for idx in range(len(pngs)):
        f.write('    &anim_frame_%d,\n' % idx)
    f.write('};\n')

print('Done: %s, %s' % (header, 'anim_frames.c'))
