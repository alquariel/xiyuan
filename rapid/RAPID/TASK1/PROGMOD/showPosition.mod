MODULE showPosition
    !//???????
    PROC ShowTcp()
        VAR robtarget p;
        p:=CRobT(\Tool:=currentTool\WObj:=currentWobj);
        TPWrite "trans: "\Pos:=p.trans;
        TPWrite "rot  : "\Orient:=p.rot;
    ENDPROC
ENDMODULE