//===- SynthAttributes.cpp - Implement Synth attributes -------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "circt/Dialect/Synth/IR/SynthAttributes.h"
#include "circt/Dialect/Synth/IR/SynthDialect.h"
#include "circt/Dialect/Synth/IR/SynthOps.h"
#include "circt/Dialect/Synth/IR/SynthTypes.h"
#include "circt/Support/LLVM.h"
#include "mlir/IR/DialectImplementation.h"
// #include "llvm/ADT/SmallPtrSet.h"
// #include "llvm/ADT/TypeSwitch.h"

using namespace circt;
using namespace circt::synth;

//===----------------------------------------------------------------------===//
// ODS Boilerplate
//===----------------------------------------------------------------------===//

#define GET_ATTRDEF_CLASSES
#include "circt/Dialect/Synth/IR/SynthAttributes.cpp.inc"

void SynthDialect::registerAttributes() {
  addAttributes<
#define GET_ATTRDEF_LIST
#include "circt/Dialect/Synth/IR/SynthAttributes.cpp.inc"
      >();
}

// //===----------------------------------------------------------------------===//
// // Synth Attribute Modification Helpers
// //===----------------------------------------------------------------------===//


unsigned synth::addSynthAttributes(Operation *op,
                                    ArrayRef<SynthAttributeAttr> newAttrs) {
    if (newAttrs.empty())
        return 0;
    unsigned numAdded = 0;
    //TODO: add function body
    return numAdded;
}