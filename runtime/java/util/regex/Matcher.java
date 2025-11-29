package java.util.regex;

public final class Matcher implements MatchResult {
    private Matcher() {}

    public Pattern pattern() { return null; }
    public MatchResult toMatchResult() { return null; }
    public Matcher usePattern(Pattern newPattern) { return null; }
    public Matcher reset() { return null; }
    public Matcher reset(CharSequence input) { return null; }
    public int start() { return 0; }
    public int start(int group) { return 0; }
    public int end() { return 0; }
    public int end(int group) { return 0; }
    public String group() { return null; }
    public String group(int group) { return null; }
    public int groupCount() { return 0; }
    public boolean matches() { return false; }
    public boolean find() { return false; }
    public boolean find(int start) { return false; }
    public boolean lookingAt() { return false; }
    public static String quoteReplacement(String s) { return null; }
    public Matcher appendReplacement(StringBuffer sb, String replacement) { return null; }
    public StringBuffer appendTail(StringBuffer sb) { return null; }
    public String replaceAll(String replacement) { return null; }
    public String replaceFirst(String replacement) { return null; }
    public Matcher region(int start, int end) { return null; }
    public int regionStart() { return 0; }
    public int regionEnd() { return 0; }
    public boolean hasTransparentBounds() { return false; }
    public Matcher useTransparentBounds(boolean b) { return null; }
    public boolean hasAnchoringBounds() { return false; }
    public Matcher useAnchoringBounds(boolean b) { return null; }
    public boolean hitEnd() { return false; }
    public boolean requireEnd() { return false; }
}
