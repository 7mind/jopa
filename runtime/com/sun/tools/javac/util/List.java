package com.sun.tools.javac.util;

import java.util.Iterator;

public class List<A> implements Iterable<A> {
    public A head;
    public List<A> tail;

    public A first() { return head; }

    public static <A> List<A> nil() { return new List<A>(); }

    @SafeVarargs
    public static <A> List<A> of(A... items) {
        return from(items);
    }

    @SafeVarargs
    public static <A> List<A> from(A... items) {
        List<A> result = nil();
        for (int i = items.length - 1; i >= 0; i--) {
            List<A> newList = new List<A>();
            newList.head = items[i];
            newList.tail = result;
            result = newList;
        }
        return result;
    }

    public Iterator<A> iterator() {
        return new Iterator<A>() {
            private List<A> current = List.this;
            public boolean hasNext() { return current != null && current.head != null; }
            public A next() {
                A result = current.head;
                current = current.tail;
                return result;
            }
            public void remove() { throw new UnsupportedOperationException(); }
        };
    }
}
