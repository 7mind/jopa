package javax.tools;
import java.util.List;
import java.util.ArrayList;
public class DiagnosticCollector<S> implements DiagnosticListener<S> {
    public void report(Diagnostic<? extends S> diagnostic) {}
    public List<Diagnostic<? extends S>> getDiagnostics() { return new ArrayList<Diagnostic<? extends S>>(); }
}
