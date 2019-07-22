# -*- coding:utf-8;mode:python -*-
u'''
コマンドライン上で
python setup.py build_ext --inplace
とすると、シェアードライブラリができる。
'''
from distutils.core import setup, Extension

MPP_CHIP_CC_MODULE = Extension(
    'mpp_chip_cc', sources=['mpp_chip_cc.cc'], depends=['mpp_chip_cc.h'])

setup(name='mpp_chip_cc',
      version='0.0.1',
      description='MPP',
      ext_modules=[MPP_CHIP_CC_MODULE])
