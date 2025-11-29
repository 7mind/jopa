#include "op.h"
#include "class.h"


namespace Jopa { // Open namespace Jopa block
#ifdef JOPA_DEBUG
int Operators::OpDesc(Opcode opc, const char** name, const char** desc)
{
    struct OpEntry
    {
        const char* op_name;
        const char* op_desc;
        char arg_bytes;
    };

    static struct OpEntry table[] =
    {
        {"nop", "do nothing", 0},
        {"aconst_null", "push null", 0},
        {"iconst_m1", "push int constant -1", 0},
        {"iconst_0", "push int constant 0", 0},
        {"iconst_1", "push int constant 1", 0},
        {"iconst_2", "push int constant 2", 0},
        {"iconst_3", "push int constant 3", 0},
        {"iconst_4", "push int constant 4", 0},
        {"iconst_5", "push int constant 5", 0},
        {"lconst_0", "push long constant 0", 0},
        {"lconst_1", "push long constant 1", 0},
        {"fconst_0", "push float 0.0", 0},
        {"fconst_1", "push float 1.0", 0},
        {"fconst_2", "push float 2.0", 0},
        {"dconst_0", "push double 0.0", 0},
        {"dconst_1", "push double 1.0", 0},
        {"bipush", "push byte", 1},
        {"sipush", "push short", 2},
        {"ldc", "push item from constant pool", 1},
        {"ldc_w", "push item from constant pool (wide index)", 2},
        {"ldc2_w", "push long or double from constant pool (wide index)", 2},
        {"iload", "load int from local variable", 1},
        {"lload", "load long from local variable", 1},
        {"fload", "load float from local variable", 1},
        {"dload", "load double from local variable", 1},
        {"aload", "load reference from local variable", 1},
        {"iload_0", "load int from local variable 0", 0},
        {"iload_1", "load int from local variable 1", 0},
        {"iload_2", "load int from local variable 2", 0},
        {"iload_3", "load int from local variable 3", 0},
        {"lload_0", "load long from local variable 0", 0},
        {"lload_1", "load long from local variable 1", 0},
        {"lload_2", "load long from local variable 2", 0},
        {"lload_3", "load long from local variable 3", 0},
        {"fload_0", "load float from local variable 0", 0},
        {"fload_1", "load float from local variable 1", 0},
        {"fload_2", "load float from local variable 2", 0},
        {"fload_3", "load float from local variable 3", 0},
        {"dload_0", "load double from local variable 0", 0},
        {"dload_1", "load double from local variable 1", 0},
        {"dload_2", "load double from local variable 2", 0},
        {"dload_3", "load double from local variable 3", 0},
        {"aload_0", "load reference from local variable 0", 0},
        {"aload_1", "load reference from local variable 1", 0},
        {"aload_2", "load reference from local variable 2", 0},
        {"aload_3", "load reference from local variable 3", 0},
        {"iaload", "load int from array", 0},
        {"laload", "load long from array", 0},
        {"faload", "load float from array", 0},
        {"daload", "load double from array", 0},
        {"aaload", "load reference from array", 0},
        {"baload", "load byte or boolean from array", 0},
        {"caload", "load char from array", 0},
        {"saload", "load short from array", 0},
        {"istore", "store int into local variable", 1},
        {"lstore", "store long into local variable", 1},
        {"fstore", "store float into local variable", 1},
        {"dstore", "store double into local variable", 1},
        {"astore", "store reference into local variable", 1},
        {"istore_0", "store int into local variable 0", 0},
        {"istore_1", "store int into local variable 1", 0},
        {"istore_2", "store int into local variable 2", 0},
        {"istore_3", "store int into local variable 3", 0},
        {"lstore_0", "store long into local variable 0", 0},
        {"lstore_1", "store long into local variable 1", 0},
        {"lstore_2", "store long into local variable 2", 0},
        {"lstore_3", "store long into local variable 3", 0},
        {"fstore_0", "store float into local variable 0", 0},
        {"fstore_1", "store float into local variable 1", 0},
        {"fstore_2", "store float into local variable 2", 0},
        {"fstore_3", "store float into local variable 3", 0},
        {"dstore_0", "store double into local variable 0", 0},
        {"dstore_1", "store double into local variable 1", 0},
        {"dstore_2", "store double into local variable 2", 0},
        {"dstore_3", "store double into local variable 3", 0},
        {"astore_0", "store reference into local variable 0", 0},
        {"astore_1", "store reference into local variable 1", 0},
        {"astore_2", "store reference into local variable 2", 0},
        {"astore_3", "store reference into local variable 3", 0},
        {"iastore", "store into int array", 0},
        {"lastore", "store into long array", 0},
        {"fastore", "store into float array", 0},
        {"dastore", "store into double array", 0},
        {"aastore", "store into reference array", 0},
        {"bastore", "store into byte or boolean array", 0},
        {"castore", "store into char array", 0},
        {"sastore", "store into short array", 0},
        {"pop", "pop top operand stack byte", 0},
        {"pop2", "pop top two operand stack bytes", 0},
        {"dup", "duplicate top operand stack byte", 0},
        {"dup_x1", "duplicate top operand stack byte under 1 byte", 0},
        {"dup_x2", "duplicate top operand stack byte under 2 bytes", 0},
        {"dup2", "duplicate top two operand stack bytes", 0},
        {"dup2_x1", "duplicate top two operand stack bytes under 1 byte", 0},
        {"dup2_x2", "duplicate top two operand stack bytes under 2 bytes", 0},
        {"swap", "swap top two operand stack bytes", 0},
        {"iadd", "add int", 0},
        {"ladd", "add long", 0},
        {"fadd", "add float", 0},
        {"dadd", "add double", 0},
        {"isub", "subtract int", 0},
        {"lsub", "subtract long", 0},
        {"fsub", "substract float", 0},
        {"dsub", "subtract double", 0},
        {"imul", "multiply int", 0},
        {"lmul", "multiply long", 0},
        {"fmul", "multiply float", 0},
        {"dmul", "multiply double", 0},
        {"idiv", "divide int", 0},
        {"ldiv", "divide long", 0},
        {"fdiv", "divide float", 0},
        {"ddiv", "divide double", 0},
        {"irem", "remainder int", 0},
        {"lrem", "remainder long", 0},
        {"frem", "remainder float", 0},
        {"drem", "remainder double", 0},
        {"ineg", "negate int", 0},
        {"lneg", "negate long", 0},
        {"fneg", "negate float", 0},
        {"dneg", "negate double", 0},
        {"ishl", "shift left int", 0},
        {"lshl", "shift left long", 0},
        {"ishr", "shift right int", 0},
        {"lshr", "shift right long", 0},
        {"iushr", "logical shift right int", 0},
        {"lushr", "logical shift right long", 0},
        {"iand", "boolean and int", 0},
        {"land", "boolean and long", 0},
        {"ior", "boolean or int", 0},
        {"lor", "boolean or long", 0},
        {"ixor", "boolean xor int", 0},
        {"lxor", "boolean xor long", 0},
        {"iinc", "increment local variable by constant", 2},
        {"i2l", "convert int to long", 0},
        {"i2f", "convert int to float", 0},
        {"i2d", "convert int to double", 0},
        {"l2i", "convert long to int", 0},
        {"l2f", "convert long to float", 0},
        {"l2d", "convert long to double", 0},
        {"f2i", "convert float to int", 0},
        {"f2l", "convert float to long", 0},
        {"f2d", "convert float to double", 0},
        {"d2i", "convert double to int", 0},
        {"d2l", "convert double to long", 0},
        {"d2f", "convert double to float", 0},
        {"i2b", "convert int to byte", 0},
        {"i2c", "convert int to char", 0},
        {"i2s", "convert int to short", 0},
        {"lcmp", "compare long", 0},
        {"fcmpl", "compare float less", 0},
        {"fcmpg", "compare float greater", 0},
        {"dcmpl", "compare double less", 0},
        {"dcmpg", "compare double greater", 0},
        {"ifeq", "branch if int eq zero", 2},
        {"ifne", "branch if int ne zero", 2},
        {"iflt", "branch if int lt zero", 2},
        {"ifge", "branch if int ge zero", 2},
        {"ifgt", "branch if int gt zero", 2},
        {"ifle", "branch if int le zero", 2},
        {"if_icmpeq", "branch if int comparison eq", 2},
        {"if_icmpne", "branch if int comparison ne", 2},
        {"if_icmplt", "branch if int comparison lt", 2},
        {"if_icmpge", "branch if int comparison ge", 2},
        {"if_icmpgt", "branch if int comparison gt", 2},
        {"if_icmple", "branch if int comparison le", 2},
        {"if_acmpeq", "branch if reference comparison eq", 2},
        {"if_acmpne", "branch if reference comparison ne", 2},
        {"goto", "branch always", 2},
        {"jsr", "jump subroutine, deprecated", 2},
        {"ret", "return from subroutine, deprecated", 2},
        {"tableswitch", "access jump table by index and jump", 12},
        {"lookupswitch", "access jump table by key match and jump", 8},
        {"ireturn", "return int from method", 0},
        {"lreturn", "return long from method", 0},
        {"freturn", "return float from method", 0},
        {"dreturn", "return double from method", 0},
        {"areturn", "return reference from method", 0},
        {"return", "return void from method", 0},
        {"getstatic", "get static field from class", 2},
        {"putstatic", "set static field in class", 2},
        {"getfield", "fetch field from object", 2},
        {"putfield", "set field in object", 2},
        {"invokevirtual", "invoke polymorphic instance method; dispatch based on runtime type", 2},
        {"invokespecial", "invoke constructor, private, or superclass instance method; dispatch based on compile-time type", 2},
        {"invokestatic", "invoke a class (static) method", 2},
        {"invokeinterface", "invoke interface method", 4},
        {"xxxunusedxxx", "xxxunusedxxx", 0},
        {"new", "create new object", 2},
        {"newarray", "create new array", 1},
        {"anewarray", "create new array of references", 2},
        {"arraylength", "get length of array", 0},
        {"athrow", "throw exception or error", 0},
        {"checkcast", "check whether object is of given type", 2},
        {"instanceof", "determine if object is of given type", 2},
        {"monitorenter", "enter monitor for object", 0},
        {"monitorexit", "exit monitor for object", 0},
        {"wide", "", 0}, // special cased elsewhere
        {"multianewarray", "create new multidimensional array", 3},
        {"ifnull", "branch if reference is null", 2},
        {"ifnonnull", "branch if reference not null", 2},
        {"goto_w", "branch always (wide index)", 4},
        {"jsr_w", "jump subroutine (wide index), deprecated", 4}
        // no need to list SOFTWARE and HARDWARE here
    };

    if (opc == OP_SOFTWARE)
    {
        if (name)
            *name = "software";
        if (desc)
            *desc = "reserved for software use";
    }
    else if (opc == OP_HARDWARE)
    {
        if (name)
            *name = "hardware";
        if (desc)
            *desc = "reserved for hardware use";
    }
    else if (opc <= 0xc9)
    {
        if (name)
            *name = table[opc].op_name;
        if (desc)
            *desc = table[opc].op_desc;
        return table[opc].arg_bytes;
    }
    else
    {
        if (name)
            *name = "illegal";
        if (desc)
            *desc = "illegal";
    }
    return 0;
}

void Operators::OpLine(const ConstantPool& constant_pool, char hdr, int pc,
                       const char* name, char* args, const char* desc,
                       OpInfo info_kind, unsigned info_index)
{
    // Generate line of opcode dump, info is extra info.
    Coutput << hdr;
    Coutput.width(5);
    int len = strlen(name);
    Coutput << pc << '\t';
    if (info_kind == INFO_WIDE)
    {
        len += 5;
        Coutput << "wide ";
    }
    Coutput << name;
    if (strlen(args))
    {
        Coutput << ' ' << args;
        len += (strlen(args) + 1);
    }
    if (len < 8)
        Coutput << "\t\t\t";
    else if (len < 16)
        Coutput << "\t\t";
    else Coutput << "\t";

    switch (info_kind)
    {
    case INFO_CONST:
        if (info_index <= constant_pool.Length())
        {
            Coutput << '(';
            constant_pool[info_index] -> Describe(constant_pool);
            Coutput << ')';
        }
        else Coutput << "(illegal)";
        break;
    case INFO_LOCAL:
    case INFO_WIDE:
        Coutput << "local#" << info_index;
        break;
    default:
        ; // Do nothing.
    }
    //
    // Skip desc for now: it's too long and should only
    // be written at user request.
    //  Coutput << ' ' << desc;
    //
    assert(desc);
    Coutput << endl;
}

void Operators::OpDmp(const ConstantPool& constant_pool, const Tuple<u1>& code)
{
    assert(sizeof(int) == 4 && "Debugger must have 32-bit int");
    unsigned pc = 0;
    while (pc < code.Length())
    {
        OpInfo info_kind = INFO_NONE; // assume no extra info
        unsigned info_index = 0;
        unsigned pc_start = pc;
        Opcode opc = (Opcode) GetAndSkipU1(code, pc);

        // Set name (mnemonic) and description of opcode. Also, ensure that
        // enough bytes remain to complete the instruction.
        const char* name;
        const char* desc;
        int need = OpDesc(opc, &name, &desc);
        if (pc_start + need >= code.Length())
        {
            Coutput.width(5);
            Coutput << pc_start << "  <incomplete: " << name << '>' << endl;
            break;
        }

        char argdesc[100];
        argdesc[0] = U_NULL; // assume no argument description needed

        u1 au1;
        i1 ai1;
        i2 ai2;
        i4 ai4;
        switch (opc)
        {
        case OP_BIPUSH:
            ai1 = GetAndSkipI1(code, pc);
            sprintf(argdesc, "%d (%#02x)", ai1, ai1);
            break;
        case OP_SIPUSH:
            ai2 = GetAndSkipI2(code, pc);
            sprintf(argdesc, "%d (%#04x)", ai2, ai2);
            break;
        case OP_ILOAD: case OP_LLOAD: case OP_FLOAD: case OP_DLOAD:
        case OP_ALOAD:
        case OP_ISTORE: case OP_LSTORE: case OP_FSTORE: case OP_DSTORE:
        case OP_ASTORE:
        case OP_RET:
            info_kind = INFO_LOCAL;
            info_index = GetAndSkipU1(code, pc);
            sprintf(argdesc, "%u", info_index);
            break;
        case OP_ILOAD_0: case OP_LLOAD_0: case OP_FLOAD_0: case OP_DLOAD_0:
        case OP_ALOAD_0:
        case OP_ISTORE_0: case OP_LSTORE_0: case OP_FSTORE_0: case OP_DSTORE_0:
        case OP_ASTORE_0:
            info_kind = INFO_LOCAL;
            info_index = 0;
            break;
        case OP_ILOAD_1: case OP_LLOAD_1: case OP_FLOAD_1: case OP_DLOAD_1:
        case OP_ALOAD_1:
        case OP_ISTORE_1: case OP_LSTORE_1: case OP_FSTORE_1: case OP_DSTORE_1:
        case OP_ASTORE_1:
            info_kind = INFO_LOCAL;
            info_index = 1;
            break;
        case OP_ILOAD_2: case OP_LLOAD_2: case OP_FLOAD_2: case OP_DLOAD_2:
        case OP_ALOAD_2:
        case OP_ISTORE_2: case OP_LSTORE_2: case OP_FSTORE_2: case OP_DSTORE_2:
        case OP_ASTORE_2:
            info_kind = INFO_LOCAL;
            info_index = 2;
            break;
        case OP_ILOAD_3: case OP_LLOAD_3: case OP_FLOAD_3: case OP_DLOAD_3:
        case OP_ALOAD_3:
        case OP_ISTORE_3: case OP_LSTORE_3: case OP_FSTORE_3: case OP_DSTORE_3:
        case OP_ASTORE_3:
            info_kind = INFO_LOCAL;
            info_index = 3;
            break;
        case OP_IINC:
            info_index = GetAndSkipU1(code, pc);
            ai1 = GetAndSkipI1(code, pc);
            info_kind = INFO_LOCAL;
            sprintf(argdesc, "%u, %d (%#02x)", info_index, ai1, ai1);
            break;
        case OP_IFEQ: case OP_IFNE:
        case OP_IFLT: case OP_IFGE: case OP_IFGT: case OP_IFLE:
        case OP_IF_ICMPEQ: case OP_IF_ICMPNE:
        case OP_IF_ICMPLT: case OP_IF_ICMPGE:
        case OP_IF_ICMPGT: case OP_IF_ICMPLE:
        case OP_IF_ACMPEQ: case OP_IF_ACMPNE:
        case OP_IFNULL: case OP_IFNONNULL:
        case OP_GOTO:
        case OP_JSR:
            ai2 = GetAndSkipI2(code, pc);
            // compute branch target
            sprintf(argdesc, "%d (pc:%u)", ai2, ai2 + pc_start);
            break;
        case OP_TABLESWITCH:
            {
                i4 def;
                i4 low;
                i4 high;
                i4 len;
                i4 val;
                unsigned pc_this;
                // account for padding
                while (pc % 4)
                {
                    au1 = GetAndSkipU1(code, pc);
                    if (au1)
                        Coutput << "       <non-zero padding: " << au1 << '>'
                                << endl;
                }
                def = GetAndSkipI4(code, pc);
                low = GetAndSkipI4(code, pc);
                if (pc + 4 >= code.Length())
                {
                    Coutput << "       <incomplete: " << name << '>' << endl;
                    info_kind = INFO_DONE;
                    break;
                }
                high = GetAndSkipI4(code, pc);
                sprintf(argdesc,
                        "default:%d (pc:%u) low:%d (%#08x) high:%d (%#08x)",
                        (int) def, (unsigned) def + pc_start,
                        (int) low, (int) low, (int) high, (int) high);
                OpLine(constant_pool, ' ', pc_start, name,
                       argdesc, desc, info_kind, info_index);
                len =  high - low + 1;
                while (len)
                {
                    if (pc + 4 >= code.Length())
                    {
                        Coutput << "       <incomplete: " << name << '>'
                                << endl;
                        break;
                    }
                    pc_this = pc;
                    val = GetAndSkipI4(code, pc);
                    low++;
                    sprintf(argdesc, "match:%d (%#08x) offset:%d (pc:%u)",
                            (int) low, (int) low,
                            (int) val, (unsigned) val + pc_start);
                    OpLine(constant_pool, '*', pc_this, name,
                           argdesc, desc, INFO_NONE, 0);
                    len--;
                }
                info_kind = INFO_DONE;
            }
            break;
        case OP_LOOKUPSWITCH:
            {
                i4 def;
                i4 npairs;
                i4 match;
                i4 offset;
                // account for padding
                while (pc % 4)
                {
                    au1 = GetAndSkipU1(code, pc);
                    if (au1)
                        Coutput << "       <non-zero padding: " << au1 << '>'
                                << endl;
                }
                def = GetAndSkipI4(code, pc);
                if (pc + 4 >= code.Length())
                {
                    Coutput << "       <incomplete: " << name << '>' << endl;
                    info_kind = INFO_DONE;
                    break;
                }
                npairs = GetAndSkipI4(code, pc);
                sprintf(argdesc, "default:%d (pc:%u) npairs:%d",
                        (int) def, (unsigned) def + pc_start, (int) npairs);
                OpLine(constant_pool, ' ', pc_start, name,
                       argdesc, desc, info_kind, info_index);
                while (npairs)
                {
                    unsigned pcb = pc;
                    if (pc + 8 >= code.Length())
                    {
                        Coutput << "       <incomplete: " << name << '>'
                                << endl;
                        break;
                    }
                    match = GetAndSkipI4(code, pc);
                    offset = GetAndSkipI4(code, pc);
                    sprintf(argdesc, "match:%d (%#08x) offset:%d (pc:%u)",
                            (int) match, (int) match,
                            (int) offset, (unsigned) offset + pc_start);
                    OpLine(constant_pool, '*', pcb, name,
                           argdesc, desc, INFO_NONE, 0);
                    npairs--;
                }
                info_kind = INFO_DONE;
            }
            break;
        case OP_LDC:
            info_index = GetAndSkipU1(code, pc);
            sprintf(argdesc, "%u", info_index);
            info_kind = INFO_CONST;
            break;
        case OP_LDC_W: case OP_LDC2_W:
        case OP_GETSTATIC: case OP_PUTSTATIC:
        case OP_GETFIELD: case OP_PUTFIELD:
        case OP_INVOKEVIRTUAL: case OP_INVOKESPECIAL: case OP_INVOKESTATIC:
        case OP_NEW: case OP_ANEWARRAY:
        case OP_CHECKCAST: case OP_INSTANCEOF:
            info_index = GetAndSkipU2(code, pc);
            sprintf(argdesc, "%u", info_index);
            info_kind = INFO_CONST;
            break;
        case OP_INVOKEINTERFACE:
            {
                info_index = GetAndSkipU2(code, pc);
                unsigned nargs = GetAndSkipU1(code, pc);
                au1 = GetAndSkipU1(code, pc);
                if (au1)
                    Coutput << "       <non-zero padding: " << au1 << '>'
                            << endl;
                sprintf(argdesc, "%u (%u args)", info_index, nargs);
                info_kind = INFO_CONST;
            }
            break;
        case OP_NEWARRAY:
            au1 = GetAndSkipU1(code, pc);
            switch (au1)
            {
            case 4: sprintf(argdesc, "boolean[]"); break;
            case 5: sprintf(argdesc, "char[]"); break;
            case 6: sprintf(argdesc, "float[]"); break;
            case 7: sprintf(argdesc, "double[]"); break;
            case 8: sprintf(argdesc, "byte[]"); break;
            case 9: sprintf(argdesc, "short[]"); break;
            case 10: sprintf(argdesc, "int[]"); break;
            case 11: sprintf(argdesc, "long[]"); break;
            default:
                sprintf(argdesc, "<UNKNOWN:%u>", au1); break;
            }
            break;
        case OP_WIDE:
            {
                Opcode opc = (Opcode) GetAndSkipU1(code, pc);
                info_kind = INFO_WIDE;
                // Reset name (mnemonic) and description of modified opcode.
                need = OpDesc(opc, &name, &desc);
                if (pc_start + 1 + need * 2 >= code.Length())
                {
                    Coutput.width(5);
                    Coutput << pc_start << "  <incomplete: wide " << name
                            << '>' << endl;
                    info_kind = INFO_DONE;
                    break;
                }
                switch (opc)
                {
                case OP_ILOAD: case OP_FLOAD: case OP_LLOAD: case OP_DLOAD:
                case OP_ALOAD:
                case OP_ISTORE: case OP_FSTORE: case OP_LSTORE: case OP_DSTORE:
                case OP_ASTORE:
                case OP_RET:
                    info_index = GetAndSkipU2(code, pc);
                    sprintf(argdesc, "%u", info_index);
                    break;
                case OP_IINC:
                    info_index = GetAndSkipU2(code, pc);
                    ai2 = GetAndSkipI2(code, pc);
                    sprintf(argdesc, "%u, %d (%#04x)", info_index, ai2, ai2);
                    break;
                default:
                    Coutput.width(5);
                    Coutput << pc_start << "  <illegal wide instruction: "
                            << name << '>' << endl;
                    info_kind = INFO_DONE;
                }
            }
            break;
        case OP_MULTIANEWARRAY:
            info_index = GetAndSkipU2(code, pc);
            au1 = GetAndSkipU1(code, pc);
            info_kind = INFO_CONST;
            // au1 gives dimensions
            sprintf(argdesc, "%u, dims:%u", info_index, au1);
            break;
        case OP_GOTO_W: case OP_JSR_W:
            ai4 = GetAndSkipI4(code, pc);
            // ai4 gives offset (wide) of branch target
            sprintf(argdesc, "%d (pc:%u)",
                    (int) ai4, (unsigned) ai4 + pc_start);
            break;
        default:
            ; // do nothing
        }

        // output first part of description
        if (info_kind != INFO_DONE)
            OpLine(constant_pool, ' ', pc_start, name,
                   argdesc, desc, info_kind, info_index);
    }
}
#endif // JOPA_DEBUG


//
// stack_effect gives effect on stack of executing an opcode
//
int Operators::stack_effect[] =
{
    0, // OP_NOP
    1, // OP_ACONST_NULL
    1, // OP_ICONST_M1
    1, // OP_ICONST_0
    1, // OP_ICONST_1
    1, // OP_ICONST_2
    1, // OP_ICONST_3
    1, // OP_ICONST_4
    1, // OP_ICONST_5
    2, // OP_LCONST_0
    2, // OP_LCONST_1
    1, // OP_FCONST_0
    1, // OP_FCONST_1
    1, // OP_FCONST_2
    2, // OP_DCONST_0
    2, // OP_DCONST_1
    1, // OP_BIPUSH
    1, // OP_SIPUSH
    1, // OP_LDC
    1, // OP_LDC_W
    2, // OP_LDC2_W
    1, // OP_ILOAD
    2, // OP_LLOAD
    1, // OP_FLOAD
    2, // OP_DLOAD
    1, // OP_ALOAD
    1, // OP_ILOAD_0
    1, // OP_ILOAD_1
    1, // OP_ILOAD_2
    1, // OP_ILOAD_3
    2, // OP_LLOAD_0
    2, // OP_LLOAD_1
    2, // OP_LLOAD_2
    2, // OP_LLOAD_3
    1, // OP_FLOAD_0
    1, // OP_FLOAD_1
    1, // OP_FLOAD_2
    1, // OP_FLOAD_3
    2, // OP_DLOAD_0
    2, // OP_DLOAD_1
    2, // OP_DLOAD_2
    2, // OP_DLOAD_3
    1, // OP_ALOAD_0
    1, // OP_ALOAD_1
    1, // OP_ALOAD_2
    1, // OP_ALOAD_3
   -1, // OP_IALOAD
    0, // OP_LALOAD
   -1, // OP_FALOAD
    0, // OP_DALOAD
   -1, // OP_AALOAD
   -1, // OP_BALOAD
   -1, // OP_CALOAD
   -1, // OP_SALOAD
   -1, // OP_ISTORE
   -2, // OP_LSTORE
   -1, // OP_FSTORE
   -2, // OP_DSTORE
   -1, // OP_ASTORE
   -1, // OP_ISTORE_0
   -1, // OP_ISTORE_1
   -1, // OP_ISTORE_2
   -1, // OP_ISTORE_3
   -2, // OP_LSTORE_0
   -2, // OP_LSTORE_1
   -2, // OP_LSTORE_2
   -2, // OP_LSTORE_3
   -1, // OP_FSTORE_0
   -1, // OP_FSTORE_1
   -1, // OP_FSTORE_2
   -1, // OP_FSTORE_3
   -2, // OP_DSTORE_0
   -2, // OP_DSTORE_1
   -2, // OP_DSTORE_2
   -2, // OP_DSTORE_3
   -1, // OP_ASTORE_0
   -1, // OP_ASTORE_1
   -1, // OP_ASTORE_2
   -1, // OP_ASTORE_3
   -3, // OP_IASTORE
   -4, // OP_LASTORE
   -3, // OP_FASTORE
   -4, // OP_DASTORE
   -3, // OP_AASTORE
   -3, // OP_BASTORE
   -3, // OP_CASTORE
   -3, // OP_SASTORE
   -1, // OP_POP
   -2, // OP_POP2
    1, // OP_DUP
    1, // OP_DUP_X1
    1, // OP_DUP_X2
    2, // OP_DUP2
    2, // OP_DUP2_X1
    2, // OP_DUP2_X2
    0, // OP_SWAP
   -1, // OP_IADD
   -2, // OP_LADD
   -1, // OP_FADD
   -2, // OP_DADD
   -1, // OP_ISUB
   -2, // OP_LSUB
   -1, // OP_FSUB
   -2, // OP_DSUB
   -1, // OP_IMUL
   -2, // OP_LMUL
   -1, // OP_FMUL
   -2, // OP_DMUL
   -1, // OP_IDIV
   -2, // OP_LDIV
   -1, // OP_FDIV
   -2, // OP_DDIV
   -1, // OP_IREM
   -2, // OP_LREM
   -1, // OP_FREM
   -2, // OP_DREM
    0, // OP_INEG
    0, // OP_LNEG
    0, // OP_FNEG
    0, // OP_DNEG
   -1, // OP_ISHL
   -1, // OP_LSHL
   -1, // OP_ISHR
   -1, // OP_LSHR
   -1, // OP_IUSHR
   -1, // OP_LUSHR
   -1, // OP_IAND
   -2, // OP_LAND
   -1, // OP_IOR
   -2, // OP_LOR
   -1, // OP_IXOR
   -2, // OP_LXOR
    0, // OP_IINC
    1, // OP_I2L
    0, // OP_I2F
    1, // OP_I2D
   -1, // OP_L2I
   -1, // OP_L2F
    0, // OP_L2D
    0, // OP_F2I
    1, // OP_F2L
    1, // OP_F2D
   -1, // OP_D2I
    0, // OP_D2L
   -1, // OP_D2F
    0, // OP_I2B
    0, // OP_I2C
    0, // OP_I2S
   -3, // OP_LCMP
   -1, // OP_FCMPL
   -1, // OP_FCMPG
   -3, // OP_DCMPL
   -3, // OP_DCMPG
   -1, // OP_IFEQ
   -1, // OP_IFNE
   -1, // OP_IFLT
   -1, // OP_IFGE
   -1, // OP_IFGT
   -1, // OP_IFLE
   -2, // OP_IF_ICMPEQ
   -2, // OP_IF_ICMPNE
   -2, // OP_IF_ICMPLT
   -2, // OP_IF_ICMPGE
   -2, // OP_IF_ICMPGT
   -2, // OP_IF_ICMPLE
   -2, // OP_IF_ACMPEQ
   -2, // OP_IF_ACMPNE
    0, // OP_GOTO
    0, // OP_JSR, caller must adjust stack by +1 at jsr target
    0, // OP_RET
   -1, // OP_TABLESWITCH
   -1, // OP_LOOKUPSWITCH
   -1, // OP_IRETURN
   -2, // OP_LRETURN
   -1, // OP_FRETURN
   -2, // OP_DRETURN
   -1, // OP_ARETURN
    0, // OP_RETURN
    1, // OP_GETSTATIC, caller must adjust +1 if long or double
   -1, // OP_PUTSTATIC, caller must adjust -1 if long or double
    0, // OP_GETFIELD, caller must adjust +1 if long or double
   -2, // OP_PUTFIELD, caller must adjust -1 if long or double
   -1, // OP_INVOKEVIRTUAL,  caller must adjust +return-args_length
   -1, // OP_INVOKESPECIAL,  caller must adjust +return-args_length
    0, // OP_INVOKESTATIC,   caller must adjust +return-args_length
   -1, // OP_INVOKEINTERFACE, caller must adjust +return-args_length
    0, // OP_XXXUNUSEDXXX
    1, // OP_NEW
    0, // OP_NEWARRAY
    0, // OP_ANEWARRAY
    0, // OP_ARRAYLENGTH
   -1, // OP_ATHROW
    0, // OP_CHECKCAST
    0, // OP_INSTANCEOF
   -1, // OP_MONITORENTER
   -1, // OP_MONITOREXIT
    0, // OP_WIDE
    0, // OP_MULTIANEWARRAY, caller must adjust 1-dims
   -1, // OP_IFNULL
   -1, // OP_IFNONNULL
    0, // OP_GOTO_W
    0  // OP_JSR_W, caller must adjust stack by +1 at jsr target
    // no need to list SOFTWARE and HARDWARE here
};


} // Close namespace Jopa block

