import os
from PIL import Image

D = "assets/skin/graphite/controls"
names = [
    "transport_prev_idle", "transport_play_idle", "transport_pause_idle",
    "transport_stop_idle", "transport_next_idle",
    "transport_prev_pressed", "transport_next_pressed",
    "shuffle_idle", "shuffle_active", "repeat_idle", "repeat_active",
    "eq_idle", "eq_active", "pl_idle", "pl_active",
]
imgs = [(n, Image.open(os.path.join(D, n + ".png")).convert("RGBA")) for n in names]
pad = 12
cols = 5
cellw = max(i.width for _, i in imgs) + pad
cellh = max(i.height for _, i in imgs) + pad + 14
rows = (len(imgs) + cols - 1) // cols
sheet = Image.new("RGBA", (cols * cellw, rows * cellh), (40, 44, 52, 255))
from PIL import ImageDraw
d = ImageDraw.Draw(sheet)
for idx, (n, im) in enumerate(imgs):
    r, c = divmod(idx, cols)
    x = c * cellw + pad // 2
    y = r * cellh + 14
    sheet.alpha_composite(im, (x, y))
    d.text((x, r * cellh + 2), n, fill=(230, 230, 230, 255))
sheet.save(".scratch/graphite-skin/_probe/controls_sheet.png")
print("wrote sheet", sheet.size)
