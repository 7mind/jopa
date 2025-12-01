package java.util;

public class Stack<E> extends Vector<E> {
    public Stack() {}
    public E push(E item) { return item; }
    public E pop() { return null; }
    public E peek() { return null; }
    public boolean empty() { return false; }
    public int search(Object o) { return 0; }
}
