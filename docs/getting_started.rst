.. _getting_started:

***************
Getting Started
***************

.. _installing-docdir:

Requirements
============
PyCALI requires the following third-party packages

* **CMake**: https://cmake.org/
  
  a software for package compilation.


* **Pybind11**: https://github.com/pybind/pybind11
  
  a lightweight header-only library for python wrapper.


* **LAPACKE**: http://performance.netlib.org/lapack/
  
  C/C++ interface to LAPACK.


* **CBLAS**: https://www.netlib.org/blas/

  C/C++ interface to BLAS.


* **GSL**: https://www.gnu.org/software/gsl/
  
  GNU Scientific Library.

* **cmaketools**: https://pypi.org/project/cmaketools/
  
  An integration of Cmake build system to Python setuptools/distutils.
  Only used for Python wrapper.

Installation
============
PyCALI uses CMake to do building and compilation. 

The following installations presume that LAPACKE and CBLAS are installed in the default paths, namely, for LAPACKE, headers placed 
at /usr/include/lapacke and libraries at /usr/lib or /usr/lib64; for CBLAS, headers placed 
at /usr/include/cblas and libraries at /usr/lib or /usr/lib64.  If this is not the case, use the CMake GUI to 
make editing

.. code-block:: bash 
  
  ccmake .

The triggered GUI generally looks like 

.. code-block:: bash 

  CBLAS_INCLUDE_DIR                /usr/include/cblas
  CBLAS_LIB                        /usr/lib64/libcblas.so
  CMAKE_BUILD_TYPE
  CMAKE_INSTALL_PREFIX             /usr/local
  LAPACKE_INCLUDE_DIR              /usr/include/lapacke
  LAPACKE_LIB                      /usr/lib64/liblapacke.so
  PYBIND11_CPP_STANDARD            -std=c++14
  PYBIND11_PYTHON_VERSION
  pybind11_DIR                     /usr/share/cmake/pybind11

C/C++ executable binary: cali
-----------------------------

If one wants to create executable binary file ``cali``, use the command 

.. code-block:: bash 

  cmake .
  make cali 

Python module: pycali
---------------------

If one wants to create Python module ``pycali``, use the command 

.. code-block:: bash
  
  cmake .
  python setup.py install --user 

This will install pycali module to a path that can be reconginzed by the Python interpretor.
Usually this path is located at, e.g., .local/lib/python3.9/site-packages. 


Basic Usage
===========

Either ``cali`` or ``pycali`` can be used to do intercalibrating.  ``cali`` is an executable binary file 
and can directly executed in a Linux terminal as

.. code-block:: bash
  
  ./cali param.txt 

in which ``param.txt`` specifies the configurations passed to ``cali``.

For the Python module ``pycali``, a Python script ``example.py`` shows
an example regarding the usage.

.. note::

  A directory "data/" in the present working directory is needed to place ouput files. ``cali`` and ``pycali``
  automatically check whether the directory exists. If not, it will be created.

A python script ``plot_results.py`` in the subdirtory ``data/`` shows how to plot 
the merged light curves and the posterior distributions of parameters. 

The final intercalibrated light curves are output to files with a name by adding a postfix "_cali" 
to the input file name. For example, if your intput file name is "exmaple.txt", the output 
file name is "example.txt_cali".