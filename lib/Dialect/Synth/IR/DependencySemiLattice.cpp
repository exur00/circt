//===- DependancySemiLattice.h - Synth dialect dependancy semi-lattice ---------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "circt/Dialect/Synth/IR/DependencySemiLattice.h"
#include "circt/Dialect/Synth/IR/SynthAttributes.h"
#include "mlir/IR/Operation.h"
#include "llvm/Support/Casting.h"

namespace circt {
namespace synth {
    Dependencies::Dependencies() {
        dependencies = {};
    }
    Dependencies::Dependencies(synth::DataDependencyEnum dependency) {
        dependencies = {dependency};
    }
    Dependencies::Dependencies(std::vector<synth::DataDependencyEnum> dependencies) {
        this->dependencies = {};
        this->dependencies.insert(dependencies.begin(), dependencies.end());
    }
    Dependencies::Dependencies(std::set<synth::DataDependencyEnum> dependencies) {
        this->dependencies = {};
        this->dependencies.insert(dependencies.begin(), dependencies.end());
    }
    std::string Dependencies::toString(){
        std::string result = "[";
        for (synth::DataDependencyEnum dep: dependencies) {
            result += stringifyDataDependencyEnum(dep).str();
            result += ",";
        }
        return (result + "]");
    }
    synth::Dependencies Dependencies::leastUpperBound(Dependencies a, Dependencies b) {
        std::set<synth::DataDependencyEnum> result = {};
        result.insert(a.dependencies.begin(), a.dependencies.end());
        result.insert(b.dependencies.begin(), a.dependencies.end());
        return Dependencies(result);
    }

    mlir::Attribute dependenciesUtils::asAttribute(mlir::MLIRContext *context, Dependencies deps) {
        auto depEnum = *deps.dependencies.begin(); // TODO: replace to make list attribute, and put all of them in
        auto enumAttr = synth::DataDependencyEnumAttr::get(context, depEnum);
        return enumAttr;
    }

    Dependencies dependenciesUtils::fromAttribute(DataDependencyEnumAttr attr) { //TODO: add function that also does cast?
        DataDependencyEnum e = attr.getValue();
        return Dependencies(e);
    }

    std::optional<Dependencies> dependenciesUtils::fromOp(mlir::Operation *op) {
        mlir::Attribute attr = op->getDiscardableAttr("synth.dataDep");
        if (!attr) {return std::nullopt;}
        auto castAttr = llvm::dyn_cast<DataDependencyEnumAttr>(attr);
        if (!castAttr) {return std::nullopt;}
        return fromAttribute(castAttr);
    }

    // void markOp(Dependencies deps, mlir::Operation op) {

    // }

    // void addDependanciesToOp (Dependencies deps, mlir::Operation op) {
    //     Dependencies currentDeps = fromOp(op);
    //     Dependencies newDeps = Dependencies::leastUpperBound(currentDeps, deps);
    //     markOp(newDeps, op);
    // }
    
} // namespace synth
} // namespace circt