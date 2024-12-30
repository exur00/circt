//===- SynthAttributes.h - Declare Synth dialect attributes ------------*- C++-*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef CIRCT_DIALECT_SYNTH_SYNTHATTRIBUTES_H
#define CIRCT_DIALECT_SYNTH_SYNTHATTRIBUTES_H


#include "mlir/IR/Attributes.h"
// #include "mlir/IR/BuiltinAttributes.h"
// #include "llvm/ADT/ArrayRef.h"
// #include "llvm/ADT/STLExtras.h"
// #include "llvm/ADT/SmallVector.h"

//#include "circt/Dialect/SV/SVEnums.h.inc"
#define GET_ATTRDEF_CLASSES
#include "circt/Dialect/Synth/IR/SynthAttributes.h.inc"

namespace circt {
namespace synth {

/// Add a list of Synth attributes to an operation. The attributes are deduplicated
/// such that only one copy of each attribute is kept on the operation. Returns
/// the number of attributes that were added.
unsigned addSynthAttributes(mlir::Operation *op,
                         llvm::ArrayRef<SynthAttributeAttr> attrs);

} // namespace synth
} // namespace circt

#endif // CIRCT_DIALECT_SYNTH_SYNTHATTRIBUTES_H