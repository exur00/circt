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
    Dependencies::Dependencies(std::vector<synth::DataDependencyEnum> &dependencies) {
        this->dependencies = {};
        this->dependencies.insert(dependencies.begin(), dependencies.end());
    }
    Dependencies::Dependencies(std::set<synth::DataDependencyEnum> dependencies) {
        this->dependencies = {};
        this->dependencies.insert(dependencies.begin(), dependencies.end());
    }
    std::string Dependencies::toString() const{
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
        result.insert(b.dependencies.begin(), b.dependencies.end());
        return Dependencies(result);
    }

    mlir::Attribute dependenciesUtils::asAttribute(mlir::MLIRContext *context, Dependencies deps) {
        std::vector<mlir::Attribute> attrs = {};
        for (auto depEnum : deps.dependencies) {
            attrs.push_back(synth::DataDependencyEnumAttr::get(context, depEnum));
        }
        mlir::ArrayRef<mlir::Attribute> arrRef = mlir::ArrayRef(attrs);
        auto arrayAttr = mlir::ArrayAttr::get(context, arrRef);
        return arrayAttr;
    }

    Dependencies dependenciesUtils::fromAttribute(DataDependencyEnumAttr attr) { //TODO: add function that also does cast?
        DataDependencyEnum e = attr.getValue();
        return Dependencies(e);
    }

    //TODO: update when dataDependenciesAttr contains list
    Dependencies dependenciesUtils::fromAttribute(DataDependenciesAttr attr) { //TODO: add function that also does cast?
        DataDependencyEnumAttr e = attr.getDataDepEnum();
        return fromAttribute(e);
    }

    Dependencies dependenciesUtils::fromAttributeArray(mlir::ArrayAttr attr) { //TODO: add function that also does cast?
        //auto attrs = attr.getValue();
        //std::vector<mlir::Attribute> vec = std::vector(attrs.begin(), attrs.end());
        std::vector<DataDependencyEnum> vec = {};
        for (auto dataAttr : attr.getValue()) {
            //vec.push_back(fromAttribute(dataAttr));
            DataDependencyEnumAttr castAttr = llvm::dyn_cast<DataDependencyEnumAttr>(dataAttr);
            vec.push_back(castAttr.getValue());
        }
        return Dependencies(vec);
    }

    std::optional<Dependencies> dependenciesUtils::fromOp(mlir::Operation *op) {
        mlir::Attribute attr = op->getDiscardableAttr("synth.dataDep");
        if (!attr) {return std::nullopt;}
        auto castArrayAttr = llvm::dyn_cast<mlir::ArrayAttr>(attr); // TODO: this does not succeed, ik denk dat het komt doordat het een lijst is?
        if (castArrayAttr) {return fromAttributeArray(castArrayAttr);}
        auto castDataEnumAttr = llvm::dyn_cast<DataDependencyEnumAttr>(attr);
        if (castDataEnumAttr) {return fromAttribute(castDataEnumAttr);}
        auto castDataAttr = llvm::dyn_cast<DataDependenciesAttr>(attr);
        if (castDataAttr) {return fromAttribute(castDataAttr);}
        return std::nullopt;
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