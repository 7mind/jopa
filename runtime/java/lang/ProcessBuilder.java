package java.lang;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;

public class ProcessBuilder {
    private final List<String> command;
    private File directory;
    private final Map<String, String> environment = new java.util.HashMap<String, String>();

    public ProcessBuilder(List<String> command) {
        this.command = new ArrayList<String>(command);
    }

    public ProcessBuilder(String... command) {
        this.command = new ArrayList<String>();
        for (String c : command) {
            this.command.add(c);
        }
    }

    public List<String> command() {
        return command;
    }

    public ProcessBuilder command(String... command) {
        this.command.clear();
        for (String c : command) {
            this.command.add(c);
        }
        return this;
    }

    public Map<String, String> environment() {
        return environment;
    }

    public ProcessBuilder directory(File directory) {
        this.directory = directory;
        return this;
    }

    public File directory() {
        return directory;
    }

    public Process start() throws IOException {
        return null;
    }
}
