# -*- coding:utf-8; mode:python -*-
u'''
mpp_chip_cc のラッパー
'''
import mpp_chip_cc

class MPP(object):
    u'''全 PE についてマイクロ命令を呼び出す。
    '''
    def __init__(self, memory_size):
        u'''
        '''
        self._mpp = mpp_chip_cc.newMPP(memory_size)
    def reset(self):
        u'''フラグの値をリセットする
        '''
        mpp_chip_cc.MPP_reset(self._mpp)
    def load_a(self, addr_a, read_flag, op_s):
        u'''LOADA マイクロ命令
        ----
        Args
            addr_a : A レジスタロードアドレス
            read_flag : 参照フラグインデックス
            op_s : S 真理値表
        '''
        mpp_chip_cc.MPP_load_a(self._mpp, addr_a, read_flag, op_s)
    def load_b(self, addr_b, context_flag, op_c):
        u'''LOADB マイクロ命令
        ----
        Args
            addr_b : B レジスタロードアドレス
            context_flag : コンテキストフラグインデックス
            op_c : C 真理値表
        '''
        mpp_chip_cc.MPP_load_b(self._mpp, addr_b, context_flag, op_c)
    def store(self, write_flag, context_value):
        u'''STORE マイクロ命令
        ----
        Args
            write_flag : 書き込み先フラグインデックス
            context_value : コンテキストフラグが 1 で有効か 0 で有効か示す
        '''
        mpp_chip_cc.MPP_store(self._mpp, write_flag, context_value)
    def recv(self, chip_no, value):
        u'''RECV マイクロ命令: 64 個の PE に対して値の受け取りを通知する。
        ----
        Args
            chip_no : 受信通知先 PE チップ
            value : 64 個の PE が受け取る値
        '''
        mpp_chip_cc.MPP_recv(self._mpp, chip_no, value)
    def send(self, chip_no):
        u'''SEND マイクロ命令: 64 個の PE に対して値の送信を促す。
        ----
        Args
            chip_no 送信を促す PE チップの番号。
        Return
            PE チップ上にある 64 個の PE が送信する値
        '''
        return mpp_chip_cc.MPP_send(self._mpp, chip_no)
class NewsRouter(object):
    u'''東西南北方向に配送を行うルータ
    '''
    def __init__(self, mpp):
        self._router = mpp_chip_cc.newRouter(mpp._mpp)
    def rotate_n(self):
        u'''N(北) 方向にルータフラグデータをローテートする。
        '''
        mpp_chip_cc.Router_news_n(self._router)
    def rotate_e(self):
        u'''E(東) 方向にルータフラグデータをローテートする。
        '''
        mpp_chip_cc.Router_news_e(self._router)
    def rotate_w(self):
        u'''W(西) 方向にルータフラグデータをローテートする。
        '''
        mpp_chip_cc.Router_news_w(self._router)
    def rotate_s(self):
        u'''S(南) 方向にルータフラグデータをローテートする。
        '''
        mpp_chip_cc.Router_news_s(self._router)
    def unicast_2d(self, x, y, b):
        u'''与えられた x, y 座標に相当する PE にビットデータを配送する。
        '''
        mpp_chip_cc.Router_unicast_2d(self._router, x, y, b)
    def read64_2d(self, x, y):
        u'''与えられた x, y 座標に相当する PE が属する PE チップにある
        64 個の PE すべてからビットデータを受信し、64bit 値として返す。
        '''
        return mpp_chip_cc.Router_read64_2d(self._router, x, y)
