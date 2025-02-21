//===- SynthAttributes.cpp - Implement Synth attributes -----------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "circt/Dialect/Synth/IR/SynthAttributes.h"
#include "circt/Dialect/Synth/IR/SynthDialect.h"
#include "circt/Dialect/Synth/IR/SynthTypes.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/Types.h"
#include "mlir/IR/DialectImplementation.h"
#include "llvm/ADT/TypeSwitch.h"

using namespace circt;
using namespace synth;

#include "circt/Dialect/Synth/IR/SynthEnums.cpp.inc"
#define GET_ATTRDEF_CLASSES
#include "circt/Dialect/Synth/IR/SynthAttributes.cpp.inc"

Type SynthEnumConstAttr::getType() { return synth::EnumType::get(getContext()); } // TODO: implement this. (see implementation in seq?)

void SynthDialect::registerAttributes() {
  addAttributes<
#define GET_ATTRDEF_LIST
#include "circt/Dialect/Synth/IR/SynthAttributes.cpp.inc"
      >();
}
