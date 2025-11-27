package java.lang;

public class Package {
    private Package() {}

    public native String getName();
    public native String getSpecificationTitle();
    public native String getSpecificationVersion();
    public native String getSpecificationVendor();
    public native String getImplementationTitle();
    public native String getImplementationVersion();
    public native String getImplementationVendor();
    public native boolean isSealed();
}
