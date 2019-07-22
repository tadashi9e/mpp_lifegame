// -*- coding:utf-8-unix; mode:c++ -*-
// Copyright 2019 tadashi9e

/*
 * コンパイル方法:
 *     python setup.py build_ext --inplace
 *
 * mpp_chip_cc.cpython-36m-x86_64-linux-gnu.so のようなものができる。
 *
 * mpp_chip.py から利用する前提。
 */


#include <cstdint>
#include <vector>
#include <iostream>

/**
 * 64 個の PE について、基本となる ALU 演算を行う。
 * A, B, F の値および真理値表を元に値を返す。
 */
inline uint64_t
alu64_core(uint64_t a,
           uint64_t b,
           uint64_t f,
           uint8_t op) {
  uint64_t t = 0;
  if ((op & 0x01) != 0) {
    t |= ~a & ~b & ~f;
  }
  if ((op & 0x02) != 0) {
    t |= ~a & ~b &  f;
  }
  if ((op & 0x04) != 0) {
    t |= ~a &  b & ~f;
  }
  if ((op & 0x08) != 0) {
    t |= ~a &  b &  f;
    }
  if ((op & 0x10) != 0) {
    t |=  a & ~b & ~f;
  }
  if ((op & 0x20) != 0) {
    t |=  a & ~b &  f;
  }
  if ((op & 0x40) != 0) {
    t |=  a &  b & ~f;
  }
  if ((op & 0x80) != 0) {
    t |=  a &  b &  f;
  }
  return t;
}


/**
 * A, B, F, コンテキストフラグ の値および真理値表 OP_S を元に A を更新する。
 * 64 * nchips 個の PE について一度に処理する。
 */
void alu64_s(std::size_t nchips,
             uint64_t* a,
             uint64_t const* b,
             uint64_t const* f,
             uint64_t const* context,
             bool context_value,
             uint8_t op_s) {
  for (std::size_t i = 0; i < nchips; ++i) {
    uint64_t t = alu64_core(a[i], b[i], f[i], op_s);
    a[i] = context_value ?
      (context[i] & t) | (~context[i] & a[i]) :
      (~context[i] & t) | (context[i] & a[i]);
  }
}

/**
 * A, B, F, コンテキストフラグ の値および真理値表 OP_C を元にフラグを更新する。
 * 64 * nchips 個の PE について一度に処理する。
 */
void alu64_c(std::size_t nchips,
             uint64_t const* a,
             uint64_t const* b,
             uint64_t const* f,
             uint64_t const* context,
             uint64_t* write_flag,
             bool context_value,
             uint8_t op_c) {
  for (std::size_t i = 0; i < nchips; ++i) {
    uint64_t t = alu64_core(a[i], b[i], f[i], op_c);
    if (write_flag) {
      write_flag[i] = context_value ?
        (context[i] & t) | (~context[i] & write_flag[i]) :
        (~context[i] & t) | (context[i] & write_flag[i]);
    }
  }
}

/**
 * 64 * nchips 個の PE について ALU 処理を行う。
 */
void alu64(std::size_t nchips,
           uint64_t* a,
           uint64_t const* b,
           uint64_t const* f,
           uint64_t const* context,
           uint64_t* write_flag,
           uint8_t op_s,
           uint8_t op_c,
           bool context_value) {
  alu64_s(nchips, a, b, f, context, context_value, op_s);
  alu64_c(nchips, a, b, f, context, write_flag, context_value, op_c);
}

/**
 * 64 * nchips 個の PE
 */
class MPP {
 private:
  /**
   * PE チップの数。各チップには 64 個の PE が搭載されている。
   */
  size_t nchips;
  /**
   * フラグに関する定義: 0 番目のフラグの値は常に 0 とする。
   */
  const int FLAG_ZERO = 0;
  /**
   * フラグに関する定義: 63 番目のフラグはルータとの通信用。
   */
  const int FLAG_ROUTE_DATA = 63;
  /**
   * フラグに関する定義: フラグは PE につき全部で 64 個。
   */
  const int FLAG_MAX = 64;
  /**
   * 全 PE のメモリ。全 PE について同一アドレスは連続領域上に置く。
   */
  std::vector<uint64_t> memory;
  /**
   * 全 PE のフラグ。全 PE について同一アドレスは連続領域上に置く。
   */
  std::vector<uint64_t> flags;
  // ---- load_a 命令で設定される値 ----
  std::size_t addr_a;  // A レジスタにロードするメモリアドレス。
  uint8_t read_flag;  // F レジスタにロードする参照フラグ。
  uint8_t op_s;  // S 真理値表。
  // ---- load_b 命令で設定される値 ----
  std::size_t addr_b;  // B レジスタにロードするメモリアドレス。
  uint8_t context_flag;  // コンテキストフラグ。
  uint8_t op_c;  // C 真理値表。

