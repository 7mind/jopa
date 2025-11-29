package com.sun.mirror.apt;

public interface RoundCompleteListener extends AnnotationProcessorListener {
    void roundComplete(RoundCompleteEvent event);
}
