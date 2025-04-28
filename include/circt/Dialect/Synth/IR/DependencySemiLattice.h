//===- DependancySemiLattice.h - Synth dialect dependancy semi-lattice ---------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "circt/Dialect/Synth/IR/SynthAttributes.h"
#include "mlir/IR/Operation.h"

namespace circt {
namespace synth {
    class Dependencies {
    public:
        std::set<synth::DataDependencyEnum> dependencies;
        Dependencies();
        Dependencies(synth::DataDependencyEnum dependency);
        Dependencies(std::vector<synth::DataDependencyEnum> dependencies);
        Dependencies(std::set<synth::DataDependencyEnum> dependencies);
        synth::Dependencies leastUpperBound(Dependencies a, Dependencies b);
    };

    namespace dependenciesUtils {
        mlir::Attribute asAttribute(mlir::MLIRContext *context, Dependencies deps);
        Dependencies fromAttribute(DataDependencyEnumAttr attr);
        Dependencies fromOp(mlir::Operation op);
        // void markOp(Dependencies deps, mlir::Operation op);
        // void addDependanciesToOp (Dependencies deps, mlir::Operation op);
    }
} // namespace synth
} // namespace circt