 public:
  MPP(std::size_t nchips, std::size_t address_size) : nchips(nchips) {
    memory.resize(nchips * address_size);
    flags.resize(nchips * FLAG_MAX);
  }
  std::size_t total_cores() const {
    return 64 * nchips;
  }
  void reset() {
    std::vector<uint64_t> cleared;
    cleared.resize(flags.size());
    flags.swap(cleared);
  }
  void load_a(std::size_t addr_a,
              uint8_t read_flag,
              uint8_t op_s) {
    this->addr_a = addr_a;
    this->read_flag = read_flag;
    this->op_s = op_s;
  }
  void load_b(std::size_t addr_b,
              uint8_t context_flag,
              uint8_t op_c) {
    this->addr_b = addr_b;
    this->context_flag = context_flag;
    this->op_c = op_c;
  }
  void store(uint8_t write_flag,
             bool context_value) {
    alu64(nchips,
          &memory[nchips * addr_a],
          &memory[nchips * addr_b],
          &flags[nchips * read_flag],
          &flags[nchips * context_flag],
          write_flag == 0 ? NULL : &flags[nchips * write_flag],
          op_s, op_c, context_value);
  }
  void recv(std::size_t chip_no, uint64_t value) {
    flags[nchips * FLAG_ROUTE_DATA + chip_no] = value;
  }
  uint64_t send(std::size_t chip_no) const {
    return flags[nchips * FLAG_ROUTE_DATA + chip_no];
  }
};

class Router {
 public:
  Router(MPP* mpp, std::size_t width, std::size_t height)
    : mpp(mpp), width(width), height(height) {
    if (width % 64) {
      throw(std::invalid_argument("WIDTH must be n times 64"));
    }
    if (mpp->total_cores() != width * height) {
      throw(std::invalid_argument("number of cores not match WIDTH*HEIGHT"));
    }
  }
  void rotate_news_n() {
    std::size_t const width64 = width / 64;
    for (std::size_t x = 0; x < width64; ++x) {
      uint64_t const data0 = mpp->send(x);
      std::size_t p0 = x + width64;
      std::size_t p1 = x;
      for (std::size_t y = 1; y < height; ++y) {
        uint64_t const data = mpp->send(p0);
        mpp->recv(p1, data);
        p0 += width64;
        p1 += width64;
      }
      mpp->recv(p1, data0);
    }
  }
  void rotate_news_s() {
    std::size_t const width64 = width / 64;
    for (std::size_t x = 0; x < width64; ++x) {
      uint64_t data = mpp->send(x + width64 * (height - 1));
      std::size_t p = x;
      for (size_t y = 0; y < height; ++y) {
        uint64_t const data0 = mpp->send(p);
        mpp->recv(p, data);
        data = data0;
        p += width64;
      }
    }
  }
  void rotate_news_e() {
    std::size_t const width64 = width / 64;
    for (std::size_t y = 0; y < height; ++y) {
      uint64_t carry = mpp->send(width64 - 1 + width64 * y) >> 63;
      for (std::size_t x = 0; x < width64; ++x) {
        std::size_t p = x + width64 * y;
        uint64_t const data0 = mpp->send(p);
        uint64_t const carry2 = data0 >> 63;
        uint64_t const data1 = (data0 << 1) | (carry & 0x01);
        carry = carry2;
        mpp->recv(p, data1);
      }
    }
  }
  void rotate_news_w() {
    std::size_t const width64 = width / 64;
    for (std::size_t y = 0; y < height; ++y) {
      std::size_t p = width64 * y;
      uint64_t data0 = mpp->send(p);
      uint64_t data = data0;
      for (std::size_t x = 0; x < width64 - 1; ++x) {
        uint64_t const d = mpp->send(p + 1);
        uint64_t const carry = d & 0x01;
        mpp->recv(p, (carry << 63) | (data >> 1));
        data = d;
        ++p;
      }
      uint64_t const carry = data0 & 0x01;
      mpp->recv(p, (carry << 63) | (data >> 1));
    }
  }
  void unicast_2d(int x, int y, bool b) {
    std::size_t p64id = p64id_of_pos2d(x, y);
    uint64_t data = mpp->send(p64id);
    if (b) {
      data |= (static_cast<uint64_t>(1) << (x % 64));
    } else {
      data &= ~(static_cast<uint64_t>(1) << (x % 64));
    }
    mpp->recv(p64id, data);
  }
  uint64_t read64_2d(int x, int y) {
    std::size_t p64id = p64id_of_pos2d(x, y);
    return mpp->send(p64id);
  }

 private:
  MPP* mpp;
  std::size_t width;
  std::size_t height;
  std::size_t pid_of_pos2d(int x, int y) const {
    return x + (y * width);
  }
  std::size_t p64id_of_pos2d(int x, int y) const {
    return pid_of_pos2d(x, y) / 64;
  }
};

