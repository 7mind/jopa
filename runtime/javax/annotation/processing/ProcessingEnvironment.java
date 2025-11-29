package javax.annotation.processing;

import java.util.Locale;
import java.util.Map;
import javax.lang.model.util.Elements;
import javax.lang.model.util.Types;

public interface ProcessingEnvironment {
    Map<String, String> getOptions();
    Messager getMessager();
    Filer getFiler();
    Elements getElementUtils();
    Types getTypeUtils();
    javax.lang.model.SourceVersion getSourceVersion();
    Locale getLocale();
}
