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
    write_flag[i] = context_value ?
      (context[i] & t) | (~context[i] & write_flag[i]) :
      (~context[i] & t) | (context[i] & write_flag[i]);
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
          &flags[nchips * write_flag],
          op_s, op_c, context_value);
    for (std::size_t i = 0; i < nchips; ++i) {
      flags[nchips * FLAG_ZERO + i] = 0;
    }
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
    for (std::size_t x = 0; x < width / 64; ++x) {
      uint64_t const data0 = mpp->send(p64id_of_pos2d(x * 64, 0));
      for (std::size_t y = 1; y < height; ++y) {
        std::size_t const p0 = p64id_of_pos2d(x * 64, y);
        std::size_t const p1 = p64id_of_pos2d(x * 64, y-1);
        uint64_t const data = mpp->send(p0);
        mpp->recv(p1, data);
      }
      mpp->recv(p64id_of_pos2d(x * 64, height - 1), data0);
    }
  }
  void rotate_news_s() {
    for (std::size_t x = 0; x < width / 64; ++x) {
      uint64_t const data0 = mpp->send(p64id_of_pos2d(x * 64, height - 1));
      for (int y = static_cast<int>(height - 1); y >= 0; --y) {
        std::size_t const p0 = p64id_of_pos2d(x * 64, y);
        std::size_t const p1 = p64id_of_pos2d(x * 64, y + 1);
        uint64_t const data = mpp->send(p0);
        mpp->recv(p1, data);
      }
      mpp->recv(p64id_of_pos2d(x * 64, 0), data0);
    }
  }
  void rotate_news_e() {
    for (std::size_t y = 0; y < height; ++y) {
      uint64_t carry = mpp->send(p64id_of_pos2d(width - 64, y)) >> 63;
      for (std::size_t x = 0; x < width / 64; ++x) {
        std::size_t p = p64id_of_pos2d(x * 64, y);
        uint64_t data = mpp->send(p);
        uint64_t const carry2 = data >> 63;
        data = (data << 1) | (carry & 0x01);
        carry = carry2;
        mpp->recv(p, data);
      }
    }
  }
  void rotate_news_w() {
    for (std::size_t y = 0; y < height; ++y) {
      uint64_t carry = mpp->send(p64id_of_pos2d(0, y)) & 0x01;
      for (int x = static_cast<int>(width / 64 - 1); x >= 0; --x) {
        std::size_t p = p64id_of_pos2d(x * 64, y);
        uint64_t data = mpp->send(p);
        uint64_t const carry2 = data & 0x01;
        data = (carry << 63) | (data >> 1);
        carry = carry2;
        mpp->recv(p, data);
      }
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

#define PY_SSIZE_T_CLEAN
#include <Python.h>
extern "C" {

static void mpp_chip_free(PyObject* obj) {
  MPP const* mpp = reinterpret_cast<MPP*>(PyCapsule_GetPointer(obj, "MPP"));
  delete mpp;
}

static PyObject*
mpp_chip_MPP(PyObject* self, PyObject* args) {
  int nchips;
  std::size_t memory_size;
  if (!PyArg_ParseTuple(args, "kk", &nchips, &memory_size)) {
    return NULL;
  }
  return PyCapsule_New(new MPP(nchips, memory_size), "MPP", mpp_chip_free);
}
static PyObject*
mpp_chip_MPP_reset(PyObject* self, PyObject* args) {
  PyObject* obj;
  if (!PyArg_ParseTuple(args, "O", &obj)) {
    return NULL;
  }
  MPP* mpp = reinterpret_cast<MPP*>(PyCapsule_GetPointer(obj, "MPP"));
  if (!mpp) {
    return NULL;
  }
  mpp->reset();
  Py_RETURN_NONE;
}
static PyObject*
mpp_chip_MPP_load_a(PyObject* self, PyObject* args) {
  PyObject* obj;
  std::size_t addr_a;
  uint8_t read_flag;
  uint8_t op_s;
  if (!PyArg_ParseTuple(args, "OkBB", &obj, &addr_a, &read_flag, &op_s)) {
    return NULL;
  }
  MPP* mpp = reinterpret_cast<MPP*>(PyCapsule_GetPointer(obj, "MPP"));
  if (!mpp) {
    return NULL;
  }
  mpp->load_a(addr_a, read_flag, op_s);
  Py_RETURN_NONE;
}
static PyObject*
mpp_chip_MPP_load_b(PyObject* self, PyObject* args) {
  PyObject* obj;
  std::size_t addr_b;
  uint8_t context_flag;
  uint8_t op_c;
  if (!PyArg_ParseTuple(args, "OkBB", &obj, &addr_b, &context_flag, &op_c)) {
    return NULL;
  }
  MPP* mpp = reinterpret_cast<MPP*>(PyCapsule_GetPointer(obj, "MPP"));
  if (!mpp) {
    return NULL;
  }
  mpp->load_b(addr_b, context_flag, op_c);
  Py_RETURN_NONE;
}
static PyObject*
mpp_chip_MPP_store(PyObject* self, PyObject* args) {
  PyObject* obj;
  uint8_t write_flag;
  bool context_value;
  if (!PyArg_ParseTuple(args, "OBB", &obj, &write_flag, &context_value)) {
    return NULL;
  }
  MPP* mpp = reinterpret_cast<MPP*>(PyCapsule_GetPointer(obj, "MPP"));
  if (!mpp) {
    return NULL;
  }
  mpp->store(write_flag, context_value);
  Py_RETURN_NONE;
}
static PyObject*
mpp_chip_MPP_recv(PyObject* self, PyObject* args) {
  PyObject* obj;
  int chip_no;
  uint64_t value;
  if (!PyArg_ParseTuple(args, "Oik", &obj, &chip_no, &value)) {
    return NULL;
  }
  MPP* mpp = reinterpret_cast<MPP*>(PyCapsule_GetPointer(obj, "MPP"));
  if (!mpp) {
    return NULL;
  }
  mpp->recv(chip_no, value);
  Py_RETURN_NONE;
}
static PyObject*
mpp_chip_MPP_send(PyObject* self, PyObject* args) {
  PyObject* obj;
  int chip_no;
  if (!PyArg_ParseTuple(args, "Oi", &obj, &chip_no)) {
    return NULL;
  }
  MPP* mpp = reinterpret_cast<MPP*>(PyCapsule_GetPointer(obj, "MPP"));
  if (!mpp) {
    return NULL;
  }
  uint64_t value = mpp->send(chip_no);
  return PyLong_FromLong(value);
}
static void mpp_router_free(PyObject* obj) {
  Router const*
    router = reinterpret_cast<Router*>(PyCapsule_GetPointer(obj, "Router"));
  delete router;
}

static PyObject*
mpp_chip_Router(PyObject* self, PyObject* args) {
  PyObject* obj;
  int width;
  int height;
  if (!PyArg_ParseTuple(args, "Oii", &obj, &width, &height)) {
    return NULL;
  }
  MPP* mpp = reinterpret_cast<MPP*>(PyCapsule_GetPointer(obj, "MPP"));
  if (!mpp) {
    return NULL;
  }
  return PyCapsule_New(new Router(mpp, width, height),
                       "Router", mpp_router_free);
}
static PyObject*
mpp_chip_Router_news_n(PyObject* self, PyObject* args) {
  PyObject* obj;
  if (!PyArg_ParseTuple(args, "O", &obj)) {
    return NULL;
  }
  Router*
    router = reinterpret_cast<Router*>(PyCapsule_GetPointer(obj, "Router"));
  router->rotate_news_n();
  Py_RETURN_NONE;
}
static PyObject*
mpp_chip_Router_news_e(PyObject* self, PyObject* args) {
  PyObject* obj;
  if (!PyArg_ParseTuple(args, "O", &obj)) {
    return NULL;
  }
  Router*
    router = reinterpret_cast<Router*>(PyCapsule_GetPointer(obj, "Router"));
  router->rotate_news_e();
  Py_RETURN_NONE;
}
static PyObject*
mpp_chip_Router_news_w(PyObject* self, PyObject* args) {
  PyObject* obj;
  if (!PyArg_ParseTuple(args, "O", &obj)) {
    return NULL;
  }
  Router*
    router = reinterpret_cast<Router*>(PyCapsule_GetPointer(obj, "Router"));
  router->rotate_news_w();
  Py_RETURN_NONE;
}
static PyObject*
mpp_chip_Router_news_s(PyObject* self, PyObject* args) {
  PyObject* obj;
  if (!PyArg_ParseTuple(args, "O", &obj)) {
    return NULL;
  }
  Router*
    router = reinterpret_cast<Router*>(PyCapsule_GetPointer(obj, "Router"));
  router->rotate_news_s();
  Py_RETURN_NONE;
}
static PyObject*
mpp_chip_Router_unicast_2d(PyObject* self, PyObject* args) {
  PyObject* obj;
  int x;
  int y;
  int b;
  if (!PyArg_ParseTuple(args, "Oiii", &obj, &x, &y, &b)) {
    return NULL;
  }
  Router*
    router = reinterpret_cast<Router*>(PyCapsule_GetPointer(obj, "Router"));
  router->unicast_2d(x, y, b);
  Py_RETURN_NONE;
}
static PyObject*
mpp_chip_Router_read64_2d(PyObject* self, PyObject* args) {
  PyObject* obj;
  int x;
  int y;
  if (!PyArg_ParseTuple(args, "Oii", &obj, &x, &y)) {
    return NULL;
  }
  Router*
    router = reinterpret_cast<Router*>(PyCapsule_GetPointer(obj, "Router"));
  uint64_t value = router->read64_2d(x, y);
  return PyLong_FromLong(value);
}

static PyMethodDef mpp_chip_methods[] = {
  {"newMPP", mpp_chip_MPP, METH_VARARGS, "new MPP."},
  {"MPP_reset", mpp_chip_MPP_reset, METH_VARARGS, "reset MPP."},
  {"MPP_load_a", mpp_chip_MPP_load_a, METH_VARARGS, "load_a"},
  {"MPP_load_b", mpp_chip_MPP_load_b, METH_VARARGS, "load_b"},
  {"MPP_store", mpp_chip_MPP_store, METH_VARARGS, "store"},
  {"MPP_recv", mpp_chip_MPP_recv, METH_VARARGS, "recv"},
  {"MPP_send", mpp_chip_MPP_send, METH_VARARGS, "send"},
  {"newRouter", mpp_chip_Router, METH_VARARGS, "new Router"},
  {"Router_news_n", mpp_chip_Router_news_n, METH_VARARGS, "news_n"},
  {"Router_news_e", mpp_chip_Router_news_e, METH_VARARGS, "news_e"},
  {"Router_news_w", mpp_chip_Router_news_w, METH_VARARGS, "news_w"},
  {"Router_news_s", mpp_chip_Router_news_s, METH_VARARGS, "news_s"},
  {"Router_unicast_2d", mpp_chip_Router_unicast_2d, METH_VARARGS, "unicast 2d"},
  {"Router_read64_2d", mpp_chip_Router_read64_2d, METH_VARARGS, "read 2d"},
  {NULL, NULL, 0, NULL}
};

static struct PyModuleDef mpp_chip_module = {
  PyModuleDef_HEAD_INIT,
  "mpp_chip_cc",
  NULL,
  -1,
  mpp_chip_methods,
};

PyMODINIT_FUNC
PyInit_mpp_chip_cc(void) {
  return PyModule_Create(&mpp_chip_module);
}
}
