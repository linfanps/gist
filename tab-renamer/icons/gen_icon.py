#!/usr/bin/env python3
"""生成 Tab Renamer 扩展图标：蓝底圆角方块 + 白色标签页 + 铅笔。

纯标准库：自己做超采样光栅化，再用 zlib 手写 PNG。
坐标系为 0..1 的单位正方形，y 向下。
"""
import math
import struct
import zlib
from pathlib import Path

BLUE = (26, 115, 232)   # #1a73e8，和扩展 UI 里的主色一致
WHITE = (255, 255, 255)

# ---------- 形状 ----------

def rrect(px, py, x0, y0, x1, y1, r):
    """圆角矩形。"""
    cx = min(max(px, x0 + r), x1 - r)
    cy = min(max(py, y0 + r), y1 - r)
    if x0 <= px <= x1 and y0 <= py <= y1:
        # 只有落在四个角的方形区域外时才需要判圆
        if (px < x0 + r or px > x1 - r) and (py < y0 + r or py > y1 - r):
            return math.hypot(px - cx, py - cy) <= r
        return True
    return False


def rrect_top(px, py, x0, y0, x1, y1, r):
    """只有上面两个角是圆的矩形——浏览器标签的形状。"""
    if not (x0 <= px <= x1 and y0 <= py <= y1):
        return False
    if py >= y0 + r:
        return True
    cx = min(max(px, x0 + r), x1 - r)
    return math.hypot(px - cx, py - (y0 + r)) <= r


# 铅笔：从笔尾 T 指向笔尖 P，靠近笔尖处线性收窄成锥形
PEN_T = (0.505, 0.885)
PEN_P = (0.905, 0.485)
PEN_W = 0.094
TAPER = 0.78          # t 超过这个比例开始收窄
FERRULE = (0.60, 0.685)  # 笔杆上那道金属箍的位置


def pencil(px, py, pad=0.0):
    tx, ty = PEN_T
    dx, dy = PEN_P[0] - tx, PEN_P[1] - ty
    length = math.hypot(dx, dy)
    ux, uy = dx / length, dy / length
    qx, qy = px - tx, py - ty
    t = (qx * ux + qy * uy) / length          # 沿轴归一化位置
    perp = abs(qx * -uy + qy * ux)            # 到轴线的距离

    if t < 0:
        return math.hypot(qx, qy) <= PEN_W + pad          # 笔尾圆头
    if t > 1:
        return math.hypot(px - PEN_P[0], py - PEN_P[1]) <= pad

    half = PEN_W if t <= TAPER else PEN_W * (1 - t) / (1 - TAPER)
    return perp <= half + pad


def pencil_axis_t(px, py):
    tx, ty = PEN_T
    dx, dy = PEN_P[0] - tx, PEN_P[1] - ty
    length = math.hypot(dx, dy)
    ux, uy = dx / length, dy / length
    return ((px - tx) * ux + (py - ty) * uy) / length


# ---------- 上色 ----------

def shade(px, py):
    """返回该点的 (r,g,b,a)，a 为 0/255。"""
    if not rrect(px, py, 0.0, 0.0, 1.0, 1.0, 0.215):
        return (0, 0, 0, 0)

    color = BLUE

    # 铅笔周围留一圈底色缝隙，让它和标签形状分开
    in_halo = pencil(px, py, pad=0.047)

    if not in_halo:
        # 标签页本身：宽而扁，只有上面两角是圆的
        tab = rrect_top(px, py, 0.115, 0.215, 0.675, 0.505, 0.075)
        # 下面的标签栏横线，横贯整个图标，让上面那块读作"一个标签"
        bar = rrect(px, py, 0.115, 0.505, 0.885, 0.590, 0.036)
        if tab or bar:
            color = WHITE
        # 标签里代表标题文字的那一横
        if rrect(px, py, 0.195, 0.315, 0.500, 0.395, 0.038):
            color = BLUE

    if pencil(px, py):
        color = WHITE
        t = pencil_axis_t(px, py)
        if FERRULE[0] <= t <= FERRULE[1]:
            color = BLUE

    return (*color, 255)


# ---------- 光栅化 + PNG ----------

def render(size, ss=6):
    """超采样后取平均，得到抗锯齿的 RGBA 像素。"""
    rows = []
    inv = 1.0 / (size * ss)
    for y in range(size):
        row = bytearray()
        for x in range(size):
            r = g = b = a = 0
            for sy in range(ss):
                py = (y * ss + sy + 0.5) * inv
                for sx in range(ss):
                    px = (x * ss + sx + 0.5) * inv
                    cr, cg, cb, ca = shade(px, py)
                    if ca:
                        r += cr; g += cg; b += cb; a += ca
            n = ss * ss
            if a == 0:
                row += bytes(4)
            else:
                cov = a // 255  # 有多少个子样本落在图形内
                row += bytes((r // cov, g // cov, b // cov, a // n))
        rows.append(bytes(row))
    return rows


def write_png(path, size, rows):
    raw = b''.join(b'\x00' + r for r in rows)  # 每行前面加 filter 字节 0

    def chunk(tag, data):
        c = tag + data
        return struct.pack('>I', len(data)) + c + struct.pack('>I', zlib.crc32(c))

    png = (b'\x89PNG\r\n\x1a\n'
           + chunk(b'IHDR', struct.pack('>IIBBBBB', size, size, 8, 6, 0, 0, 0))
           + chunk(b'IDAT', zlib.compress(raw, 9))
           + chunk(b'IEND', b''))
    Path(path).write_bytes(png)


if __name__ == '__main__':
    out = Path(__file__).resolve().parent
    out.mkdir(parents=True, exist_ok=True)
    for size in (16, 32, 48, 128):
        # 小尺寸多采样几次，边缘更干净
        ss = 12 if size <= 32 else 6
        write_png(out / f'icon{size}.png', size, render(size, ss))
        print('wrote', out / f'icon{size}.png')
