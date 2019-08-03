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


#include <condition_variable>
#include <cstdint>
#include <vector>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>

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
template<std::size_t nchips>
void alu64_s(uint64_t* a,
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
template<std::size_t nchips>
void alu64_c(uint64_t const* a,
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
template<std::size_t nchips>
void alu64(uint64_t* a,
           uint64_t const* b,
           uint64_t const* f,
           uint64_t const* context,
           uint64_t* write_flag,
           uint8_t op_s,
           uint8_t op_c,
           bool context_value) {
  alu64_s<nchips>(a, b, f, context, context_value, op_s);
  alu64_c<nchips>(a, b, f, context, write_flag, context_value, op_c);
}

/**
 * 64 * nchips 個の PE
 */
template<std::size_t nchips>
class MPP {
 private:
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
  MPP(std::size_t address_size) {
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
    alu64<nchips>(
          &memory[nchips * addr_a],
          &memory[nchips * addr_b],
          &flags[nchips * read_flag],
          &flags[nchips * context_flag],
          write_flag == 0 ? NULL : &flags[nchips * write_flag],
          op_s, op_c, context_value);
  }
  void recv64(std::size_t chip_no, uint64_t value) {
    flags[nchips * FLAG_ROUTE_DATA + chip_no] = value;
  }
  uint64_t send64(std::size_t chip_no) const {
    return flags[nchips * FLAG_ROUTE_DATA + chip_no];
  }
  std::vector<uint64_t> send_bulk() const {
    std::vector<uint64_t> vec;
    vec.reserve(nchips);
    for (std::size_t n = 0; n < nchips; ++n) {
      vec.push_back(flags[nchips * FLAG_ROUTE_DATA + n]);
    }
    return vec;
  }
};

template<std::size_t nchips, std::size_t width, std::size_t height>
class Router {
 public:
  Router(MPP<nchips>* mpp)
    : mpp(mpp) {
  }
  void news_rotate_n() {
    std::size_t const width64 = width / 64;
    for (std::size_t x = 0; x < width64; ++x) {
      uint64_t const data0 = mpp->send64(x);
      std::size_t p0 = x + width64;
      std::size_t p1 = x;
      for (std::size_t y = 1; y < height; ++y) {
        uint64_t const data = mpp->send64(p0);
        mpp->recv64(p1, data);
        p0 += width64;
        p1 += width64;
      }
      mpp->recv64(p1, data0);
    }
  }
  void news_rotate_s() {
    std::size_t const width64 = width / 64;
    for (std::size_t x = 0; x < width64; ++x) {
      uint64_t data = mpp->send64(x + width64 * (height - 1));
      std::size_t p = x;
      for (size_t y = 0; y < height; ++y) {
        uint64_t const data0 = mpp->send64(p);
        mpp->recv64(p, data);
        data = data0;
        p += width64;
      }
    }
  }
  void news_rotate_e() {
    std::size_t const width64 = width / 64;
    for (std::size_t y = 0; y < height; ++y) {
      uint64_t carry = mpp->send64(width64 - 1 + width64 * y) >> 63;
      for (std::size_t x = 0; x < width64; ++x) {
        std::size_t p = x + width64 * y;
        uint64_t const data0 = mpp->send64(p);
        uint64_t const carry2 = data0 >> 63;
        uint64_t const data1 = (data0 << 1) | (carry & 0x01);
        carry = carry2;
        mpp->recv64(p, data1);
      }
    }
  }
  void news_rotate_w() {
    std::size_t const width64 = width / 64;
    for (std::size_t y = 0; y < height; ++y) {
      std::size_t p = width64 * y;
      uint64_t data0 = mpp->send64(p);
      uint64_t data = data0;
      for (std::size_t x = 0; x < width64 - 1; ++x) {
        uint64_t const d = mpp->send64(p + 1);
        uint64_t const carry = d & 0x01;
        mpp->recv64(p, (carry << 63) | (data >> 1));
        data = d;
        ++p;
      }
      uint64_t const carry = data0 & 0x01;
      mpp->recv64(p, (carry << 63) | (data >> 1));
    }
  }
  void news_unicast_recv(int x, int y, bool b) {
    std::size_t p64id = p64id_of_pos2d(x, y);
    uint64_t data = mpp->send64(p64id);
    if (b) {
      data |= (static_cast<uint64_t>(1) << (x % 64));
    } else {
      data &= ~(static_cast<uint64_t>(1) << (x % 64));
    }
    mpp->recv64(p64id, data);
  }
  bool news_unicast_send(int x, int y) {
    std::size_t p64id = p64id_of_pos2d(x, y);
    uint64_t data = mpp->send64(p64id);
    return (data & (static_cast<uint64_t>(1) << (x % 64))) != 0;
  }

 private:
  MPP<nchips>* mpp;
  std::size_t pid_of_pos2d(int x, int y) const {
    return x + (y * width);
  }
  std::size_t p64id_of_pos2d(int x, int y) const {
    return pid_of_pos2d(x, y) / 64;
  }
};
typedef MPP<1024> MPP64;
typedef Router<1024, 256, 256> Router64;
class ControllerCommand {
public:
  virtual void execute(MPP64* mpp, Router64* router) = 0;
};
class CommandReset : public ControllerCommand {
public:
  CommandReset() {
  }
  virtual void execute(MPP64* mpp, Router64* router) {
    mpp->reset();
  }
};
class CommandLoadA : public ControllerCommand {
public:
  CommandLoadA(std::size_t addr, uint8_t flag, uint8_t op_s)
    : addr(addr), flag(flag), op_s(op_s) {
  }
  void execute(MPP64* mpp, Router64* router) {
    mpp->load_a(addr, flag, op_s);
  }
private:
  std::size_t const addr;
  uint8_t const flag;
  uint8_t const op_s;
};
class CommandLoadB : public ControllerCommand {
public:
  CommandLoadB(std::size_t addr, uint8_t flag, uint8_t op_c)
    : addr(addr), flag(flag), op_c(op_c) {
  }
  void execute(MPP64* mpp, Router64* router) {
    mpp->load_b(addr, flag, op_c);
  }
private:
  std::size_t const addr;
  uint8_t const flag;
  uint8_t const op_c;
};
class CommandStore : public ControllerCommand {
public:
  CommandStore(uint8_t flag, bool context_value)
    : flag(flag), context_value(context_value) {
  }
  void execute(MPP64* mpp, Router64* router) {
    mpp->store(flag, context_value);
  }
private:
  uint8_t const flag;
  bool const context_value;
};
class CommandNewsRotateN : public ControllerCommand {
public:
  CommandNewsRotateN() {
  }
  void execute(MPP64* mpp, Router64* router) {
    router->news_rotate_n();
  }
};
class CommandNewsRotateE : public ControllerCommand {
public:
  CommandNewsRotateE() {
  }
  void execute(MPP64* mpp, Router64* router) {
    router->news_rotate_e();
  }
};
class CommandNewsRotateW : public ControllerCommand {
public:
  CommandNewsRotateW() {
  }
  void execute(MPP64* mpp, Router64* router) {
    router->news_rotate_w();
  }
};
class CommandNewsRotateS : public ControllerCommand {
public:
  CommandNewsRotateS() {
  }
  void execute(MPP64* mpp, Router64* router) {
    router->news_rotate_s();
  }
};
class CommandRecv64 : public ControllerCommand {
public:
  CommandRecv64(std::size_t chip_no, uint64_t data)
    : chip_no(chip_no), data(data) {
  }
  void execute(MPP64* mpp, Router64* router) {
    mpp->recv64(chip_no, data);
  }
private:
  std::size_t const chip_no;
  uint64_t const data;
};
class CommandSend64 : public ControllerCommand {
public:
  CommandSend64(std::size_t chip_no)
    : chip_no(chip_no), done(false) {
  }
  void execute(MPP64* mpp, Router64* router) {
    uint64_t data_ = mpp->send64(chip_no);
    std::unique_lock<std::mutex> lock(mutex_);
    data = data_;
    done = true;
    cond_.notify_all();
  }
  uint64_t wait_result() const {
    std::unique_lock<std::mutex> lock(mutex_);
    while (!done) {
      cond_.wait(lock);
    }
    return data;
  }
private:
  std::size_t const chip_no;
  mutable std::mutex mutex_;
  mutable std::condition_variable cond_;
  bool done;
  uint64_t data;
};
class CommandSendBulk : public ControllerCommand {
public:
  CommandSendBulk() : done(false) {
  }
  void execute(MPP64* mpp, Router64* router) {
    std::vector<uint64_t> data_ = mpp->send_bulk();
    std::unique_lock<std::mutex> lock(mutex_);
    data = data_;
    done = true;
    cond_.notify_all();
  }
  std::vector<uint64_t> wait_result() const {
    std::unique_lock<std::mutex> lock(mutex_);
    while (!done) {
      cond_.wait(lock);
    }
    return data;
  }
private:
  mutable std::mutex mutex_;
  mutable std::condition_variable cond_;
  bool done;
  std::vector<uint64_t> data;
};
class CommandNewsUnicastRecv : public ControllerCommand {
public:
  CommandNewsUnicastRecv(int x, int y, bool b)
    : x(x), y(y), b(b) {
  }
  void execute(MPP64* mpp, Router64* router) {
    router->news_unicast_recv(x, y, b);
  }
private:
  int const x;
  int const y;
  bool const b;
};
class CommandNewsUnicastSend : public ControllerCommand {
public:
  CommandNewsUnicastSend(int x, int y)
    : x(x), y(y), done(false) {
  }
  void execute(MPP64* mpp, Router64* router) {
    bool b_ = router->news_unicast_send(x, y);
    std::unique_lock<std::mutex> lock(mutex_);
    b = b_;
    done = true;
    cond_.notify_all();
  }
  bool wait_result() const {
    std::unique_lock<std::mutex> lock(mutex_);
    while (!done) {
      cond_.wait(lock);
    }
    return b;
  }
private:
  int const x;
  int const y;
  mutable std::mutex mutex_;
  mutable std::condition_variable cond_;
  bool done;
  bool b;
};
class Controller64 {
public:
  Controller64(std::size_t address_size)
    : mpp_(address_size), router_(&mpp_), stop_(false) {
  }
  void enqueue(std::shared_ptr<ControllerCommand> cmd) {
    std::unique_lock<std::mutex> lock(mutex_);
    cmd_queue_.push(cmd);
    cond_.notify_all();
  }
  bool empty() const {
    std::unique_lock<std::mutex> lock(mutex_);
    return cmd_queue_.empty();
  }
  void start() {
    thread_ = std::make_shared<std::thread>(&Controller64::execute_, this);
  }
  void stop() {
    {
      std::unique_lock<std::mutex> lock(mutex_);
      stop_ = true;
      cond_.notify_all();
    }
    if (thread_) {
      thread_->join();
    }
  }
private:
  MPP64 mpp_;
  Router64 router_;
  std::shared_ptr<std::thread> thread_;
  mutable std::mutex mutex_;
  mutable std::condition_variable cond_;
  bool stop_;
  std::queue<std::shared_ptr<ControllerCommand> > cmd_queue_;
  void execute_() {
    for (;;) {
      std::shared_ptr<ControllerCommand> cmd;
      {
        std::unique_lock<std::mutex> lock(mutex_);
        while (cmd_queue_.empty()) {
          if (stop_) {
            return;
          }
          cond_.wait(lock);
        }
        cmd = cmd_queue_.front();
        cmd_queue_.pop();
      }
      cmd->execute(&mpp_, &router_);
    }
  }
};
