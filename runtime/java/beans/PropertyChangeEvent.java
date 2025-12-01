package java.beans;

public class PropertyChangeEvent extends java.util.EventObject {
    public PropertyChangeEvent(Object source, String propertyName, Object oldValue, Object newValue) {
        super(source);
    }
    public String getPropertyName() { return null; }
    public Object getNewValue() { return null; }
    public Object getOldValue() { return null; }
    public void setPropagationId(Object propagationId) {}
    public Object getPropagationId() { return null; }
}
