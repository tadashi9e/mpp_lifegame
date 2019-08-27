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

#include "mpp_chip_cc.h"
#define PY_SSIZE_T_CLEAN
#include <Python.h>


extern "C" {

typedef MPP<1024> MPP_T;
typedef Router<1024, 256, 256> ROUTER_T;

static void mpp_chip_free(PyObject* obj) {
  MPP_T
    const* mpp = reinterpret_cast<MPP_T*>(PyCapsule_GetPointer(obj,
                                                                   "MPP"));
  delete mpp;
}

static PyObject*
mpp_chip_MPP(PyObject* self, PyObject* args) {
  std::size_t memory_size;
  if (!PyArg_ParseTuple(args, "k", &memory_size)) {
    return NULL;
  }
  return PyCapsule_New(new MPP_T(memory_size),
                       "MPP", mpp_chip_free);
}
static PyObject*
mpp_chip_MPP_reset(PyObject* self, PyObject* args) {
  PyObject* obj;
  if (!PyArg_ParseTuple(args, "O", &obj)) {
    return NULL;
  }
  MPP_T*
    mpp = reinterpret_cast<MPP_T*>(PyCapsule_GetPointer(obj, "MPP"));
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
  MPP_T*
    mpp = reinterpret_cast<MPP_T*>(PyCapsule_GetPointer(obj, "MPP"));
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
  MPP_T*
    mpp = reinterpret_cast<MPP_T*>(PyCapsule_GetPointer(obj, "MPP"));
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
  MPP_T*
    mpp = reinterpret_cast<MPP_T*>(PyCapsule_GetPointer(obj, "MPP"));
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
  MPP_T*
    mpp = reinterpret_cast<MPP_T*>(PyCapsule_GetPointer(obj, "MPP"));
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
  MPP_T*
    mpp = reinterpret_cast<MPP_T*>(PyCapsule_GetPointer(obj, "MPP"));
  if (!mpp) {
    return NULL;
  }
  uint64_t value = mpp->send(chip_no);
  return PyLong_FromUnsignedLong(value);
}
static void mpp_router_free(PyObject* obj) {
  ROUTER_T const*
    router = reinterpret_cast<ROUTER_T*>(PyCapsule_GetPointer(obj, "Router"));
  delete router;
}

static PyObject*
mpp_chip_Router(PyObject* self, PyObject* args) {
  PyObject* obj;
  if (!PyArg_ParseTuple(args, "O", &obj)) {
    return NULL;
  }
  MPP_T*
    mpp = reinterpret_cast<MPP_T*>(PyCapsule_GetPointer(obj, "MPP"));
  if (!mpp) {
    return NULL;
  }
  return PyCapsule_New(new ROUTER_T(mpp),
                       "Router", mpp_router_free);
}
static PyObject*
mpp_chip_Router_news_n(PyObject* self, PyObject* args) {
  PyObject* obj;
  if (!PyArg_ParseTuple(args, "O", &obj)) {
    return NULL;
  }
  ROUTER_T*
    router = reinterpret_cast<ROUTER_T*>(PyCapsule_GetPointer(obj, "Router"));
  router->rotate_news_n();
  Py_RETURN_NONE;
}
static PyObject*
mpp_chip_Router_news_e(PyObject* self, PyObject* args) {
  PyObject* obj;
  if (!PyArg_ParseTuple(args, "O", &obj)) {
    return NULL;
  }
  ROUTER_T*
    router = reinterpret_cast<ROUTER_T*>(PyCapsule_GetPointer(obj, "Router"));
  router->rotate_news_e();
  Py_RETURN_NONE;
}
static PyObject*
mpp_chip_Router_news_w(PyObject* self, PyObject* args) {
  PyObject* obj;
  if (!PyArg_ParseTuple(args, "O", &obj)) {
    return NULL;
  }
  ROUTER_T*
    router = reinterpret_cast<ROUTER_T*>(PyCapsule_GetPointer(obj, "Router"));
  router->rotate_news_w();
  Py_RETURN_NONE;
}
static PyObject*
mpp_chip_Router_news_s(PyObject* self, PyObject* args) {
  PyObject* obj;
  if (!PyArg_ParseTuple(args, "O", &obj)) {
    return NULL;
  }
  ROUTER_T*
    router = reinterpret_cast<ROUTER_T*>(PyCapsule_GetPointer(obj, "Router"));
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
  ROUTER_T*
    router = reinterpret_cast<ROUTER_T*>(PyCapsule_GetPointer(obj, "Router"));
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
  ROUTER_T*
    router = reinterpret_cast<ROUTER_T*>(PyCapsule_GetPointer(obj, "Router"));
  uint64_t value = router->read64_2d(x, y);
  return PyLong_FromUnsignedLong(value);
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
