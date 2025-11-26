JIKESPG = @JIKESPG@
JAVAROOT = .

check_JAVA = bnfact.java \
              bnfdcl.java \
              bnfdef.java \
              bnfhdr.java \
              bnfprs.java \
              bnfsym.java \
              Ast.java \
              Token.java \
              LexStream.java \
              Main.java \
              Option.java \
              Parser.java \
              Scanner.java

bnfact.java bnfdcl.java bnfdef.java bnfhdr.java bnfprs.java bnfsym.java : bnf.g
	$(JIKESPG) bnf.g

EXTRA_DIST = bnf.g \
             example1.bnf \
             example2.bnf

CLEANFILES = bnf.l

MAINTAINERCLEANFILES = bnfact.java \
                       bnfdcl.java \
                       bnfdef.java \
                       bnfhdr.java \
                       bnfprs.java \
                       bnfsym.java
