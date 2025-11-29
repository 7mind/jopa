package com.sun.mirror.apt;

public interface RoundState {
    boolean finalRound();
    boolean errorRaised();
    boolean sourceFilesCreated();
    boolean classFilesCreated();
}
