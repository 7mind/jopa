package com.sun.tools.classfile;

public class ConstantPool {
    public Object get(int index) {
        return null;
    }

    public String getUTF8Value(int index) {
        return null;
    }

    public static class CPRefInfo {
        public String getClassName() throws ConstantPoolException {
            return "";
        }

        public NameAndType_info getNameAndTypeInfo() throws ConstantPoolException {
            return null;
        }
    }

    public static class NameAndType_info {
        public String getName() throws ConstantPoolException {
            return "";
        }

        public String getType() throws ConstantPoolException {
            return "";
        }
    }

    public static class CONSTANT_Methodref_info extends CPRefInfo {
    }
}
