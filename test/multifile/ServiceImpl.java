// Service implementation - depends on Service interface

public class ServiceImpl implements Service {
    private String name;
    private int multiplier;

    public ServiceImpl(String name, int multiplier) {
        this.name = name;
        this.multiplier = multiplier;
    }

    public String getName() {
        return name;
    }

    public int compute(int value) {
        return value * multiplier;
    }
}
