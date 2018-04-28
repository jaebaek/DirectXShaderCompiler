//===--- TransFormAction.cpp - TransFormAction implementation -------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "clang/HLSLFuzzer/TransFormAction.h"

#include <cstdio>
#include <memory>
#include <sstream>
#include <string>

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

//------------------------------------------------------------------------------
// Clang rewriter sample. Demonstrates:
//
// * How to use RecursiveASTVisitor to find interesting AST nodes.
// * How to use the Rewriter API to rewrite the source code.
//
// Eli Bendersky (eliben@gmail.com)
// This code is in the public domain
//------------------------------------------------------------------------------

using namespace clang;

// By implementing RecursiveASTVisitor, we can specify which AST nodes
// we're interested in by overriding relevant methods.
class TransformASTVisitor : public RecursiveASTVisitor<TransformASTVisitor> {
public:
  TransformASTVisitor(Rewriter &R) : rewriter(R) {}

  bool VisitStmt(Stmt *s) {
    // Only care about If statements.
    if (isa<IfStmt>(s)) {
      IfStmt *IfStatement = cast<IfStmt>(s);
      Stmt *Then = IfStatement->getThen();

      rewriter.InsertText(Then->getLocStart(), "// the 'if' part\n", true,
                          true);

      Stmt *Else = IfStatement->getElse();
      if (Else)
        rewriter.InsertText(Else->getLocStart(), "// the 'else' part\n", true,
                            true);
    }

    return true;
  }

  bool VisitFunctionDecl(FunctionDecl *f) {
    // Only function definitions (with bodies), not declarations.
    if (f->hasBody()) {
      Stmt *FuncBody = f->getBody();

      // Type name as string
      QualType QT = f->getReturnType();
      std::string TypeStr = QT.getAsString();

      // Function name
      DeclarationName DeclName = f->getNameInfo().getName();
      std::string FuncName = DeclName.getAsString();

      // Add comment before
      std::stringstream SSBefore;
      SSBefore << "// Begin function " << FuncName << " returning " << TypeStr
               << "\n";
      SourceLocation ST = f->getSourceRange().getBegin();
      rewriter.InsertText(ST, SSBefore.str(), true, true);

      // And after
      std::stringstream SSAfter;
      SSAfter << "\n// End function " << FuncName;
      ST = FuncBody->getLocEnd().getLocWithOffset(1);
      rewriter.InsertText(ST, SSAfter.str(), true, true);
    }

    return true;
  }

private:
  Rewriter &rewriter;
};

// Implementation of the ASTConsumer interface for reading an AST produced
// by the Clang parser.
class TransformASTConsumer : public ASTConsumer {
public:
  TransformASTConsumer(Rewriter &R) : Visitor(R) {}

  // Override the method that gets called for each parsed top-level
  // declaration.
  virtual bool HandleTopLevelDecl(DeclGroupRef DR) {
    for (DeclGroupRef::iterator b = DR.begin(), e = DR.end(); b != e; ++b)
      // Traverse the declaration using our AST visitor.
      Visitor.TraverseDecl(*b);
    return true;
  }

private:
  TransformASTVisitor Visitor;
};

namespace clang {

std::unique_ptr<ASTConsumer>
TransFormAction::CreateASTConsumer(CompilerInstance &CI, StringRef InFile) {
  rewriter.setSourceMgr(CI.getSourceManager(), CI.getLangOpts());
  outStream = CI.getOutStream();
  return llvm::make_unique<TransformASTConsumer>(rewriter);
}

void TransFormAction::EndSourceFileAction() {
  rewriter.getEditBuffer(rewriter.getSourceMgr().getMainFileID())
      .write(*outStream);
}

} // end namespace clang
