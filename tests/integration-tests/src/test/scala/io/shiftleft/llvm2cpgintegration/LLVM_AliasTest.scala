package io.shiftleft.llvm2cpgintegration

import io.shiftleft.codepropertygraph.cpgloading.CpgLoader
import io.shiftleft.semanticcpg.language._

class LLVM_AliasTest extends CPGMatcher {
  private val cpg = CpgLoader.load(TestCpgPaths.LLVM_AliasTestCpg)
  private val methodName = "aliases"

  "types" in {
    validateTypes(cpg, Set("ANY", "i32*"))
  }

  "AST" in {
  /*
    @x = global i32 0
    @y = alias i32, i32* @x

    define i32* @aliases() {
      ret i32* @y
    }
  */

    val method = cpg.method.name(methodName).head
    val ret = method.start.block.astChildren.isReturnNode.head
    val aliasRef = ret.start.astChildren.isIdentifier.head
    aliasRef.code shouldBe "y"
  }
}