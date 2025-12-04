// Reproducer for stack overflow in Format hierarchy
// Mimics java.text.Format -> DateFormat -> SimpleDateFormat

abstract class Format {
    public final String format(Object obj) {
        StringBuffer sb = new StringBuffer();
        format(obj, sb);
        return sb.toString();
    }
    
    public abstract StringBuffer format(Object obj, StringBuffer sb);
}

abstract class DateFormat extends Format {
    @Override
    public final StringBuffer format(Object obj, StringBuffer sb) {
        // Delegate to Date-specific format
        return format((java.util.Date) obj, sb);
    }
    
    public final String format(java.util.Date date) {
        StringBuffer sb = new StringBuffer();
        format(date, sb);
        return sb.toString();
    }
    
    public abstract StringBuffer format(java.util.Date date, StringBuffer sb);
}

class SimpleDateFormat extends DateFormat {
    @Override
    public StringBuffer format(java.util.Date date, StringBuffer sb) {
        sb.append("formatted date");
        return sb;
    }
}

public class FormatHierarchyTest {
    public static void main(String[] args) {
        SimpleDateFormat sdf = new SimpleDateFormat();
        String result = sdf.format(new java.util.Date());
        System.out.println("Result: " + result);
    }
}
