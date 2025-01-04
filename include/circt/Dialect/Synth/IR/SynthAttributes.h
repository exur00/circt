//===- SynthAttributes.h - Declare Synth dialect attributes ----------*- C++-*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef CIRCT_DIALECT_SYNTH_IR_SYNTHATTRIBUTES_H
#define CIRCT_DIALECT_SYNTH_IR_SYNTHATTRIBUTES_H

#include "mlir/IR/Attributes.h"
#include "mlir/IR/BuiltinAttributes.h"
#include "mlir/IR/BuiltinTypes.h"

#include "circt/Dialect/Synth/IR/SynthEnums.h.inc"
#define GET_ATTRDEF_CLASSES
#include "circt/Dialect/Synth/IR/SynthAttributes.h.inc"

#endif // CIRCT_DIALECT_SYNTH_IR_SYNTHATTRIBUTES_H
