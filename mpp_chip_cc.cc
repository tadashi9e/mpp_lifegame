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

static void mpp_free(PyObject* obj) {
  Controller64*
    c64 = reinterpret_cast<Controller64*>(PyCapsule_GetPointer(obj,
                                                               "MPP"));
  c64->stop();
  delete c64;
}

static PyObject*
mpp_new_MPP(PyObject* self, PyObject* args) {
  std::size_t memory_size;
  if (!PyArg_ParseTuple(args, "k", &memory_size)) {
    return NULL;
  }
  Controller64* controller = new Controller64(memory_size);
  controller->start();
  return PyCapsule_New(controller,
                       "MPP", mpp_free);
}
static PyObject*
mpp_reset(PyObject* self, PyObject* args) {
  PyObject* obj;
  if (!PyArg_ParseTuple(args, "O", &obj)) {
    return NULL;
  }
  Controller64*
    c64 = reinterpret_cast<Controller64*>(PyCapsule_GetPointer(obj, "MPP"));
  if (!c64) {
    return NULL;
  }
  std::shared_ptr<ControllerCommand>
    cmd(std::make_shared<CommandReset>());
  c64->enqueue(cmd);
  Py_RETURN_NONE;
}
static PyObject*
mpp_load_a(PyObject* self, PyObject* args) {
  PyObject* obj;
  std::size_t addr_a;
  uint8_t read_flag;
  uint8_t op_s;
  if (!PyArg_ParseTuple(args, "OkBB", &obj, &addr_a, &read_flag, &op_s)) {
    return NULL;
  }
  Controller64*
    c64 = reinterpret_cast<Controller64*>(PyCapsule_GetPointer(obj, "MPP"));
  if (!c64) {
    return NULL;
  }
  std::shared_ptr<ControllerCommand>
    cmd(std::make_shared<CommandLoadA>(addr_a, read_flag, op_s));
  c64->enqueue(cmd);
  Py_RETURN_NONE;
}
static PyObject*
mpp_load_b(PyObject* self, PyObject* args) {
  PyObject* obj;
  std::size_t addr_b;
  uint8_t context_flag;
  uint8_t op_c;
  if (!PyArg_ParseTuple(args, "OkBB", &obj, &addr_b, &context_flag, &op_c)) {
    return NULL;
  }
  Controller64*
    c64 = reinterpret_cast<Controller64*>(PyCapsule_GetPointer(obj, "MPP"));
  if (!c64) {
    return NULL;
  }
  std::shared_ptr<ControllerCommand>
    cmd(std::make_shared<CommandLoadB>(addr_b, context_flag, op_c));
  c64->enqueue(cmd);
  Py_RETURN_NONE;
}
static PyObject*
mpp_store(PyObject* self, PyObject* args) {
  PyObject* obj;
  uint8_t write_flag;
  bool context_value;
  if (!PyArg_ParseTuple(args, "OBB", &obj, &write_flag, &context_value)) {
    return NULL;
  }
  Controller64*
    c64 = reinterpret_cast<Controller64*>(PyCapsule_GetPointer(obj, "MPP"));
  if (!c64) {
    return NULL;
  }
  std::shared_ptr<ControllerCommand>
    cmd(std::make_shared<CommandStore>(write_flag, context_value));
  c64->enqueue(cmd);
  Py_RETURN_NONE;
}
static PyObject*
mpp_recv64(PyObject* self, PyObject* args) {
  PyObject* obj;
  int chip_no;
  uint64_t value;
  if (!PyArg_ParseTuple(args, "Oik", &obj, &chip_no, &value)) {
    return NULL;
  }
  Controller64*
    c64 = reinterpret_cast<Controller64*>(PyCapsule_GetPointer(obj, "MPP"));
  if (!c64) {
    return NULL;
  }
  std::shared_ptr<ControllerCommand>
    cmd(std::make_shared<CommandRecv64>(chip_no, value));
  c64->enqueue(cmd);
  Py_RETURN_NONE;
}
static PyObject*
mpp_send64(PyObject* self, PyObject* args) {
  PyObject* obj;
  int chip_no;
  if (!PyArg_ParseTuple(args, "Oi", &obj, &chip_no)) {
    return NULL;
  }
  Controller64*
    c64 = reinterpret_cast<Controller64*>(PyCapsule_GetPointer(obj, "MPP"));
  if (!c64) {
    return NULL;
  }
  std::shared_ptr<CommandSend64>
    cmd(std::make_shared<CommandSend64>(chip_no));
  c64->enqueue(std::static_pointer_cast<ControllerCommand>(cmd));
  uint64_t value = cmd->wait_result();
  return PyLong_FromLong(value);
}
static PyObject*
mpp_send_bulk(PyObject* self, PyObject* args) {
  PyObject* obj;
  if (!PyArg_ParseTuple(args, "O", &obj)) {
    return NULL;
  }
  Controller64*
    c64 = reinterpret_cast<Controller64*>(PyCapsule_GetPointer(obj, "MPP"));
  if (!c64) {
    return NULL;
  }
  std::shared_ptr<CommandSendBulk>
    cmd(std::make_shared<CommandSendBulk>());
  c64->enqueue(std::static_pointer_cast<ControllerCommand>(cmd));
  std::vector<uint64_t> value = cmd->wait_result();
  PyObject* listObject = PyList_New(value.size());
  if (!listObject) {
    return NULL;
  }
  for (unsigned int i = 0; i < value.size(); ++i) {
    PyObject* element = PyLong_FromUnsignedLong(value[i]);
    if (!element) {
      return NULL;
    }
    PyList_SET_ITEM(listObject, i, element);
  }
  return listObject;
}
static PyObject*
mpp_news_n(PyObject* self, PyObject* args) {
  PyObject* obj;
  if (!PyArg_ParseTuple(args, "O", &obj)) {
    return NULL;
  }
  Controller64*
    c64 = reinterpret_cast<Controller64*>(PyCapsule_GetPointer(obj, "MPP"));
  if (!c64) {
    return NULL;
  }
  std::shared_ptr<ControllerCommand>
    cmd(std::make_shared<CommandNewsRotateN>());
  c64->enqueue(cmd);
  Py_RETURN_NONE;
}
static PyObject*
mpp_news_e(PyObject* self, PyObject* args) {
  PyObject* obj;
  if (!PyArg_ParseTuple(args, "O", &obj)) {
    return NULL;
  }
  Controller64*
    c64 = reinterpret_cast<Controller64*>(PyCapsule_GetPointer(obj, "MPP"));
  if (!c64) {
    return NULL;
  }
  std::shared_ptr<ControllerCommand>
    cmd(std::make_shared<CommandNewsRotateE>());
  c64->enqueue(cmd);
  Py_RETURN_NONE;
}
static PyObject*
mpp_news_w(PyObject* self, PyObject* args) {
  PyObject* obj;
  if (!PyArg_ParseTuple(args, "O", &obj)) {
    return NULL;
  }
  Controller64*
    c64 = reinterpret_cast<Controller64*>(PyCapsule_GetPointer(obj, "MPP"));
  if (!c64) {
    return NULL;
  }
  std::shared_ptr<ControllerCommand>
    cmd(std::make_shared<CommandNewsRotateW>());
  c64->enqueue(cmd);
  Py_RETURN_NONE;
}
static PyObject*
mpp_news_s(PyObject* self, PyObject* args) {
  PyObject* obj;
  if (!PyArg_ParseTuple(args, "O", &obj)) {
    return NULL;
  }
  Controller64*
    c64 = reinterpret_cast<Controller64*>(PyCapsule_GetPointer(obj, "MPP"));
  if (!c64) {
    return NULL;
  }
  std::shared_ptr<ControllerCommand>
    cmd(std::make_shared<CommandNewsRotateS>());
  c64->enqueue(cmd);
  Py_RETURN_NONE;
}
static PyObject*
mpp_news_unicast_recv(PyObject* self, PyObject* args) {
  PyObject* obj;
  int x;
  int y;
  int b;
  if (!PyArg_ParseTuple(args, "Oiii", &obj, &x, &y, &b)) {
    return NULL;
  }
  Controller64*
    c64 = reinterpret_cast<Controller64*>(PyCapsule_GetPointer(obj, "MPP"));
  if (!c64) {
    return NULL;
  }
  std::shared_ptr<ControllerCommand>
    cmd(std::make_shared<CommandNewsUnicastRecv>(x, y, b != 0));
  c64->enqueue(cmd);
  Py_RETURN_NONE;
}
static PyObject*
mpp_news_unicast_send(PyObject* self, PyObject* args) {
  PyObject* obj;
  int x;
  int y;
  if (!PyArg_ParseTuple(args, "Oii", &obj, &x, &y)) {
    return NULL;
  }
  Controller64*
    c64 = reinterpret_cast<Controller64*>(PyCapsule_GetPointer(obj, "MPP"));
  if (!c64) {
    return NULL;
  }
  std::shared_ptr<CommandNewsUnicastSend>
    cmd(std::make_shared<CommandNewsUnicastSend>(x, y));
  c64->enqueue(cmd);
  uint64_t value = cmd->wait_result();
  return PyLong_FromUnsignedLong(value);
}

