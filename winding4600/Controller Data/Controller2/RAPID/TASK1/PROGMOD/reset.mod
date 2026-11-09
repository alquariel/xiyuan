MODULE reset
    PROC resetJoint()
        CONST jointtarget pHome:=[[-26.98,39.72,12.21,0.43,38.54,-117.32],[0,0,0,0,0,0]];
        MoveAbsJ pHome,v500,fine,tool0;
    ENDPROC

ENDMODULE