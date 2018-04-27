//===-- TransFormAction.h - FrontendAction for Emitting SPIR-V --*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#ifndef LLVM_CLANG_HLSLFUZZER_FRONTENDACTION_H
#define LLVM_CLANG_HLSLFUZZER_FRONTENDACTION_H

#include "clang/Frontend/FrontendAction.h"
#include "clang/Rewrite/Core/Rewriter.h"

#include <string>

class Transformation;
class TransformationManager;

namespace clang {

class TransFormAction : public ASTFrontendAction {
public:
  TransFormAction(std::string transform, int Counter);

protected:
  std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI,
                                                 StringRef InFile) override;
  void EndSourceFileAction() override;
private:
  TransformationManager *TransMgr;
  Transformation        *transformation;
  llvm::raw_ostream     *outStream;
};

} // end namespace clang

#endif
