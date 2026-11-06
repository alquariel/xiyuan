MODULE testmod copy
      PROC file_attributes()
            PROGRAM_ID "testmod copy";
            MATERIAL "CF/PA_3x12.7";
            NOMINAL_WIDTH 12.7;
            NUMBER_OF_TOWS 3;
            WATER_COOLING_TEMP 30;
            LAYER_Thickness 0.18;
            FIBER_ANGLE 0, 45, 90, 135;
            LAYER_NUMBER 1;
      ENDPROC

      PROC main_task()
            BEGIN_PROGRAM;
            BEGIN_LAYER;
            LAYER_ID 1;
            BEGIN_TRACE;
            TRACE_ID 1;
            COMPACTION_PRESSURE 80;
            LASER_EMISSION 1;
            LASER_POWER 0;
            CUTTER_STATE 0;
            COMPACTION_ROLLER_STATE 1;
            PINCH_ROLLER_STATE 0;
            LAYING_SPEED 900,150,50,100;
            EXTERNAL_AXIS_DISPLACEMENT 0,0,0;
            TOW_TENSION "xxx";
            HEAD_ANGLE 0;
            FEED_LENGTH 35,15.5;
            FEED_SPEED 3.6,1.3;
            PLC_FEED_SPEED 0;
            MoveL [[-540.00,0.00,297.70],[-0.000000,0.258819,-0.965926,0.000000],[0,0,0,0],[0,0,0,0,0,0]],[900,150,50,100],currentZone,currentTool\WObj:=currentWobj;
            MoveL [[-488.00,0.00,211.70],[-0.000000,0.258819,-0.965926,0.000000],[0,0,0,0],[0,0,0,0,0,0]],[114,150,50,100],currentZone,currentTool\WObj:=currentWobj;
            PLC_FEED_SPEED 3.6;
            LASER_POWER 8;
            LASER_EMISSION 0;
            MoveL [[-440.00,0.00,197.70],[-0.000000,0.258819,-0.965926,0.000000],[0,0,0,0],[0,0,0,0,0,0]],[114,150,50,100],currentZone,currentTool\WObj:=currentWobj;
            PLC_FEED_SPEED 0;
            MoveL [[-390.00,0.00,197.70],[-0.000000,0.258819,-0.965926,0.000000],[0,0,0,0],[0,0,0,0,0,0]],[200,200,200,100],currentZone,currentTool\WObj:=currentWobj;
            LASER_POWER 0;
            MoveL [[-340.00,0.00,197.70],[-0.000000,0.258819,-0.965926,0.000000],[0,0,0,0],[0,0,0,0,0,0]],[200,200,200,100],currentZone,currentTool\WObj:=currentWobj;
            MoveL [[-290.00,0.00,197.70],[-0.000000,0.258819,-0.965926,0.000000],[0,0,0,0],[0,0,0,0,0,0]],[200,200,200,100],currentZone,currentTool\WObj:=currentWobj;
            MoveL [[-240.00,0.00,197.70],[-0.000000,0.258819,-0.965926,0.000000],[0,0,0,0],[0,0,0,0,0,0]],[200,200,200,100],currentZone,currentTool\WObj:=currentWobj;
            MoveL [[-190.00,0.00,197.70],[-0.000000,0.258819,-0.965926,0.000000],[0,0,0,0],[0,0,0,0,0,0]],[200,200,200,100],currentZone,currentTool\WObj:=currentWobj;
            MoveL [[-140.00,0.00,197.70],[-0.000000,0.258819,-0.965926,0.000000],[0,0,0,0],[0,0,0,0,0,0]],[200,200,200,100],currentZone,currentTool\WObj:=currentWobj;
            MoveL [[-90.00,0.00,197.70],[-0.000000,0.258819,-0.965926,0.000000],[0,0,0,0],[0,0,0,0,0,0]],[200,200,200,100],currentZone,currentTool\WObj:=currentWobj;
            MoveL [[-40.00,0.00,197.70],[-0.000000,0.258819,-0.965926,0.000000],[0,0,0,0],[0,0,0,0,0,0]],[200,200,200,100],currentZone,currentTool\WObj:=currentWobj;
            MoveL [[10.00,0.00,197.70],[-0.000000,0.258819,-0.965926,0.000000],[0,0,0,0],[0,0,0,0,0,0]],[200,200,200,100],currentZone,currentTool\WObj:=currentWobj;
            MoveL [[60.00,0.00,197.70],[-0.000000,0.258819,-0.965926,0.000000],[0,0,0,0],[0,0,0,0,0,0]],[200,200,200,100],currentZone,currentTool\WObj:=currentWobj;
            MoveL [[110.00,0.00,197.70],[-0.000000,0.258819,-0.965926,0.000000],[0,0,0,0],[0,0,0,0,0,0]],[200,200,200,100],currentZone,currentTool\WObj:=currentWobj;
            MoveL [[130.00,0.00,197.70],[-0.000000,0.258819,-0.965926,0.000000],[0,0,0,0],[0,0,0,0,0,0]],[200,200,200,100],currentZone,currentTool\WObj:=currentWobj;
            MoveL [[135.00,0.00,197.70],[-0.000000,0.258819,-0.965926,0.000000],[0,0,0,0],[0,0,0,0,0,0]],[200,200,200,100],currentZone,currentTool\WObj:=currentWobj;
            CUTTER_STATE 1;
            MoveL [[260.00,0.00,197.70],[-0.000000,0.258819,-0.965926,0.000000],[0,0,0,0],[0,0,0,0,0,0]],[90,150,50,100],currentZone,currentTool\WObj:=currentWobj;
            CUTTER_STATE 0
            MoveL [[275.00,0.00,197.70],[-0.000000,0.258819,-0.965926,0.000000],[0,0,0,0],[0,0,0,0,0,0]],[90,150,50,100],currentZone,currentTool\WObj:=currentWobj;
            MoveL [[305.00,0.00,197.70],[-0.000000,0.258819,-0.965926,0.000000],[0,0,0,0],[0,0,0,0,0,0]],[90,150,50,100],currentZone,currentTool\WObj:=currentWobj;
            PLC_FEED_SPEED 1.3;
            LASER_POWER 1;
            LASER_EMISSION 1;
            MoveL [[305.00,0.00,397.70],[-0.000000,0.258819,-0.965926,0.000000],[0,0,0,0],[0,0,0,0,0,0]],[25,150,50,100],currentZone,currentTool\WObj:=currentWobj;
            PLC_FEED_SPEED 0;
            LASER_POWER 0;
            MoveL [[305.00,0.00,397.70],[-0.000000,0.258819,-0.965926,0.000000],[0,0,0,0],[0,0,0,0,0,0]],[25,150,50,100],currentZone,currentTool\WObj:=currentWobj;
            MoveL [[305.00,0.00,397.70],[-0.000000,0.258819,-0.965926,0.000000],[0,0,0,0],[0,0,0,0,0,0]],[25,150,50,100],currentZone,currentTool\WObj:=currentWobj;
            MoveL [[305.00,0.00,397.70],[-0.000000,0.258819,-0.965926,0.000000],[0,0,0,0],[0,0,0,0,0,0]],[25,150,50,100],currentZone,currentTool\WObj:=currentWobj;
            MoveL [[305.00,0.00,397.70],[-0.000000,0.258819,-0.965926,0.000000],[0,0,0,0],[0,0,0,0,0,0]],[25,150,50,100],currentZone,currentTool\WObj:=currentWobj;
            END_TRACE;
            END_LAYER;
            END_PROGRAM;
      ENDPROC
ENDMODULE
