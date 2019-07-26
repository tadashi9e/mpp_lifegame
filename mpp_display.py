# -*- coding:utf-8; mode:python -*-
import threading

WIDTH=256
HEIGHT=256
WIDTH_BITS=8

def pos2d_of_pid(pid):
    x = (pid & ((1 << WIDTH_BITS) - 1))
    y = (pid >> WIDTH_BITS)
    return (x, y)
def pid_of_pos2d(x, y):
    return x | (y << WIDTH_BITS )
def p64id_of_pos2d(x, y):
    return pid_of_pos2d(x, y) >> 6

import pygame
from pygame.locals import DOUBLEBUF
from PIL import Image
from PIL import ImageDraw

class LEDPanel:
    def __init__(self, cell_size=2):
        self._lock = threading.Lock()
        self._updating = False
        self._dirty = False
        self._r = [0] * 1024
        self._g = [0] * 1024
        self._b = [0] * 1024
        self._prev_r = [0] * 1024
        self._prev_g = [0] * 1024
        self._prev_b = [0] * 1024
        self._cell_size = cell_size
        self._width = WIDTH * cell_size
        self._height = HEIGHT * cell_size
        self._bg_color = (0, 0, 0)
        self._screen = pygame.display.set_mode(
            (self._width, self._height), DOUBLEBUF)
        self._screen.set_alpha(None)
        pygame.display.set_caption('MPP simulator')
        self.generate_images = None
        self.clock = pygame.time.Clock()
    def set64_r(self, x, y, f64):
        u'''R 情報を更新する。同時に dirty=True にする。
        '''
        p64id = p64id_of_pos2d(x, y)
        self._lock.acquire()
        try:
            self._r[p64id] = f64
            self._dirty = True
        finally:
            self._lock.release()
    def set64_g(self, x, y, f64):
        u'''G 情報を更新する。同時に dirty=True にする。
        '''
        p64id = p64id_of_pos2d(x, y)
        self._lock.acquire()
        try:
            self._g[p64id] = f64
            self._dirty = True
        finally:
            self._lock.release()
    def set64_b(self, x, y, f64):
        u'''B 情報を更新する。同時に dirty=True にする。
        '''
        p64id = p64id_of_pos2d(x, y)
        self._lock.acquire()
        try:
            self._b[p64id] = f64
            self._dirty = True
        finally:
            self._lock.release()
    def start_generate_images(self):
        self.generate_images = []
    def update_display(self):
        u'''描画スレッドが動作中でないなら描画スレッドを起動する。
        描画によって dirty=False になる。
        '''
        self._lock.acquire()
        try:
            if not self._updating:
                self._updating = True
                self._dirty = False
                threading.Thread(target=self._update_display).start()
        finally:
            self._lock.release()
    def _update_display(self):
        u'''描画処理本体。
        描画終了時点で dirty=True だったら再描画を行う。
        '''
        pid = 0
        if self.generate_images is not None:
            if len(self.generate_images) > 0:
                image = self.generate_images[
                    len(self.generate_images) - 1].copy()
            else:
                image = Image.new('RGB', (self._width, self._height), (0,0,0))
            draw = ImageDraw.Draw(image)
        for index in range(0, len(self._r)):
            pr = self._prev_r[index]
            pg = self._prev_g[index]
            pb = self._prev_b[index]
            self._lock.acquire()
            try:
                r = self._r[index]
                g = self._g[index]
                b = self._b[index]
            finally:
                self._lock.release()
            if pr == r and pg == g and pb == b:
                pid += 64
                continue
            for i in range(0, 64):
                mask = 1 << i
                pcolor = (255 if (pr & mask) != 0 else 0,
                          255 if (pg & mask) != 0 else 0,
                          255 if (pb & mask) != 0 else 0)
                color = (255 if (r & mask) != 0 else 0,
                         255 if (g & mask) != 0 else 0,
                         255 if (b & mask) != 0 else 0)
                if pcolor == color:
                    pid += 1
                    continue
                (x, y) = pos2d_of_pid(pid)
                (px, py) = (x * self._cell_size, y * self._cell_size)
                rect = pygame.Rect(px, py, self._cell_size, self._cell_size)
                pygame.draw.rect(
                    self._screen, color,
                    rect)
                if self.generate_images is not None:
                    draw.rectangle((px, py,
                                    px + self._cell_size,
                                    py + self._cell_size),
                                    fill=color)

                pid += 1
            self._prev_r[index] = r
            self._prev_g[index] = g
            self._prev_b[index] = b
        pygame.display.flip()
        if self.generate_images is not None:
            self.generate_images.append(image)
        # dirty なら再描画する
        if self._dirty:
            threading.Thread(target=self._update_display).start()
        else:
            self._updating = False
    def clock_tick(self, fps):
        u'''与えられたフレームレートに従って待つ。
        Args:
            fps : int : フレームレート
        '''
        self.clock.tick(fps)
    def generate_animation_gif(self, gen_file_path, **kwargs):
        u'''与えられたファイルパス名でアニメーション GIF を作る。
        Args:
            gen_file_path : str : アニメーション GIF ファイル作成先パス名
        '''
        self.generate_images[0].save(
            gen_file_path,
            save_all=True, append_images=self.generate_images[1:],
            **kwargs)
