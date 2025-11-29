// Test boxing/unboxing with numeric bitwise operators: &, |, ^, ~
public class NumericBitwiseBoxingTest {
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
        // === BITWISE AND (&) ===
        // int & Integer
        int i1 = 0xFF;
        Integer I1 = Integer.valueOf(0x0F);
        test("int & Integer", (i1 & I1) == 0x0F);
        test("Integer & int", (I1 & i1) == 0x0F);

        // Integer & Integer
        Integer I2 = Integer.valueOf(0xF0);
        test("Integer & Integer", (I1 & I2) == 0);

        // long & Long
        long l1 = 0xFFFFL;
        Long L1 = Long.valueOf(0x00FFL);
        test("long & Long", (l1 & L1) == 0x00FFL);
        test("Long & long", (L1 & l1) == 0x00FFL);

        // Long & Long
        Long L2 = Long.valueOf(0xFF00L);
        test("Long & Long", (L1 & L2) == 0L);

        // byte & Byte
        byte b1 = 0x0F;
        Byte B1 = Byte.valueOf((byte)0x07);
        test("byte & Byte", (b1 & B1) == 0x07);
        test("Byte & byte", (B1 & b1) == 0x07);

        // short & Short
        short s1 = 0xFF;
        Short S1 = Short.valueOf((short)0x0F);
        test("short & Short", (s1 & S1) == 0x0F);
        test("Short & short", (S1 & s1) == 0x0F);

        // === BITWISE OR (|) ===
        // int | Integer
        int i3 = 0xF0;
        Integer I3 = Integer.valueOf(0x0F);
        test("int | Integer", (i3 | I3) == 0xFF);
        test("Integer | int", (I3 | i3) == 0xFF);

        // Integer | Integer
        test("Integer | Integer", (I1 | I2) == 0xF0);

        // long | Long
        long l3 = 0xF000L;
        Long L3 = Long.valueOf(0x0F00L);
        test("long | Long", (l3 | L3) == 0xFF00L);
        test("Long | long", (L3 | l3) == 0xFF00L);

        // Long | Long
        test("Long | Long", (L1 | L2) == 0xFFFFL);

        // byte | Byte
        byte b2 = 0x30;
        Byte B2 = Byte.valueOf((byte)0x03);
        test("byte | Byte", (b2 | B2) == 0x33);
        test("Byte | byte", (B2 | b2) == 0x33);

        // short | Short
        short s2 = (short)0xF00;
        Short S2 = Short.valueOf((short)0x0F0);
        test("short | Short", (s2 | S2) == 0xFF0);
        test("Short | short", (S2 | s2) == 0xFF0);

        // === BITWISE XOR (^) ===
        // int ^ Integer
        int i4 = 0xFF;
        Integer I4 = Integer.valueOf(0x0F);
        test("int ^ Integer", (i4 ^ I4) == 0xF0);
        test("Integer ^ int", (I4 ^ i4) == 0xF0);

        // Integer ^ Integer
        Integer I5 = Integer.valueOf(0xFF);
        test("Integer ^ Integer", (I4 ^ I5) == 0xF0);

        // long ^ Long
        long l4 = 0xFFFF_FFFFL;
        Long L4 = Long.valueOf(0x0000_FFFFL);
        test("long ^ Long", (l4 ^ L4) == 0xFFFF_0000L);
        test("Long ^ long", (L4 ^ l4) == 0xFFFF_0000L);

        // Long ^ Long
        Long L5 = Long.valueOf(0xFFFF_0000L);
        test("Long ^ Long", (L4 ^ L5) == 0xFFFF_FFFFL);

        // byte ^ Byte
        byte b3 = (byte)0xFF;
        Byte B3 = Byte.valueOf((byte)0x0F);
        test("byte ^ Byte", ((byte)(b3 ^ B3)) == (byte)0xF0);
        test("Byte ^ byte", ((byte)(B3 ^ b3)) == (byte)0xF0);

        // short ^ Short
        short s3 = (short)0xFFFF;
        Short S3 = Short.valueOf((short)0x00FF);
        test("short ^ Short", ((short)(s3 ^ S3)) == (short)0xFF00);
        test("Short ^ short", ((short)(S3 ^ s3)) == (short)0xFF00);

        // === BITWISE NOT (~) ===
        // ~Integer
        Integer I6 = Integer.valueOf(0);
        test("~Integer", (~I6) == -1);

        // ~Long
        Long L6 = Long.valueOf(0L);
        test("~Long", (~L6) == -1L);

        // ~Byte (result is int)
        Byte B4 = Byte.valueOf((byte)0);
        test("~Byte", (~B4) == -1);

        // ~Short (result is int)
        Short S4 = Short.valueOf((short)0);
        test("~Short", (~S4) == -1);

        // === COMPOUND ASSIGNMENT OPERATORS ===
        // int &= Integer
        int ci1 = 0xFF;
        Integer CI1 = Integer.valueOf(0x0F);
        ci1 &= CI1;
        test("int &= Integer", ci1 == 0x0F);

        // Integer &= int (Integer is unboxed, operated, then we can't assign back to Integer without explicit boxing)
        // This test verifies the expression works, not compound assignment to Integer
        int ci2 = 0xFF;
        test("(Integer &= ...) via temp", ((ci2 & I1.intValue()) == 0x0F));

        // int |= Integer
        int ci3 = 0xF0;
        Integer CI3 = Integer.valueOf(0x0F);
        ci3 |= CI3;
        test("int |= Integer", ci3 == 0xFF);

        // int ^= Integer
        int ci4 = 0xFF;
        Integer CI4 = Integer.valueOf(0x0F);
        ci4 ^= CI4;
        test("int ^= Integer", ci4 == 0xF0);

        // long &= Long
        long cl1 = 0xFFFFL;
        Long CL1 = Long.valueOf(0x00FFL);
        cl1 &= CL1;
        test("long &= Long", cl1 == 0x00FFL);

        // long |= Long
        long cl2 = 0xF000L;
        Long CL2 = Long.valueOf(0x0F00L);
        cl2 |= CL2;
        test("long |= Long", cl2 == 0xFF00L);

        // long ^= Long
        long cl3 = 0xFFFFL;
        Long CL3 = Long.valueOf(0x00FFL);
        cl3 ^= CL3;
        test("long ^= Long", cl3 == 0xFF00L);

        // === MIXED TYPE PROMOTION ===
        // int & Long (int promoted to long)
        int mi1 = 0xFF;
        Long ML1 = Long.valueOf(0x0FL);
        test("int & Long", (mi1 & ML1) == 0x0FL);

        // Integer & long
        Integer MI1 = Integer.valueOf(0xFF);
        long ml1 = 0x0FL;
        test("Integer & long", (MI1 & ml1) == 0x0FL);

        // byte & Integer (byte promoted to int)
        byte mb1 = 0x0F;
        Integer MBI1 = Integer.valueOf(0x07);
        test("byte & Integer", (mb1 & MBI1) == 0x07);

        // Short | Long (short promoted to long)
        Short MS1 = Short.valueOf((short)0xF0);
        Long ML2 = Long.valueOf(0x0FL);
        test("Short | Long", (MS1 | ML2) == 0xFFL);

        System.out.println("Passed: " + passed + ", Failed: " + failed);
        if (failed > 0) {
            System.exit(1);
        }
        System.out.println("PASS: NumericBitwiseBoxingTest");
    }
}
