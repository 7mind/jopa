// Test enum switch with top-level enum (different from nested enum)
enum Direction { NORTH, SOUTH, EAST, WEST }

public class EnumTopLevelSwitchTest {
    static int dirToNum(Direction d) {
        switch (d) {
            case NORTH: return 0;
            case SOUTH: return 1;
            case EAST: return 2;
            case WEST: return 3;
            default: return -1;
        }
    }

    public static void main(String[] args) {
        boolean passed = true;

        if (dirToNum(Direction.NORTH) != 0) {
            System.out.println("FAIL: NORTH");
            passed = false;
        }
        if (dirToNum(Direction.SOUTH) != 1) {
            System.out.println("FAIL: SOUTH");
            passed = false;
        }
        if (dirToNum(Direction.EAST) != 2) {
            System.out.println("FAIL: EAST");
            passed = false;
        }
        if (dirToNum(Direction.WEST) != 3) {
            System.out.println("FAIL: WEST");
            passed = false;
        }

        if (passed) {
            System.out.println("Top-level enum switch test passed!");
        } else {
            System.exit(1);
        }
    }
}
