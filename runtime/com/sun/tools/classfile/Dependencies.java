package com.sun.tools.classfile;

import java.io.IOException;
import java.util.Set;
import java.util.regex.Pattern;

public class Dependencies {
    public interface ClassFileReader {
        ClassFile getClassFile(String className) throws IOException, ConstantPoolException;
    }

    public interface Recorder {
        void addDependency(Dependency d);
    }

    public interface Filter {
        boolean accepts(Dependency d);
    }

    private Filter filter;

    public void setFilter(Filter filter) {
        this.filter = filter;
    }

    public static Filter getRegexFilter(final Pattern pattern) {
        return new Filter() {
            public boolean accepts(Dependency d) {
                return d != null && pattern.matcher(d.toString()).matches();
            }
        };
    }

    public static Filter getPackageFilter(final Set<String> packages, final boolean allowSubpackages) {
        return new Filter() {
            public boolean accepts(Dependency d) {
                if (d == null) {
                    return false;
                }
                String target = d.getTarget().toString();
                for (String pkg : packages) {
                    if (target.startsWith(pkg)) {
                        return allowSubpackages || target.equals(pkg);
                    }
                }
                return false;
            }
        };
    }

    public void findAllDependencies(ClassFileReader reader, Set<String> roots, boolean transitive, Recorder recorder) {
        // Stub: no dependency analysis performed
    }
}
