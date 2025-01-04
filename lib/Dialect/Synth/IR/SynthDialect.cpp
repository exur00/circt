//===- SynthDialect.cpp - Implement the Synth dialect -------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file implements the Synth dialect.
//
//===----------------------------------------------------------------------===//

#include "circt/Dialect/Synth/IR/SynthDialect.h"
#include "circt/Dialect/Synth/IR/SynthOps.h"
#include "circt/Dialect/Synth/IR/SynthTypes.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/BuiltinTypes.h"
#include "mlir/IR/DialectImplementation.h"

using namespace circt;
using namespace synth;

//===----------------------------------------------------------------------===//
// Dialect specification.
//===----------------------------------------------------------------------===//

void SynthDialect::initialize() {
  registerTypes();
  // Register operations.
  registerAttributes(); //TODO: probably this also needed?
  addOperations<
#define GET_OP_LIST
#include "circt/Dialect/Synth/IR/Synth.cpp.inc"
      >();
}

#define GET_ATTRDEF_CLASSES
#include "circt/Dialect/Synth/IR/SynthDialect.cpp.inc"