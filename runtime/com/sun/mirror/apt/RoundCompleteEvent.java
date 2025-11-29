package com.sun.mirror.apt;

public abstract class RoundCompleteEvent extends java.util.EventObject {
    protected RoundCompleteEvent(AnnotationProcessorEnvironment source) {
        super(source);
    }

    public AnnotationProcessorEnvironment getSource() {
        return (AnnotationProcessorEnvironment) super.getSource();
    }

    public abstract RoundState getRoundState();
}
