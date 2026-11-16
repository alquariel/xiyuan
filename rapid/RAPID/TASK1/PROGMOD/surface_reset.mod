MODULE surface_reset
    PROC surface_r()
        CONST robtarget Target_290:=[[1192.034829877,824.7783174,-427.027498953],[0.004674547,0.999940989,-0.008815887,0.004295127],[-1,-1,1,0],[9E+09,9E+09,9E+09,9E+09,9E+09,9E+09]];
        MoveL Target_290,v1000,z100,xykTool\WObj:=currentWobj;
    ENDPROC
ENDMODULE