static PyMethodDef mpp_chip_methods[] = {
  {"newMPP", mpp_new_MPP, METH_VARARGS, "new MPP."},
  {"MPP_reset", mpp_reset, METH_VARARGS, "reset MPP."},
  {"MPP_load_a", mpp_load_a, METH_VARARGS, "load_a"},
  {"MPP_load_b", mpp_load_b, METH_VARARGS, "load_b"},
  {"MPP_store", mpp_store, METH_VARARGS, "store"},
  {"MPP_recv64", mpp_recv64, METH_VARARGS, "recv64"},
  {"MPP_send64", mpp_send64, METH_VARARGS, "send64"},
  {"MPP_send_bulk", mpp_send_bulk, METH_VARARGS, "send_bulk"},
  {"Router_news_n", mpp_news_n, METH_VARARGS, "news_n"},
  {"Router_news_e", mpp_news_e, METH_VARARGS, "news_e"},
  {"Router_news_w", mpp_news_w, METH_VARARGS, "news_w"},
  {"Router_news_s", mpp_news_s, METH_VARARGS, "news_s"},
  {"Router_news_recv", mpp_news_unicast_recv, METH_VARARGS, "news_recv"},
  {"Router_news_send", mpp_news_unicast_send, METH_VARARGS, "news_send"},
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
