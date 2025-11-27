JIKESPG = @JIKESPG@
JAVAROOT = .

check_JAVA = expract.java \
             exprhdr.java \
             exprdcl.java \
             exprsym.java \
             exprdef.java \
             exprprs.java \
             Ast.java \
             AstBinary.java \
             AstMinus.java \
             AstNegativeNumber.java \
             AstNumber.java \
             AstParen.java \
             AstPlus.java \
             AstSlash.java \
             AstStar.java \
             LexStream.java \
             Main.java \
             Option.java \
             Parser.java \
             Scanner.java \
             Token.java

expract.java exprhdr.java exprdcl.java exprsym.java exprdef.java exprprs.java : expr.g
	$(JIKESPG) expr.g

EXTRA_DIST = expr.g

CLEANFILES = expr.l

MAINTAINERCLEANFILES = expract.java \
                       exprhdr.java \
                       exprdcl.java \
                       exprsym.java \
                       exprdef.java \
                       exprprs.java

