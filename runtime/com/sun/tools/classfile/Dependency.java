package com.sun.tools.classfile;

public class Dependency {
    private final Location origin;
    private final Location target;

    public Dependency(Location origin, Location target) {
        this.origin = origin;
        this.target = target;
    }

    public Location getOrigin() {
        return origin;
    }

    public Location getTarget() {
        return target;
    }

    public String toString() {
        return origin + " -> " + target;
    }

    public static class Location {
        private final String name;

        public Location(String name) {
            this.name = name;
        }

        public String toString() {
            return name;
        }
    }
}
