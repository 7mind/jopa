%Options gp=java,act,tab,an=expract.java,hn=exprhdr.java,tab=space,fp=expr,prefix=TK_,
%Options nogoto-default,output-size=125,name=max,error-maps
%Define

-- This software is subject to the terms of the IBM Jikes Parser
-- Generator License Agreement available at the following URL:
-- http://www.ibm.com/research/jikes.
-- Copyright (C) 1983, 1999, International Business Machines Corporation
-- and others.  All Rights Reserved.
-- You must accept the terms of that agreement to use this software.

-- This grammar has been augmented with productions that captures
-- most errors that a user is likely to make. This saves the need
-- to have an error recovery system.
-- 
-- This macro is used to initialize the rule_action array
-- to the null_action function.
--
%null_action
/.
        new NullAction(),
./
 
-- 
-- This macro is used to initialize the rule_action array
-- to the no_action function.
--
%no_action
/.
        new NoAction(),
./
 
%Terminals

    PLUS MINUS STAR SLASH NUMBER LPAREN RPAREN
    EOF ERROR

%Alias

    '+'  ::= PLUS
    '-'  ::= MINUS
    '*'  ::= STAR
    '/'  ::= SLASH
    '('  ::= LPAREN
    ')'  ::= RPAREN

    %EOF   ::= EOF
    %ERROR ::= ERROR

%Rules
/:
class exprhdr extends expract
{
    Action rule_action[] = {
        null, // no element 0
:/

/.
class expract
{
    Parser parser;

    expract(Parser parser)
    {
        this.parser = parser;
    }

    interface Action
    {
        public void action();
    }

    final class NoAction implements Action
    {
        public void action() {}
    }

    final class NullAction implements Action
    {
        public void action() { parser.setSYM1(null); }
    }

./

Goal ::= %empty
  /:%null_action:/

Goal ::= Expression
  /:%no_action:/

Expression ::= Expression '+' Term
  /:        new act%rule_number(),:/
  /.
    // 
    // Rule %rule_number:  %rule_text
    //
    final class act%rule_number implements Action
    {
        public void action()
        {
            AstPlus node = new AstPlus();
            node.left  = parser.SYM(1);
            node.op    = parser.TOKEN(2);
            node.right = parser.SYM(3);

            parser.setSYM1(node);
            return;
        }
    }
  ./

Expression ::= Expression '-' Term
  /:        new act%rule_number(),:/
  /.
    // 
    // Rule %rule_number:  %rule_text
    //
    final class act%rule_number implements Action
    {
        public void action()
        {
            AstMinus node = new AstMinus();
            node.left  = parser.SYM(1);
            node.op    = parser.TOKEN(2);
            node.right = parser.SYM(3);

            parser.setSYM1(node);
        }
    }
  ./

Expression ::= Term
  /:%no_action:/

Term ::= Term '*' Factor
  /:        new act%rule_number(),:/
  /.
    // 
    // Rule %rule_number:  %rule_text
    //
    final class act%rule_number implements Action
    {
        public void action()
        {
            AstStar node = new AstStar();
            node.left  = parser.SYM(1);
            node.op    = parser.TOKEN(2);
            node.right = parser.SYM(3);

            parser.setSYM1(node);
        }
    }
  ./

Term ::= Term '/' Factor
  /:        new act%rule_number(),:/
  /.
    // 
    // Rule %rule_number:  %rule_text
    //
    final class act%rule_number implements Action
    {
        public void action()
        {
            AstSlash node = new AstSlash();
            node.left  = parser.SYM(1);
            node.op    = parser.TOKEN(2);
            node.right = parser.SYM(3);

            parser.setSYM1(node);
        }
    }
  ./

Term ::= Factor
  /:%no_action:/

Factor ::= NUMBER
  /:        new act%rule_number(),:/
  /.
    // 
    // Rule %rule_number:  %rule_text
    //
    final class act%rule_number implements Action
    {
        public void action()
        {
            AstNumber node = new AstNumber();
            node.token = parser.TOKEN(1);
            node.value = Integer.parseInt(parser.lex_stream.Name(node.token));

            parser.setSYM1(node);
        }
    }
  ./

Factor ::= '-' NUMBER
  /:        new act%rule_number(),:/
  /.
    // 
    // Rule %rule_number:  %rule_text
    //
    final class act%rule_number implements Action
    {
        public void action()
        {
            AstNegativeNumber node = new AstNegativeNumber();
            node.op = parser.TOKEN(1);
            node.token = parser.TOKEN(2);
            node.value = - Integer.parseInt(parser.lex_stream.Name(node.token));

            parser.setSYM1(node);
        }
    }
  ./

Factor ::= '(' Expression ')'
  /:        new act%rule_number(),:/
  /.
    // 
    // Rule %rule_number:  %rule_text
    //
    final class act%rule_number implements Action
    {
        public void action()
        {
            AstParen node = new AstParen();
            node.expression = parser.SYM(2);

            parser.setSYM1(node);
        }
    }
  ./

/:
    };

    exprhdr(Parser parser)
    {
        super(parser);
    }
}
:/

/.
}
./
%End


