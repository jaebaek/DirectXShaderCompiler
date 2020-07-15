======================================================
SPIR-V generation of OpenCL.DebugInfo.100 instructions
======================================================

.. contents::
   :local:
   :depth: 3

Introduction
============

This document describes the generation of OpenCL.DebugInfo.100 instructions
in SPIR-V code. The `OpenCL.DebugInfo.100 <https://www.khronos.org/registry/spir-v/specs/unified1/OpenCL.DebugInfo.100.html>`__
is a SPIR-V extended instruction set.

How to use
==========

Use ``-fspv-debug=rich-with-source`` or ``-fspv-debug=rich`` **without**
``-fcgl``. ``-fspv-debug=rich-with-source`` will dump the source code using
`DebugSource <https://www.khronos.org/registry/spir-v/specs/unified1/OpenCL.DebugInfo.100.html#DebugSource>`__.
``-fspv-debug=rich-with-source`` is the same with ``-fspv-debug=rich`` other
than dumping the source code.

For example, you can compile your vertex shader ``foo.hlsl`` using

.. code-block:: shellscript
    path/to/dxc -T vs_6_0 -E main -spirv -fspv-debug=rich-with-source \
        -Fo foo.spv foo.hlsl

Design and implementation
=========================

Will be added.
