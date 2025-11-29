package javax.annotation.processing;

import java.util.Set;
import javax.lang.model.element.TypeElement;
import javax.lang.model.SourceVersion;

public abstract class AbstractProcessor implements Processor {
    protected ProcessingEnvironment processingEnv;

    protected AbstractProcessor() {}

    public Set<String> getSupportedOptions() { return java.util.Collections.emptySet(); }
    public Set<String> getSupportedAnnotationTypes() { return java.util.Collections.emptySet(); }
    public SourceVersion getSupportedSourceVersion() { return SourceVersion.RELEASE_6; }
    public void init(ProcessingEnvironment processingEnv) { this.processingEnv = processingEnv; }
    public abstract boolean process(Set<? extends TypeElement> annotations, RoundEnvironment roundEnv);
    public Iterable<? extends Completion> getCompletions(javax.lang.model.element.Element element, javax.lang.model.element.AnnotationMirror annotation, javax.lang.model.element.ExecutableElement member, String userText) { return java.util.Collections.emptyList(); }
}
