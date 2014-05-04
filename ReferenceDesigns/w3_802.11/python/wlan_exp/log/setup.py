from distutils.core import setup
from distutils.extension import Extension
from Cython.Distutils import build_ext
import numpy as np
import os

os.environ["CC"] = "gcc-4.8"

# python setup.py build_ext --inplace

ext_module = Extension(
    "coll_util",
    ["coll_util.pyx"],
    extra_compile_args=['-fopenmp'],
    extra_link_args=['-fopenmp'],
)

setup(
    name = 'Collision Utility',
    cmdclass = {'build_ext': build_ext},
    include_dirs = [np.get_include()],
    ext_modules = [ext_module],
)