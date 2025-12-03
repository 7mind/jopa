package javax.swing;

import java.awt.Component;

public class JOptionPane extends JComponent {
    public static final int DEFAULT_OPTION = -1;
    public static final int YES_NO_OPTION = 0;
    public static final int YES_NO_CANCEL_OPTION = 1;
    public static final int OK_CANCEL_OPTION = 2;

    public static final int YES_OPTION = 0;
    public static final int NO_OPTION = 1;
    public static final int CANCEL_OPTION = 2;
    public static final int OK_OPTION = 0;
    public static final int CLOSED_OPTION = -1;

    public static final int ERROR_MESSAGE = 0;
    public static final int INFORMATION_MESSAGE = 1;
    public static final int WARNING_MESSAGE = 2;
    public static final int QUESTION_MESSAGE = 3;
    public static final int PLAIN_MESSAGE = -1;

    public static String showInputDialog(Object message) {
        throw new UnsupportedOperationException();
    }

    public static String showInputDialog(Object message, Object initialSelectionValue) {
        throw new UnsupportedOperationException();
    }

    public static String showInputDialog(Component parentComponent, Object message) {
        throw new UnsupportedOperationException();
    }

    public static String showInputDialog(Component parentComponent, Object message, Object initialSelectionValue) {
        throw new UnsupportedOperationException();
    }

    public static String showInputDialog(Component parentComponent, Object message, String title, int messageType) {
        throw new UnsupportedOperationException();
    }

    public static void showMessageDialog(Component parentComponent, Object message) {
        throw new UnsupportedOperationException();
    }

    public static void showMessageDialog(Component parentComponent, Object message, String title, int messageType) {
        throw new UnsupportedOperationException();
    }

    public static int showConfirmDialog(Component parentComponent, Object message) {
        throw new UnsupportedOperationException();
    }

    public static int showConfirmDialog(Component parentComponent, Object message, String title, int optionType) {
        throw new UnsupportedOperationException();
    }

    public static int showConfirmDialog(Component parentComponent, Object message, String title, int optionType, int messageType) {
        throw new UnsupportedOperationException();
    }
}
