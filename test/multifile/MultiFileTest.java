// Multi-file compilation test - depends on Service and ServiceImpl

public class MultiFileTest {
    static int passed = 0;
    static int failed = 0;

    static void test(String name, boolean condition) {
        if (condition) {
            passed++;
            System.out.println("PASS: " + name);
        } else {
            failed++;
            System.out.println("FAIL: " + name);
        }
    }

    public static void main(String[] args) {
        System.out.println("=== Multi-File Compilation Test ===");

        // Test 1: Create service implementation
        ServiceImpl impl = new ServiceImpl("TestService", 10);
        test("1.1 Create ServiceImpl", impl != null);

        // Test 2: Call getName through impl
        test("1.2 ServiceImpl.getName()", "TestService".equals(impl.getName()));

        // Test 3: Call compute through impl
        test("1.3 ServiceImpl.compute()", impl.compute(5) == 50);

        // Test 4: Use through interface reference
        Service service = impl;
        test("1.4 Assign to Service interface", service != null);

        // Test 5: Call through interface
        test("1.5 Service.getName()", "TestService".equals(service.getName()));
        test("1.6 Service.compute()", service.compute(3) == 30);

        // Test 7: Create another instance
        Service service2 = new ServiceImpl("AnotherService", 2);
        test("1.7 Second ServiceImpl", service2.compute(10) == 20);

        System.out.println("\n=== RESULTS ===");
        System.out.println("Passed: " + passed);
        System.out.println("Failed: " + failed);

        if (failed > 0) {
            System.exit(1);
        }
    }
}
