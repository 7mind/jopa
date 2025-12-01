package java.awt;

public abstract class Component implements java.awt.image.ImageObserver, java.awt.MenuContainer, java.io.Serializable {
    public boolean imageUpdate(java.awt.Image img, int infoflags, int x, int y, int width, int height) {
        return false;
    }
    public void addPropertyChangeListener(java.beans.PropertyChangeListener listener) {}
    public void removePropertyChangeListener(java.beans.PropertyChangeListener listener) {}
}
