// Test parameterized types with type arguments

// Simple class to use as type argument (avoid java.lang dependency issues)
class MyData {
    int value;
}

class Node<T> {
    T data;

    Node(T data) {
        this.data = data;
    }

    T getData() {
        return data;
    }
}

class UseParameterized {
    // Field with parameterized type
    Node<MyData> dataNode;

    void test() {
        // Local variables with parameterized types
        MyData data = new MyData();
        Node<MyData> local1 = new Node<MyData>(data);
    }
}
