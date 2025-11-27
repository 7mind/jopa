

 IBM Research Jikes Parser Generator    1.2 Wed Nov 26 18:42:20 2025       Page 1

%OPTIONS act
%OPTIONS table
%OPTIONS gp=cpp
%OPTIONS fp=java
%OPTIONS prefix=TK_
%OPTIONS error-maps
%OPTIONS scopes
%OPTIONS names=max

Options in effect:
    ACTION  ACTFILE-NAME=javaact.h  BLOCKB=/.  BLOCKE=./  BYTE  CONFLICTS  
    DEFAULT=5  NODEBUG  DEFERRED  NOEDIT  ERROR-MAPS  ESCAPE=%  
    FILE-PREFIX=java  NOFIRST  NOFOLLOW  GENERATE-PARSER=C++  GOTO-DEFAULT  
    HACTFILE-NAME=javahdr.h  HBLOCKB=/:  HBLOCKE=:/  LALR=1  NOLIST  
    MAX-DISTANCE=30  MIN-DISTANCE=3  NAMES=MAXIMUM  NONT-CHECK  ORMARK=|  
    OUTPUT-SIZE=80  PREFIX=TK_  READ-REDUCE  SCOPES  NOSHIFT-DEFAULT  
    NOSINGLE-PRODUCTIONS  STACK-SIZE=128  NOSTATES  SUFFIX=  TABLE=SPACE  
    TRACE=CONFLICTS  NOVERBOSE  WARNINGS  NOXREF  



 IBM Research Jikes Parser Generator    1.2 Wed Nov 26 18:42:20 2025       Page 2

*** The following Terminals are useless: 


        const  goto  


This grammar is LALR(1).


 IBM Research Jikes Parser Generator    1.2 Wed Nov 26 18:42:20 2025       Page 3


Number of Terminals: 111
Number of Nonterminals: 234
Number of Productions: 567
Number of Items: 1877
Number of Scopes: 105
Number of States: 681
Number of Shift actions: 1971
Number of Goto actions: 2333
Number of Shift/Reduce actions: 3577
Number of Goto/Reduce actions: 3465
Number of Reduce actions: 3109
Number of Shift-Reduce conflicts: 0
Number of Reduce-Reduce conflicts: 0
Number of Goto entries saved by default: 1703
Number of Goto/Reduce entries saved by default: 3172
Number of non-terminals eliminated: 117
Length of base Check Table: 1839
Length of base Action Table: 1604
Number of entries in base Action Table: 1604
Percentage of increase: 0.0%
Storage required for base Tables: 5517 Bytes, 6K
Storage required for Rules: 1132 Bytes
Number of unique terminal states: 416
Number of Shift actions saved by merging: 3783
Number of Reduce actions saved by merging: 496
Number of Reduce saved by default: 2521
Length of Terminal Check Table: 2409
Length of Terminal Action Table: 2325
Number of entries in Terminal Action Table: 2273
Percentage of increase: 2.2%
Storage required for Terminal Tables: 7059 Bytes, 7K
Total storage required for Tables: 12576 Bytes, 13K


Actions in Compressed Tables:
     Number of Shifts: 780
     Number of Shift/Reduces: 985
     Number of Gotos: 630
     Number of Goto/Reduces: 293
     Number of Reduces: 92
     Number of Defaults: 234

Error maps storage:
    Storage required for ACTION_SYMBOLS_BASE map: 1362 Bytes
    Storage required for ACTION_SYMBOLS_RANGE map: 1007 Bytes
    Storage required for NACTION_SYMBOLS_BASE map: 1362 Bytes
    Storage required for NACTION_SYMBOLS_RANGE map: 578 Bytes
    Storage required for TERMINAL_INDEX map: 222 Bytes
    Storage required for NON_TERMINAL_INDEX map: 470 Bytes
    Storage required for STRING_BUFFER map: 4762 Bytes
    Storage required for NAME_START map: 694 Bytes
    Storage required for NAME_LENGTH map: 347 Bytes

***Warning: Base Check vector contains value > 255. 16-bit words used.
