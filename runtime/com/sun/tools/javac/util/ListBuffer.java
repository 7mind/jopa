package com.sun.tools.javac.util;

public class ListBuffer<A> {
    private java.util.ArrayList<A> items = new java.util.ArrayList<A>();

    public static <A> ListBuffer<A> lb() { return new ListBuffer<A>(); }

    public ListBuffer<A> append(A item) {
        items.add(item);
        return this;
    }

    @SuppressWarnings("unchecked")
    public List<A> toList() {
        return List.from((A[]) items.toArray());
    }
}
