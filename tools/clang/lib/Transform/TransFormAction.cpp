//===--- TransFormAction.cpp - TransFormAction implementation -------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "clang/Transform/TransFormAction.h"

#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Basic/Diagnostic.h"
#include "clang/Basic/FileManager.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Basic/TargetInfo.h"
#include "clang/Basic/TargetOptions.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Parse/ParseAST.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/Rewrite/Frontend/Rewriters.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/raw_ostream.h"

#include "clang/Basic/Specifiers.h"
#include "clang/Basic/IdentifierTable.h"
#include "clang/AST/ASTContext.h"

#include "TransformationManager.h"
#include "Transformation.h"
#include "SimplifyIf.h"

namespace {

template<typename TransformationClass>
class DoubleCheckRegisterTransformation {

public:
  DoubleCheckRegisterTransformation(const char *TransName, const char *TransDesc)
    : Name(TransName), Desc(TransDesc) {}

  void doubleCheck() {
    Transformation *TransImpl = new TransformationClass(Name, Desc);
    assert(TransImpl && "Fail to create TransformationClass");
    TransformationManager::registerTransformation(Name, TransImpl);
  }

private:
  const char *Name;
  const char *Desc;

};

const char *SimplifyIfDescriptionMsg =
"Simplify an if-else statement. It transforms the following code: \n\
  if (guard1) \n\
  {... } \n\
  else if (guard2) \n\
  else \n\
  {...} \n\
to \n\
  (guard1) \n\
  {... } \n\
  if (guard2) \n\
  else \n\
  {...} \n\
if there is no else-if left, the last else keyword will be removed. \n";  

static DoubleCheckRegisterTransformation<SimplifyIf>
         SimplifyIfTrans("simplify-if", SimplifyIfDescriptionMsg);

} // end namespace

namespace clang {

TransFormAction::TransFormAction(std::string transform, int Counter) {
  SimplifyIfTrans.doubleCheck();

  TransMgr = TransformationManager::GetInstance();
  TransMgr->setTransformation(transform);
  transformation = TransMgr->getTransformation();
  transformation->setTransformationCounter(Counter);
}

std::unique_ptr<ASTConsumer>
TransFormAction::CreateASTConsumer(CompilerInstance &CI, StringRef InFile) {
  outStream = CI.getOutStream();
  return std::unique_ptr<ASTConsumer>(transformation);
}

void TransFormAction::EndSourceFileAction() {
  transformation->outputTransformedSource(*outStream);
}

} // end namespace clang
