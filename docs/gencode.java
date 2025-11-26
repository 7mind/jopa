import java.io.*;
import java.util.*;

/**
 * This helper class generates code.h and code.cpp, the lookup table used
 * by Jikes in deciding how to categorize Unicode input characters.  This
 * is written in Java in order to accurately track the Unicode rules as
 * implemented by the latest version of java.lang.Character.
 */
class gencode
{
    static final int SPACE_CODE    = 0; // Character.isWhitespace
    static final int BAD_CODE      = 1; // everything else ...
    static final int DIGIT_CODE    = 2; // Character.isDigit
    static final int ID_PART_CODE  = 3; // Character.isJavaIdentifierPart
    static final int LOWER_CODE    = 4; // Character.isLowerCase
    static final int UPPER_CODE    = 5; // Character.isUpperCase
    static final int ID_START_CODE = 6; // Character.isJavaIdentifierStart
    static final int MAX_CODEPOINT = 0x10ffff; // Maximum Unicode.

    static final String[] CODE_NAMES = {
        "SPACE_CODE", "BAD_CODE", "DIGIT_CODE", "ID_PART_CODE", "LOWER_CODE",
        "UPPER_CODE", "ID_START_CODE"
    };

    static public void main(String args[])
        throws FileNotFoundException, IOException
    {
        System.out.println("Gathering data...");
        // Use char[] for ease in building strings, despite only using 8 bits.
        int numElements = MAX_CODEPOINT + 1;
        char[] info = new char[numElements];
        for (int i = 0; i < info.length; i++)
        {
            if (Character.isWhitespace(i))
                info[i] = SPACE_CODE;
            else if (Character.isLowerCase(i))
            {
                assert Character.isJavaIdentifierStart(i);
                info[i] = LOWER_CODE;
            }
            else if (Character.isUpperCase(i))
            {
                assert Character.isJavaIdentifierStart(i);
                info[i] = UPPER_CODE;
            }
            else if (Character.isDigit(i))
            {
                assert Character.isJavaIdentifierPart(i);
                info[i] = DIGIT_CODE;
            }
            else if (Character.isJavaIdentifierStart(i))
                info[i] = ID_START_CODE;
            else if (Character.isJavaIdentifierPart(i))
                info[i] = ID_PART_CODE;
            else
            {
                info[i] = BAD_CODE;
                numElements--;
            }
        }

        System.out.println("Compressing tables...");
        int bestShift = 0;
        int bestEst = info.length;
        String bestBlkStr = null;

        for (int i = 3; i < 12; i++)
        {
            int blkSize = 1 << i;
            Map blocks = new HashMap();
            List blkArray = new ArrayList();
            System.out.print("shift: " + i);

            for (int j = 0; j < info.length; j += blkSize)
            {
                String key = new String(info, j, blkSize);
                if (blocks.get(key) == null)
                {
                    blkArray.add(key);
                    blocks.put(key, new Integer(blkArray.size()));
                }
            }
            int blkNum = blkArray.size();
            int blockLen = blkNum * blkSize;
            System.out.print(" before " + blockLen);

            //
            // Try to pack blkArray, by finding successively smaller matches
            // between heads and tails of blocks.
            //
            for (int j = blkSize - 1; j > 0; j--)
            {
                Map tails = new HashMap();
                for (int k = 0; k < blkArray.size(); k++)
                {
                    String str = (String) blkArray.get(k);
                    if (str == null)
                        continue;
                    String tail = str.substring(str.length() - j);
                    List l = (List) tails.get(tail);
                    if (l == null)
                        tails.put(tail,
                                  new LinkedList(Collections
                                                 .singleton(new Integer(k))));
                    else l.add(new Integer(k));
                }

                //
                // Now calculate the heads, and merge overlapping blocks
                //
            block:
                for (int k = 0; k < blkArray.size(); k++)
                {
                    String tomerge = (String) blkArray.get(k);
                    if (tomerge == null)
                        continue;
                    while (true)
                    {
                        String head = tomerge.substring(0, j);
                        LinkedList entry = (LinkedList) tails.get(head);
                        if (entry == null)
                            continue block;
                        Integer other = (Integer) entry.removeFirst();
                        if (other.intValue() == k)
                        {
                            if (entry.size() > 0)
                            {
                                entry.add(other);
                                other = (Integer) entry.removeFirst();
                            }
                            else
                            {
                                entry.add(other);
                                continue block;
                            }
                        }
                        if (entry.size() == 0)
                            tails.remove(head);

                        //
                        // A match was found.
                        //
                        String merge = blkArray.get(other.intValue()) +
                            tomerge.substring(j);
                        blockLen -= j;
                        blkNum--;
                        if (other.intValue() < k)
                        {
                            blkArray.set(k, null);
                            blkArray.set(other.intValue(), merge);
                            String tail = merge.substring(merge.length() - j);
                            List l = (List) tails.get(tail);
                            Collections.replaceAll(l, new Integer(k), other);
                            continue block;
                        }
                        blkArray.set(k, merge);
                        blkArray.set(other.intValue(), null);
                        tomerge = merge;
                    }
                }
            }
            StringBuffer blockStr = new StringBuffer(blockLen);
            for (int k = 0; k < blkArray.size(); k++)
            {
                String str = (String) blkArray.get(k);
                if (str != null)
                    blockStr.append(str);
            }
            assert blockStr.length() == blockLen :
                "Unexpected blockLen " + blockLen;
            int estimate = blockLen + (info.length >> (i - 1));
            System.out.println(" after merge " + blockLen + ": " + estimate +
                               " bytes");
            if (estimate < bestEst)
            {
                bestEst = estimate;
                bestShift = i;
                bestBlkStr = blockStr.toString();
            }
        }

        System.out.println("Generating code.h with shift of " + bestShift);
        int blkSize = 1 << bestShift;
        char[] blocks = new char[info.length >> bestShift];
        for (int j = 0; j < info.length; j += blkSize)
        {
            String key = new String(info, j, blkSize);
            int index = bestBlkStr.indexOf(key);
            assert index != -1 : "Unexpected index for " + j;
            blocks[j >> bestShift] = (char) (index - j);
        }

        //
        // Process the code.h file
        //
        PrintStream hfile = new PrintStream(new FileOutputStream("code.h"));
        printHeader(hfile, new String[] {"\"platform.h\""});
        hfile.println("#ifndef code_INCLUDED");
        hfile.println("#define code_INCLUDED");
        hfile.println();
        hfile.println("class Code");
        hfile.println("{");
        hfile.println("    //");
        hfile.println("    // To facilitate the scanning, the character set is partitioned into");
        hfile.println("    // categories using the array CODE. These are described below together");
        hfile.println("    // with some self-explanatory functions defined on CODE.");
        hfile.println("    //");
        hfile.println("    enum {");
        hfile.println("        SHIFT = " + bestShift + ",");
        hfile.println("        SPACE_CODE = " + SPACE_CODE + ',');
        hfile.println("        BAD_CODE = " + BAD_CODE + ',');
        hfile.println("        DIGIT_CODE = " + DIGIT_CODE + ',');
        hfile.println("        ID_PART_CODE = " + ID_PART_CODE + ',');
        hfile.println("        LOWER_CODE = " + LOWER_CODE + ',');
        hfile.println("        UPPER_CODE = " + UPPER_CODE + ',');
        hfile.println("        ID_START_CODE = " + ID_START_CODE);
        hfile.println("    };");
        hfile.println();
        hfile.println("    static char codes[" + bestBlkStr.length() + "];");
        hfile.println("    static u2 blocks[" +  blocks.length + "];");
        hfile.println();
        hfile.println();
        hfile.println("public:");
        hfile.println("#ifdef JIKES_DEBUG");
        hfile.println("    static inline void CodeCheck(u4 c)");
        hfile.println("    {");
        hfile.println("        assert((u2) (blocks[c >> SHIFT] + c) < " +
                      bestBlkStr.length() + ");");
        hfile.println("    }");
        hfile.println();
        hfile.println("    static inline bool CodeCheck(void)");
        hfile.println("    {");
        hfile.println("        for (u4 c = 0; c <= " + MAX_CODEPOINT +
                      "; c++)");
        hfile.println("            CodeCheck(c);");
        hfile.println("        return true;");
        hfile.println("    }");
        hfile.println("#endif // JIKES_DEBUG");
        hfile.println();
        hfile.println("//");
        hfile.println("// These methods test for Unicode surrogate pairs.");
        hfile.println("//");
        hfile.println("    static inline bool IsHighSurrogate(wchar_t c)");
        hfile.println("    {");
        hfile.println("        return c >= 0xd800 && c <= 0xdbff;");
        hfile.println("    }");
        hfile.println("    static inline bool IsLowSurrogate(wchar_t c)");
        hfile.println("    {");
        hfile.println("        return c >= 0xdc00 && c <= 0xdfff;");
        hfile.println("    }");
        hfile.println();
        hfile.println("    static inline u4 Codepoint(wchar_t hi, wchar_t lo)");
        hfile.println("    {");
        hfile.println("        assert(IsHighSurrogate(hi) && IsLowSurrogate(lo));");
        hfile.println("        return (hi << 10) + lo + (0x10000 - (0xd800 << 10) - 0xdc00);");
        hfile.println("    }");
        hfile.println("    static inline u4 Codepoint(const wchar_t* p)");
        hfile.println("    {");
        hfile.println("        u4 result = (u4) *p;");
        hfile.println("        if (IsHighSurrogate(result) && IsLowSurrogate(p[1]))");
        hfile.println("            result = Codepoint(result, p[1]);");
        hfile.println("        return result;");
        hfile.println("    }");
        hfile.println("    static inline int Codelength(const wchar_t* p)");
        hfile.println("    {");
        hfile.println("        return (IsHighSurrogate(*p) && IsLowSurrogate(p[1])) ? 2 : 1;");
        hfile.println("    }");
        hfile.println();
        hfile.println("//");
        hfile.println("// These methods test for ASCII characteristics. Since it is strictly ASCII,");
        hfile.println("// there is no need to check for Unicode surrogate pairs.");
        hfile.println("//");
        hfile.println("    static inline bool IsNewline(wchar_t c)");
        hfile.println("    {");
        hfile.println("        return c == U_LF || c == U_CR;");
        hfile.println("    }");
        hfile.println("    static inline bool IsSpaceButNotNewline(wchar_t c)");
        hfile.println("    {");
        hfile.println("        return c == U_SP || c == U_FF || c == U_HT;");
        hfile.println("    }");
        hfile.println("    static inline bool IsSpace(wchar_t c)");
        hfile.println("    {");
        hfile.println("        return c == U_SP || c == U_CR || c == U_LF ||");
        hfile.println("            c == U_HT || c == U_FF;");
        hfile.println("    }");
        hfile.println();
        hfile.println("    static inline bool IsDecimalDigit(wchar_t c)");
        hfile.println("    {");
        hfile.println("        return c <= U_9 && c >= U_0;");
        hfile.println("    }");
        hfile.println("    static inline bool IsOctalDigit(wchar_t c)");
        hfile.println("    {");
        hfile.println("        return c <= U_7 && c >= U_0;");
        hfile.println("    }");
        hfile.println("    static inline bool IsHexDigit(wchar_t c)");
        hfile.println("    {");
        hfile.println("        return c <= U_f && (c >= U_a ||");
        hfile.println("                            (c >= U_A && c <= U_F) ||");
        hfile.println("                            (c >= U_0 && c <= U_9));");
        hfile.println("    }");
        hfile.println("    static inline int Value(wchar_t c)");
        hfile.println("    {");
        hfile.println("        assert(IsHexDigit(c));");
        hfile.println("        return c - (c <= U_9 ? U_0 : c < U_a ? U_A - 10 : U_a - 10);");
        hfile.println("    }");
        hfile.println("    static inline bool IsSign(wchar_t c)");
        hfile.println("    {");
        hfile.println("        return c == U_MINUS || c == U_PLUS;");
        hfile.println("    }");
        hfile.println();
        hfile.println("    static inline bool IsAsciiUpper(wchar_t c)");
        hfile.println("    {");
        hfile.println("        return c <= U_Z && c >= U_A;");
        hfile.println("    }");
        hfile.println("    static inline bool IsAsciiLower(wchar_t c)");
        hfile.println("    {");
        hfile.println("        return c <= U_z && c >= U_a;");
        hfile.println("    }");
        hfile.println();
        hfile.println("//");
        hfile.println("// The following methods recognize Unicode surrogate pairs, hence the need to");
        hfile.println("// pass a pointer. Use Codelength() to determine if one or two characters");
        hfile.println("// were used in the formation of a character.");
        hfile.println("//");
        printMethod(hfile, "IsWhitespace", "SPACE_CODE", "==");
        printMethod(hfile, "IsDigit", "DIGIT_CODE", "==");
        printMethod(hfile, "IsUpper", "UPPER_CODE", "==");
        printMethod(hfile, "IsLower", "LOWER_CODE", "==");
        printMethod(hfile, "IsAlpha", "LOWER_CODE", ">=");
        printMethod(hfile, "IsAlnum", "DIGIT_CODE", ">=");
        hfile.println("};");
        hfile.println();
        hfile.println("#endif // code_INCLUDED");
        printFooter(hfile);
        hfile.close();

        //
        // Process the code.cpp file
        //
        System.out.println("Generating code.cpp");
        PrintStream cfile = new PrintStream(new FileOutputStream("code.cpp"));
        printHeader(cfile, new String[] {"\"code.h\""});
        cfile.println("char Code::codes[" + bestBlkStr.length() + "] =");
        cfile.println("{");
        for (int j = 0; j < bestBlkStr.length(); j += 5)
        {
            for (int k = 0; k < 5; k++)
            {
                if (k + j >= bestBlkStr.length())
                    break;
                if (k == 0)
                    cfile.print("   ");
                cfile.print(" " + CODE_NAMES[bestBlkStr.charAt(k + j)] + ",");
            }
            cfile.println();
        }
        cfile.println("};");
        cfile.println();
        cfile.println();
        cfile.println("//");
        cfile.println("// The Blocks vector:");
        cfile.println("//");
        cfile.println("u2 Code::blocks[" + blocks.length + "] =");
        cfile.println("{");
        for (int k = 0; k < blocks.length; k += 9)
        {
            for (int i = 0; i < 9; i++)
            {
                if (k + i >= blocks.length)
                    break;
                if (i == 0)
                    cfile.print("   ");
                cfile.print(" 0x" + Integer.toHexString(blocks[k + i]) + ",");
            }
            cfile.println();
        }
        cfile.println("};");
        printFooter(cfile);
        cfile.close();

        //
        // Print statistics.
        //
        System.out.println("Total static storage utilization is " +
                           blocks.length * 2 + " bytes for block lookup");
        System.out.println("   plus " + bestBlkStr.length() +
                           " bytes for the encodings");
        System.out.println("Number of legal unicode codepoints:" +
                           numElements);
    }

