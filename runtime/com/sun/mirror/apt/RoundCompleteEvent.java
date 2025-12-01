package com.sun.mirror.apt;
public abstract class RoundCompleteEvent extends java.util.EventObject {
    protected RoundCompleteEvent(AnnotationProcessorEnvironment source, RoundState rs) { super(source); }
    public abstract RoundState getRoundState();
    public abstract Source getSource();
    public enum Source { BORN, REBORN }
}