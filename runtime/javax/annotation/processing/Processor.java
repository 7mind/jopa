package javax.annotation.processing;

import java.util.Set;
import javax.lang.model.element.TypeElement;

public interface Processor {
    Set<String> getSupportedOptions();
    Set<String> getSupportedAnnotationTypes();
    javax.lang.model.SourceVersion getSupportedSourceVersion();
    void init(ProcessingEnvironment processingEnv);
    boolean process(Set<? extends TypeElement> annotations, RoundEnvironment roundEnv);
    Iterable<? extends Completion> getCompletions(javax.lang.model.element.Element element,
        javax.lang.model.element.AnnotationMirror annotation,
        javax.lang.model.element.ExecutableElement member, String userText);
}