    static void printHeader(PrintStream file, String[] includes)
    {
        file.println("// $I" + /* CVS hack */ "d$ -*- c++ -*-");
        file.println("// DO NOT MODIFY THIS FILE - it is generated using gencode.java.");
        file.println("//");
        file.println("// This software is subject to the terms of the IBM Jikes Compiler");
        file.println("// License Agreement available at the following URL:");
        file.println("// http://www.ibm.com/research/jikes.");
        file.println("// Copyright (C) 1999, 2004 IBM Corporation and others.  All Rights Reserved.");
        file.println("// You must accept the terms of that agreement to use this software.");
        file.println("//");
        file.println();

        for (int i = 0; i < includes.length; i++)
        {
            file.println("#include " + includes[i]);
        }

        file.println();
        file.println("#ifdef HAVE_JIKES_NAMESPACE");
        file.println("namespace Jikes { // Open namespace Jikes block");
        file.println("#endif");
        file.println();
    }

    static void printMethod(PrintStream file, String name, String value,
                            String relation)
    {
        file.println("    static inline bool " + name + "(const wchar_t* p)");
        file.println("    {");
        file.println("        u4 c = Codepoint(p);");
        file.println("        return codes[(u2) (blocks[c >> SHIFT] + c)] " +
                     relation + ' ' + value + ";");
        file.println("    }");
    }

    static void printFooter(PrintStream file)
    {
        file.println();
        file.println("#ifdef HAVE_JIKES_NAMESPACE");
        file.println("} // Close namespace Jikes block");
        file.println("#endif");
        file.println();
    }
}
