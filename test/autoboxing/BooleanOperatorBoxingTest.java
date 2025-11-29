// Test boxing/unboxing with boolean operators: &, |, ^, !
public class BooleanOperatorBoxingTest {
    static int passed = 0;
    static int failed = 0;

    static void test(String name, boolean condition) {
        if (condition) {
            passed++;
        } else {
            failed++;
            System.out.println("FAIL: " + name);
        }
    }

    public static void main(String[] args) {
        // === LOGICAL AND (&) - non-short-circuit ===
        // boolean & Boolean
        boolean b1 = true;
        Boolean B1 = Boolean.TRUE;
        test("true & Boolean.TRUE", (b1 & B1) == true);

        boolean b2 = true;
        Boolean B2 = Boolean.FALSE;
        test("true & Boolean.FALSE", (b2 & B2) == false);

        boolean b3 = false;
        Boolean B3 = Boolean.TRUE;
        test("false & Boolean.TRUE", (b3 & B3) == false);

        boolean b4 = false;
        Boolean B4 = Boolean.FALSE;
        test("false & Boolean.FALSE", (b4 & B4) == false);

        // Boolean & boolean
        Boolean B5 = Boolean.TRUE;
        boolean b5 = true;
        test("Boolean.TRUE & true", (B5 & b5) == true);

        Boolean B6 = Boolean.TRUE;
        boolean b6 = false;
        test("Boolean.TRUE & false", (B6 & b6) == false);

        Boolean B7 = Boolean.FALSE;
        boolean b7 = true;
        test("Boolean.FALSE & true", (B7 & b7) == false);

        Boolean B8 = Boolean.FALSE;
        boolean b8 = false;
        test("Boolean.FALSE & false", (B8 & b8) == false);

        // Boolean & Boolean
        test("Boolean.TRUE & Boolean.TRUE", (Boolean.TRUE & Boolean.TRUE) == true);
        test("Boolean.TRUE & Boolean.FALSE", (Boolean.TRUE & Boolean.FALSE) == false);
        test("Boolean.FALSE & Boolean.TRUE", (Boolean.FALSE & Boolean.TRUE) == false);
        test("Boolean.FALSE & Boolean.FALSE", (Boolean.FALSE & Boolean.FALSE) == false);

        // === LOGICAL OR (|) - non-short-circuit ===
        // boolean | Boolean
        test("true | Boolean.TRUE", (true | Boolean.TRUE) == true);
        test("true | Boolean.FALSE", (true | Boolean.FALSE) == true);
        test("false | Boolean.TRUE", (false | Boolean.TRUE) == true);
        test("false | Boolean.FALSE", (false | Boolean.FALSE) == false);

        // Boolean | boolean
        test("Boolean.TRUE | true", (Boolean.TRUE | true) == true);
        test("Boolean.TRUE | false", (Boolean.TRUE | false) == true);
        test("Boolean.FALSE | true", (Boolean.FALSE | true) == true);
        test("Boolean.FALSE | false", (Boolean.FALSE | false) == false);

        // Boolean | Boolean
        test("Boolean.TRUE | Boolean.TRUE", (Boolean.TRUE | Boolean.TRUE) == true);
        test("Boolean.TRUE | Boolean.FALSE", (Boolean.TRUE | Boolean.FALSE) == true);
        test("Boolean.FALSE | Boolean.TRUE", (Boolean.FALSE | Boolean.TRUE) == true);
        test("Boolean.FALSE | Boolean.FALSE", (Boolean.FALSE | Boolean.FALSE) == false);

        // === LOGICAL XOR (^) ===
        // boolean ^ Boolean
        test("true ^ Boolean.TRUE", (true ^ Boolean.TRUE) == false);
        test("true ^ Boolean.FALSE", (true ^ Boolean.FALSE) == true);
        test("false ^ Boolean.TRUE", (false ^ Boolean.TRUE) == true);
        test("false ^ Boolean.FALSE", (false ^ Boolean.FALSE) == false);

        // Boolean ^ boolean
        test("Boolean.TRUE ^ true", (Boolean.TRUE ^ true) == false);
        test("Boolean.TRUE ^ false", (Boolean.TRUE ^ false) == true);
        test("Boolean.FALSE ^ true", (Boolean.FALSE ^ true) == true);
        test("Boolean.FALSE ^ false", (Boolean.FALSE ^ false) == false);

        // Boolean ^ Boolean
        test("Boolean.TRUE ^ Boolean.TRUE", (Boolean.TRUE ^ Boolean.TRUE) == false);
        test("Boolean.TRUE ^ Boolean.FALSE", (Boolean.TRUE ^ Boolean.FALSE) == true);
        test("Boolean.FALSE ^ Boolean.TRUE", (Boolean.FALSE ^ Boolean.TRUE) == true);
        test("Boolean.FALSE ^ Boolean.FALSE", (Boolean.FALSE ^ Boolean.FALSE) == false);

        // === LOGICAL NOT (!) ===
        // !Boolean
        test("!Boolean.TRUE", (!Boolean.TRUE) == false);
        test("!Boolean.FALSE", (!Boolean.FALSE) == true);

        // Double negation
        test("!!Boolean.TRUE", (!!Boolean.TRUE) == true);
        test("!!Boolean.FALSE", (!!Boolean.FALSE) == false);

        // === SHORT-CIRCUIT OPERATORS WITH BOXING ===
        // boolean && Boolean
        test("true && Boolean.TRUE", (true && Boolean.TRUE) == true);
        test("true && Boolean.FALSE", (true && Boolean.FALSE) == false);
        test("false && Boolean.TRUE", (false && Boolean.TRUE) == false);
        test("false && Boolean.FALSE", (false && Boolean.FALSE) == false);

        // Boolean && boolean
        test("Boolean.TRUE && true", (Boolean.TRUE && true) == true);
        test("Boolean.TRUE && false", (Boolean.TRUE && false) == false);
        test("Boolean.FALSE && true", (Boolean.FALSE && true) == false);
        test("Boolean.FALSE && false", (Boolean.FALSE && false) == false);

        // Boolean && Boolean
        test("Boolean.TRUE && Boolean.TRUE", (Boolean.TRUE && Boolean.TRUE) == true);
        test("Boolean.TRUE && Boolean.FALSE", (Boolean.TRUE && Boolean.FALSE) == false);
        test("Boolean.FALSE && Boolean.TRUE", (Boolean.FALSE && Boolean.TRUE) == false);
        test("Boolean.FALSE && Boolean.FALSE", (Boolean.FALSE && Boolean.FALSE) == false);

        // boolean || Boolean
        test("true || Boolean.TRUE", (true || Boolean.TRUE) == true);
        test("true || Boolean.FALSE", (true || Boolean.FALSE) == true);
        test("false || Boolean.TRUE", (false || Boolean.TRUE) == true);
        test("false || Boolean.FALSE", (false || Boolean.FALSE) == false);

        // Boolean || boolean
        test("Boolean.TRUE || true", (Boolean.TRUE || true) == true);
        test("Boolean.TRUE || false", (Boolean.TRUE || false) == true);
        test("Boolean.FALSE || true", (Boolean.FALSE || true) == true);
        test("Boolean.FALSE || false", (Boolean.FALSE || false) == false);

        // Boolean || Boolean
        test("Boolean.TRUE || Boolean.TRUE", (Boolean.TRUE || Boolean.TRUE) == true);
        test("Boolean.TRUE || Boolean.FALSE", (Boolean.TRUE || Boolean.FALSE) == true);
        test("Boolean.FALSE || Boolean.TRUE", (Boolean.FALSE || Boolean.TRUE) == true);
        test("Boolean.FALSE || Boolean.FALSE", (Boolean.FALSE || Boolean.FALSE) == false);

        // === COMPOUND ASSIGNMENT OPERATORS ===
        // boolean &= Boolean
        boolean ca1 = true;
        ca1 &= Boolean.TRUE;
        test("boolean &= Boolean.TRUE", ca1 == true);

        boolean ca2 = true;
        ca2 &= Boolean.FALSE;
        test("boolean &= Boolean.FALSE", ca2 == false);

        // boolean |= Boolean
        boolean ca3 = false;
        ca3 |= Boolean.TRUE;
        test("boolean |= Boolean.TRUE", ca3 == true);

        boolean ca4 = false;
        ca4 |= Boolean.FALSE;
        test("boolean |= Boolean.FALSE", ca4 == false);

        // boolean ^= Boolean
        boolean ca5 = true;
        ca5 ^= Boolean.TRUE;
        test("boolean ^= Boolean.TRUE", ca5 == false);

        boolean ca6 = true;
        ca6 ^= Boolean.FALSE;
        test("boolean ^= Boolean.FALSE", ca6 == true);

        // === CONDITIONAL EXPRESSION WITH BOXING ===
        // boolean ? Boolean : Boolean
        Boolean result1 = true ? Boolean.TRUE : Boolean.FALSE;
        test("true ? Boolean.TRUE : Boolean.FALSE", result1.booleanValue() == true);

        Boolean result2 = false ? Boolean.TRUE : Boolean.FALSE;
        test("false ? Boolean.TRUE : Boolean.FALSE", result2.booleanValue() == false);

        // Boolean ? boolean : boolean
        boolean result3 = Boolean.TRUE ? true : false;
        test("Boolean.TRUE ? true : false", result3 == true);

        boolean result4 = Boolean.FALSE ? true : false;
        test("Boolean.FALSE ? true : false", result4 == false);

        // === COMPLEX EXPRESSIONS ===
        // Chained operations
        Boolean chain1 = Boolean.TRUE;
        Boolean chain2 = Boolean.FALSE;
        Boolean chain3 = Boolean.TRUE;
        test("Boolean.TRUE & Boolean.FALSE | Boolean.TRUE", ((chain1 & chain2) | chain3) == true);
        test("Boolean.TRUE | Boolean.FALSE & Boolean.TRUE", (chain1 | (chain2 & chain3)) == true);
        test("Boolean.TRUE ^ Boolean.FALSE ^ Boolean.TRUE", ((chain1 ^ chain2) ^ chain3) == false);

        // Mixed with primitives
        boolean prim1 = true;
        boolean prim2 = false;
        test("boolean & Boolean | boolean", ((prim1 & Boolean.FALSE) | prim2) == false);
        test("Boolean | boolean & Boolean", (Boolean.TRUE | (prim2 & Boolean.TRUE)) == true);

        // De Morgan's laws with boxing
        Boolean dm1 = Boolean.TRUE;
        Boolean dm2 = Boolean.FALSE;
        test("!(Boolean & Boolean) == !Boolean | !Boolean", (!(dm1 & dm2)) == ((!dm1) | (!dm2)));
        test("!(Boolean | Boolean) == !Boolean & !Boolean", (!(dm1 | dm2)) == ((!dm1) & (!dm2)));

        System.out.println("Passed: " + passed + ", Failed: " + failed);
        if (failed > 0) {
            System.exit(1);
        }
        System.out.println("PASS: BooleanOperatorBoxingTest");
    }
}
