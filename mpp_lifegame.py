# -*- coding:utf-8; mode:python -*-
u'''
65536 個の 1bit PE を 256×256 の二次元結合した計算機のシミュレータで
ライフゲームを実行するサンプル。
'''
from mpp_chip import MPP, NewsRouter
from mpp_display import LEDPanel, WIDTH, HEIGHT
from life_file import read_life_105_file
import random

FLAG_ZERO = 0
FLAG_ROUTE_DATA = 63

class Controller:
    u'''PE にマイクロ命令を発行するコントローラ。
    '''
    def __init__(self, memory_size):
        u'''
        コントローラインスタンスを生成する。
        ----
        Args
            memory_size : 各 PE のメモリサイズ（ビット数）
        '''
        self.mpp = MPP(1024, memory_size)
        self.router = NewsRouter(self.mpp, WIDTH, HEIGHT)
        self.panel = LEDPanel()
        # ヒープ管理
        self.htop = 0
    def reset(self):
        self.mpp.reset()
    def to_led_r(self):
        u'''ルータデータフラグ上のデータを LED ディスプレイパネルの R に送信する。
        '''
        for y in range(0, HEIGHT):
            for x in range(0, WIDTH >> 6):
                data64 = self.router.read64_2d(x << 6, y)
                self.panel.set64_r(x << 6, y, data64)
    def to_led_g(self):
        u'''ルータデータフラグ上のデータを LED ディスプレイパネルの G に送信する。
        '''
        for y in range(0, HEIGHT):
            for x in range(0, WIDTH >> 6):
                data64 = self.router.read64_2d(x << 6, y)
                self.panel.set64_g(x << 6, y, data64)
    def to_led_b(self):
        u'''ルータデータフラグ上のデータを LED ディスプレイパネルの B に送信する。
        '''
        for y in range(0, HEIGHT):
            for x in range(0, WIDTH >> 6):
                data64 = self.router.read64_2d(x << 6, y)
                self.panel.set64_b(x << 6, y, data64)
    def led_update(self):
        u'''LED ディスプレイパネルの表示を更新する。
        '''
        self.panel.update_display()
    def clear_memory(self, start_address, end_address=None):
        u'''全 PE のメモリ start_address から end_address までを 0 クリアする。
        '''
        if end_address is None:
            end_address = start_address + 1
        for address in range(start_address, end_address):
            self.mpp.load_a(address,  FLAG_ZERO, 0x00)
            self.mpp.load_b(address, FLAG_ZERO, 0x00)
            self.mpp.store(FLAG_ZERO, False)
    def set_memory(self, start_address, end_address=None):
        u'''全 PE のメモリ start_address から end_address までを 1 埋めする。
        '''
        if end_address is None:
            end_address = start_address + 1
        for address in range(start_address, end_address):
            self.mpp.load_a(address, FLAG_ZERO, 0xff)
            self.mpp.load_b(address, FLAG_ZERO, 0x00)
            self.mpp.store(FLAG_ZERO, False)
    def send_memory(self, address):
        u'''全 PE のメモリ address にあるビットをルータに送信する
        '''
        self.mpp.load_a(address, FLAG_ZERO, 0xf0)
        self.mpp.load_b(address, FLAG_ZERO, 0xf0)
        self.mpp.store(FLAG_ROUTE_DATA, False)
    def recv_memory(self, address):
        u'''全 PE について、ルータからビットデータを受信して address に書き込む
        '''
        self.mpp.load_a(address, FLAG_ROUTE_DATA, 0xaa)
        self.mpp.load_b(address, FLAG_ZERO, 0x00)
        self.mpp.store(FLAG_ZERO, False)
    def log_and(self, addr, addr2):
        u'''全 PE について、AND を実行し addr を更新する。
        '''
        self.mpp.load_a(addr, FLAG_ZERO, 0xc0)
        self.mpp.load_b(addr2, FLAG_ZERO, 0xc0)
        self.mpp.store(FLAG_ZERO, False)
    def log_xor(self, addr, addr2):
        u'''全 PE について、XOR を実行し addr を更新する。
        '''
        self.mpp.load_a(addr, FLAG_ZERO, 0x3c)
        self.mpp.load_b(addr2, FLAG_ZERO, 0x3c)
        self.mpp.store(FLAG_ZERO, False)
    def copy_from_to(self, src_addr, dest_addr):
        u'''全 PE について、src_addr から dest_addr にコピーする。
        '''
        self.mpp.load_a(dest_addr, FLAG_ZERO, 0xcc)
        self.mpp.load_b(src_addr, FLAG_ZERO, 0x00)
        self.mpp.store(FLAG_ZERO, False)
    def set_if(self, addr, check_addr):
        u'''
        全 PE について、check_addr のアドレスに格納された値が 1 なら
        addr の値を 1 にする。addr の値がもともと 1 だったならそのままとする。
        '''
        self.mpp.load_a(addr, FLAG_ZERO, 0xfc)
        self.mpp.load_b(check_addr, FLAG_ZERO, 0x00)
        self.mpp.store(FLAG_ZERO, False)
    def copy_if(self, to_addr, from_addr, flag):
        u'''
        全 PE について、フラグの値が 1 ならば
        from_addr のアドレスに格納された値を to_addr のアドレスにコピーする。
        '''
        self.mpp.load_a(to_addr, flag, 0xd8)
        self.mpp.load_b(from_addr, FLAG_ZERO, 0x00)
        self.mpp.store(FLAG_ZERO, False)
    def count_flag(self, counter_start, counter_end, flag):
        u'''
        全 PE について、flag の値が 1 ならばビット 1 を増やす。
        counter_start は常に 1 で、カウントが進む度に
        counter_start + (カウント数) のアドレスのビットが 1 になる。
        '''
        for i in reversed(range(counter_start, counter_end - 1)):
            self.copy_if(i + 1, i, flag)
    def allocate_memory(self, size):
        u'''全PE上に size ビットのメモリを確保する。
        ----
        Return:
            確保した領域の [開始アドレス, 終了アドレス)。
        '''
        p = self.htop
        self.htop += size
        return p, self.htop
    def deallocate_all_memory(self):
        u'''全PE上に確保したメモリをすべて開放する。
        '''
        self.htop = 0
    def randomize(self, rate=0.5):
        u'''与えられた確率に従ってルータデータフラグの値をランダムに設定する。
        '''
        for y in range(0, HEIGHT):
            for x in range(0, WIDTH):
                self.router.unicast_2d(
                    x, y, 1 if random.random() <= rate else 0)
    def load_life(self, x0, y0, path):
        u'''与えられたファイルパスからライフゲームのパターンファイルを読み込む。
        読み込んだ内容は座標 (x0, y0) を起点としてルータデータフラグに反映する。
        反映後、R(赤) でパネルに表示する。
        '''
        for x, y in read_life_105_file(x0, y0, path):
            self.router.unicast_2d(x, y, 1)
        self.to_led_r()
        self.led_update()
        self.panel.clock_tick(1)
    def do_life(self):
        u'''ライフゲームを実行する。
        '''
        while True:
            #self.panel.clock_tick(30)
            self.life()
    def life(self):
        u'''ライフゲームのステップを実行する。
        '''
        # 周囲のビット 1 をカウントするための領域
        counter_start, counter_end = self.allocate_memory(10)
        # 現在のセルの状態を格納するアドレス
        current_cell = self.allocate_memory(1)[0]
        # 次のセルの状態を格納するアドレス
        next_cell = self.allocate_memory(1)[0]
        # カウンタ領域をクリアしておく
        self.clear_memory(counter_start, counter_end)
        # 初期状態はカウンタ=0
        self.set_memory(counter_start)
        # 現在のセル状態をルータデータフラグから取り込んでおく
        self.recv_memory(current_cell)
        # ルータデータフラグにある現在のセル状態を青色で表示
        self.to_led_b()
        #self.led_update()
        # ルータはセル状態を S-E-N-N-W-W-S-S と順に回す。
        # 各ノードは回ってきたセル状態が 1 だった回数をカウントする
        self.router.rotate_s()
        self.count_flag(counter_start, counter_start + 2, FLAG_ROUTE_DATA)
        self.router.rotate_e()
        self.count_flag(counter_start, counter_start + 3, FLAG_ROUTE_DATA)
        self.router.rotate_n()
        self.count_flag(counter_start, counter_start + 4, FLAG_ROUTE_DATA)
        self.router.rotate_n()
        self.count_flag(counter_start, counter_start + 5, FLAG_ROUTE_DATA)
        self.router.rotate_w()
        self.count_flag(counter_start, counter_start + 6, FLAG_ROUTE_DATA)
        self.router.rotate_w()
        self.count_flag(counter_start, counter_start + 7, FLAG_ROUTE_DATA)
        self.router.rotate_s()
        self.count_flag(counter_start, counter_start + 8, FLAG_ROUTE_DATA)
        self.router.rotate_s()
        self.count_flag(counter_start, counter_end, FLAG_ROUTE_DATA)
        # カウント結果は 1 の並びなので、XOR でカウント位置のみ 1 にする
        for i in range(counter_start, counter_end - 1):
            self.log_xor(i, i + 1)
        # 元の状態が「生」でカウント値が 2 なら「生」
        self.copy_from_to(current_cell, next_cell)
        self.set_if(next_cell, current_cell)
        self.log_and(next_cell, 2)
        # カウント値が 3 なら無条件に「生」
        self.set_if(next_cell, 3)
        # 次状態を決定できたので状態を更新し、ルータデータフラグへ送信する
        self.copy_from_to(next_cell, current_cell)
        self.send_memory(current_cell)
        # ルータデータフラグにある新しい状態を赤色で表示する
        self.to_led_r()
        self.led_update()
        self.deallocate_all_memory()
    def test_line(self):
        u'''ルータテスト用。
        画面全体に斜めの線を描く。
        '''
        for i in range(0, 256):
            self.router.unicast_2d(i, i, 1)
            self.router.unicast_2d(WIDTH-i, i, 1)
        self.to_led_r()
        self.led_update()
    def test_n(self):
        u'''ルータテスト用。100 回 N 方向に移動する。
        '''
        for i in range(0, 100):
            self.router.rotate_n()
            self.to_led_r()
            self.led_update()
    def test_e(self):
        u'''ルータテスト用。100 回 E 方向に移動する。
        '''
        for i in range(0, 100):
            self.router.rotate_e()
            self.to_led_r()
            self.led_update()
    def test_w(self):
        u'''ルータテスト用。100 回 W 方向に移動する。
        '''
        for i in range(0, 100):
            self.router.rotate_w()
            self.to_led_r()
            self.led_update()
    def test_s(self):
        u'''ルータテスト用。100 回 S 方向に移動する。
        '''
        for i in range(0, 100):
            self.router.rotate_s()
            self.to_led_r()
            self.led_update()
if __name__ == '__main__':
    import sys
    c = Controller(100)
    if len(sys.argv) < 2:
        c.randomize()
    else:
        c.load_life(128, 128, sys.argv[1])
    c.do_life()
