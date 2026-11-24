MODULE TASK_LOAD
      PERS bool connected;
      VAR string module_name;
      VAR num modulePos;
      VAR bool MODFLAG;

      PROC main_load()
            !            !WaitTime(0.5);
            !            BEGIN_PROGRAM;
            !!            !WaitTime(0.5);
            !            PROGRAM_ID "Demo_Boat_Layer12";
            !!            !WaitTime(0.5);
            !            MATERIAL "CF/PA_3x12.7";
            !            !WaitTime(0.5);
            !            NOMINAL_WIDTH 12.7;
            !            !WaitTime(0.5);
            !            NUMBER_OF_TOWS 3;
            !            !WaitTime(0.5);
            !            WATER_COOLING_TEMP 30;
            !            !WaitTime(0.5);
            !            LAYER_Thickness 0.18;
            !            !WaitTime(0.5);
            !            FIBER_ANGLE 0,45,90,135;
            !            !WaitTime(0.5);
            !            LAYER_NUMBER 12;
            !            !WaitTime(0.5);
            !            BEGIN_LAYER;
            !            !WaitTime(0.5);
            !            LAYER_ID 1;
            !            !WaitTime(0.5);
            !            BEGIN_TRACE;
            !            !WaitTime(0.5);
            !            TRACE_ID 1;
            !            !WaitTime(0.5);
            !            COMPACTION_PRESSURE 80;
            !            !WaitTime(0.5);
            !            LASER_EMISSION 0;
            !            !WaitTime(0.5);
            !            LASER_POWER 80;
            !            !WaitTime(0.5);
            !            CUTTER_STATE 0;
            !            !WaitTime(0.5);
            !            COMPACTION_ROLLER_STATE 1;
            !            !WaitTime(0.5);
            !            PINCH_ROLLER_STATE 1;
            !            !WaitTime(0.5);
            !            LAYING_SPEED 80,95,0,0;
            !            !WaitTime(0.5);
            !            EXTERNAL_AXIS_DISPLACEMENT 0,90,0;
            !            !WaitTime(0.5);
            !            TOW_TENSION "xxx";
            !            !WaitTime(0.5);
            !            HEAD_ANGLE 0;
            !            !WaitTime(0.5);
            !            PRE_FEED_LENGTH 35;
            !            !WaitTime(0.5);
            !            PRE_FEED_SPEED 3.6;
            !            !WaitTime(0.5);
            !            POST_FEED_LENGTH 15.5;
            !            !WaitTime(0.5);
            !            POST_FEED_SPEED 1.2;
            !            !WaitTime(0.5);
            !            !MOVEL_TO "X+0725.11 Y+0265.13 Z+0252.56 Qw+0.8772 Qx-0.0431 Qy-0.0802 Qz-0.4715 GRA+0 TA+0 RA+0 SRL800 SRO95 SEL0 SEO0";
            !            !WaitTime(0.5);
            !            !MOVEL_TO "X+0688.65 Y+0285.92 Z+0161.25 Qw+0.8772 Qx-0.0431 Qy-0.0802 Qz-0.4715 GRA+0 TA+0 RA+0 SRL114 SRO95 SEL0 SEO0";
            !            !WaitTime(0.5);
            !            PRE_FEED_SPEED 3.6;
            !            !WaitTime(0.5);
            !            PINCH_ROLLER_STATE 1;
            !            !WaitTime(0.5);
            !            LASER_POWER 80;
            !            !WaitTime(0.5);
            !            !MOVEL_TO "X+0655.00 Y+0317.02 Z+0141.24 Qw+0.8772 Qx-0.0431 Qy-0.0802 Qz-0.4715 GRA+0 TA+0 RA+0 SRL114 SRO95 SEL0 SEO0";
            !            !WaitTime(0.5);
            !            PINCH_ROLLER_STATE 1;
            !            !WaitTime(0.5);
            !            CUTTER_STATE 1;
            !            !WaitTime(0.5);
            !            !MOVEL_TO "X+0511.97 Y+0460.04 Z+0114.74 Qw+0.8772 Qx-0.0431 Qy-0.0802 Qz-0.4715 GRA+0 TA+0 RA+0 SRL80 SRO95 SEL0 SEO0";
            !            !WaitTime(0.5);
            !            LASER_EMISSION 1;
            !            !WaitTime(0.5);
            !            !MOVEL_TO "X+0502.16 Y+0469.86 Z+0112.92 Qw+0.8772 Qx-0.0431 Qy-0.0802 Qz-0.4715 GRA+0 TA+0 RA+0 SRL90 SRO95 SEL0 SEO0";
            !            !WaitTime(0.5);
            !            !MOVEL "X+0480.39 Y+0491.63 Z+0108.88 Qw+0.8772 Qx-0.0431 Qy-0.0802 Qz-0.4715 GRA+0 TA+0 RA+0 SRL90 SRO95 SEL0 SEO0";
            !            !WaitTime(0.5);
            !            POST_FEED_SPEED 1.2;
            !            !WaitTime(0.5);
            !            !MOVEL_TO "X+0480.39 Y+0491.63 Z+0308.88 Qw+0.8772 Qx-0.0431 Qy-0.0802 Qz-0.4715 GRA+0 TA+0 RA+0 SRL25 SRO95 SEL0 SEO0";
            !            !WaitTime(0.5);
            !            END_TRACE;
            !            !WaitTime(0.5);
            !            BEGIN_TRACE;
            !            !WaitTime(0.5);
            !            TRACE_ID 2;
            !            !WaitTime(0.5);
            !            COMPACTION_PRESSURE 80;
            !            !WaitTime(0.5);
            !            LASER_EMISSION 0;
            !            !WaitTime(0.5);
            !            LASER_POWER 80;
            !            !WaitTime(0.5);
            !            CUTTER_STATE 0;
            !            !WaitTime(0.5);
            !            COMPACTION_ROLLER_STATE 1;
            !            !WaitTime(0.5);
            !            PINCH_ROLLER_STATE 1;
            !            !WaitTime(0.5);
            !            LAYING_SPEED 80,95,0,0;
            !            !WaitTime(0.5);
            !            EXTERNAL_AXIS_DISPLACEMENT 0,90,0;
            !            !WaitTime(0.5);
            !            TOW_TENSION "xxx";
            !            !WaitTime(0.5);
            !            HEAD_ANGLE 0;
            !            !WaitTime(0.5);
            !            PRE_FEED_LENGTH 35;
            !            !WaitTime(0.5);
            !            PRE_FEED_SPEED 3.6;
            !            !WaitTime(0.5);
            !            POST_FEED_LENGTH 15.5;
            !            !WaitTime(0.5);
            !            POST_FEED_SPEED 1.2;
            !            !WaitTime(0.5);
            !            !MOVEL_TO "X+0725.11 Y+0211.13 Z+0262.57 Qw+0.8772 Qx-0.0431 Qy-0.0802 Qz-0.4715 GRA+0 TA+0 RA+0 SRL800 SRO95 SEL0 SEO0";
            !            !WaitTime(0.5);
            !            !MOVEL_TO "X+0688.65 Y+0231.91 Z+0171.26 Qw+0.8772 Qx-0.0431 Qy-0.0802 Qz-0.4715 GRA+0 TA+0 RA+0 SRL114 SRO95 SEL0 SEO0";
            !            !WaitTime(0.5);
            !            PRE_FEED_SPEED 3.6;
            !            !WaitTime(0.5);
            !            PINCH_ROLLER_STATE 1;
            !            !WaitTime(0.5);
            !            LASER_POWER 80;
            !            !WaitTime(0.5);
            !            !MOVEL_TO "X+0619.94 Y+0298.07 Z+0144.76 Qw+0.8772 Qx-0.0431 Qy-0.0802 Qz-0.4715 GRA+0 TA+0 RA+0 SRL114 SRO95 SEL0 SEO0";
            !            !WaitTime(0.5);
            !            PINCH_ROLLER_STATE 1;
            !            !WaitTime(0.5);
            !            CUTTER_STATE 1;
            !            !WaitTime(0.5);
            !            !MOVEL_TO "X+0458.69 Y+0459.33 Z+0114.87 Qw+0.8772 Qx-0.0431 Qy-0.0802 Qz-0.4715 GRA+0 TA+0 RA+0 SRL80 SRO95 SEL0 SEO0";
            !            !WaitTime(0.5);
            !            LASER_EMISSION 1;
            !            !WaitTime(0.5);
            !            !MOVEL_TO "X+0447.47 Y+0470.55 Z+0112.79 Qw+0.8772 Qx-0.0431 Qy-0.0802 Qz-0.4715 GRA+0 TA+0 RA+0 SRL90 SRO95 SEL0 SEO0";
            !            !WaitTime(0.5);
            !            !MOVEL "X+0426.39 Y+0491.63 Z+0108.88 Qw+0.8772 Qx-0.0431 Qy-0.0802 Qz-0.4715 GRA+0 TA+0 RA+0 SRL90 SRO95 SEL0 SEO0";
            !            !WaitTime(0.5);
            !            POST_FEED_SPEED 1.2;
            !            !WaitTime(0.5);
            !            !MOVEL_TO "X+0426.39 Y+0491.63 Z+0308.88 Qw+0.8772 Qx-0.0431 Qy-0.0802 Qz-0.4715 GRA+0 TA+0 RA+0 SRL25 SRO95 SEL0 SEO0";
            !            !WaitTime(0.5);
            !            END_TRACE;
            !            !WaitTime(0.5);
            !            END_LAYER;
            !            !WaitTime(0.5);
            !            BEGIN_LAYER;
            !            !WaitTime(0.5);
            !            LAYER_ID 2;
            !            !WaitTime(0.5);
            !            BEGIN_TRACE;
            !            !WaitTime(0.5);
            !            TRACE_ID 1;
            !            !WaitTime(0.5);
            !            COMPACTION_PRESSURE 80;
            !            !WaitTime(0.5);
            !            LASER_EMISSION 0;
            !            !WaitTime(0.5);
            !            LASER_POWER 80;
            !            !WaitTime(0.5);
            !            CUTTER_STATE 0;
            !            !WaitTime(0.5);
            !            COMPACTION_ROLLER_STATE 1;
            !            !WaitTime(0.5);
            !            PINCH_ROLLER_STATE 1;
            !            !WaitTime(0.5);
            !            LAYING_SPEED 80,95,0,0;
            !            !WaitTime(0.5);
            !            EXTERNAL_AXIS_DISPLACEMENT 0,90,0;
            !            !WaitTime(0.5);
            !            TOW_TENSION "xxx";
            !            !WaitTime(0.5);
            !            HEAD_ANGLE -45;
            !            !WaitTime(0.5);
            !            PRE_FEED_LENGTH 35;
            !            !WaitTime(0.5);
            !            PRE_FEED_SPEED 3.6;
            !            !WaitTime(0.5);
            !            POST_FEED_LENGTH 15.5;
            !            !WaitTime(0.5);
            !            POST_FEED_SPEED 1.2;
            !            !WaitTime(0.5);
            !            !MOVEL_TO "X+0430.33 Y-0560.66 Z+0193.53 Qw+0.8775 Qx+0.0448 Qy+0.0835 Qz-0.4700 GRA+0 TA+0 RA+0 SRL800 SRO95 SEL0 SEO0";
            !            !WaitTime(0.5);
            !            !MOVEL_TO "X+0466.76 Y-0507.91 Z+0116.13 Qw+0.8775 Qx+0.0448 Qy+0.0835 Qz-0.4700 GRA+0 TA+0 RA+0 SRL114 SRO95 SEL0 SEO0";
            !            !WaitTime(0.5);
            !            PRE_FEED_SPEED 3.6;
            !            !WaitTime(0.5);
            !            PINCH_ROLLER_STATE 1;
            !            !WaitTime(0.5);
            !            LASER_POWER 80;
            !            !WaitTime(0.5);
            !            !MOVEL_TO "X+0500.39 Y-0471.63 Z+0108.88 Qw+0.8775 Qx+0.0448 Qy+0.0835 Qz-0.4700 GRA+0 TA+0 RA+0 SRL114 SRO95 SEL0 SEO0";
            !            !WaitTime(0.5);
            !            PINCH_ROLLER_STATE 1;
            !            !WaitTime(0.5);
            !            CUTTER_STATE 1;
            !            !WaitTime(0.5);
            !            !MOVEL_TO "X+0643.31 Y-0328.71 Z+0136.49 Qw+0.8775 Qx+0.0448 Qy+0.0835 Qz-0.4700 GRA+0 TA+0 RA+0 SRL80 SRO95 SEL0 SEO0";
            !            !WaitTime(0.5);
            !            LASER_EMISSION 1;
            !            !WaitTime(0.5);
            !            !MOVEL_TO "X+0643.31 Y-0328.71 Z+0136.49 Qw+0.8775 Qx+0.0448 Qy+0.0835 Qz-0.4700 GRA+0 TA+0 RA+0 SRL90 SRO95 SEL0 SEO0";
            !            !WaitTime(0.5);
            !            !MOVEL "X+0653.12 Y-0318.90 Z+0138.39 Qw+0.8775 Qx+0.0448 Qy+0.0835 Qz-0.4700 GRA+0 TA+0 RA+0 SRL90 SRO95 SEL0 SEO0";
            !            !WaitTime(0.5);
            !            POST_FEED_SPEED 1.2;
            !            !WaitTime(0.5);
            !            !MOVEL_TO "X+0675.00 Y-0297.02 Z+0142.62 Qw+0.8775 Qx+0.0448 Qy+0.0835 Qz-0.4700 GRA+0 TA+0 RA+0 SRL25 SRO95 SEL0 SEO0";
            !            !WaitTime(0.5);
            !            END_TRACE;
            !            !WaitTime(0.5);
            !            BEGIN_TRACE;
            !            !WaitTime(0.5);
            !            TRACE_ID 2;
            !            !WaitTime(0.5);
            !            COMPACTION_PRESSURE 80;
            !            !WaitTime(0.5);
            !            LASER_EMISSION 0;
            !            !WaitTime(0.5);
            !            LASER_POWER 80;
            !            !WaitTime(0.5);
            !            CUTTER_STATE 0;
            !            !WaitTime(0.5);
            !            COMPACTION_ROLLER_STATE 1;
            !            !WaitTime(0.5);
            !            PINCH_ROLLER_STATE 1;
            !            !WaitTime(0.5);
            !            LAYING_SPEED 80,95,0,0;
            !            !WaitTime(0.5);
            !            EXTERNAL_AXIS_DISPLACEMENT 0,90,0;
            !            !WaitTime(0.5);
            !            TOW_TENSION "xxx";
            !            !WaitTime(0.5);
            !            HEAD_ANGLE -45;
            !            !WaitTime(0.5);
            !            PRE_FEED_LENGTH 35;
            !            !WaitTime(0.5);
            !            PRE_FEED_SPEED 3.6;
            !            !WaitTime(0.5);
            !            POST_FEED_LENGTH 15.5;
            !            !WaitTime(0.5);
            !            POST_FEED_SPEED 1.2;
            !            !WaitTime(0.5);
            !            !MOVEL_TO "X+0376.33 Y-0560.66 Z+0193.53 Qw+0.8775 Qx+0.0448 Qy+0.0835 Qz-0.4700 GRA+0 TA+0 RA+0 SRL800 SRO95 SEL0 SEO0";
            !            !WaitTime(0.5);
            !            !MOVEL_TO "X+0412.76 Y-0507.91 Z+0116.13 Qw+0.8775 Qx+0.0448 Qy+0.0835 Qz-0.4700 GRA+0 TA+0 RA+0 SRL114 SRO95 SEL0 SEO0";
            !            !WaitTime(0.5);
            !            PRE_FEED_SPEED 3.6;
            !            !WaitTime(0.5);
            !            PINCH_ROLLER_STATE 1;
            !            !WaitTime(0.5);
            !            LASER_POWER 80;
            !            !WaitTime(0.5);
            !            !MOVEL_TO "X+0446.39 Y-0471.63 Z+0108.88 Qw+0.8775 Qx+0.0448 Qy+0.0835 Qz-0.4700 GRA+0 TA+0 RA+0 SRL114 SRO95 SEL0 SEO0";
            !            !WaitTime(0.5);
            !            PINCH_ROLLER_STATE 1;
            !            !WaitTime(0.5);
            !            CUTTER_STATE 1;
            !            !WaitTime(0.5);
            !            !MOVEL_TO "X+0642.56 Y-0275.46 Z+0146.78 Qw+0.8775 Qx+0.0448 Qy+0.0835 Qz-0.4700 GRA+0 TA+0 RA+0 SRL80 SRO95 SEL0 SEO0";
            !            !WaitTime(0.5);
            !            LASER_EMISSION 1;
            !            !WaitTime(0.5);
            !            !MOVEL_TO "X+0653.77 Y-0264.25 Z+0148.95 Qw+0.8775 Qx+0.0448 Qy+0.0835 Qz-0.4700 GRA+0 TA+0 RA+0 SRL90 SRO95 SEL0 SEO0";
            !            !WaitTime(0.5);
            !            !MOVEL "X+0675.00 Y-0243.02 Z+0153.05 Qw+0.8775 Qx+0.0448 Qy+0.0835 Qz-0.4700 GRA+0 TA+0 RA+0 SRL90 SRO95 SEL0 SEO0";
            !            !WaitTime(0.5);
            !            POST_FEED_SPEED 1.2;
            !            !WaitTime(0.5);
            !            !MOVEL_TO "X+0675.00 Y-0243.02 Z+0353.05 Qw+0.8775 Qx+0.0448 Qy+0.0835 Qz-0.4700 GRA+0 TA+0 RA+0 SRL25 SRO95 SEL0 SEO0";
            !            !WaitTime(0.5);
            !            END_TRACE;
            !            !WaitTime(0.5);
            !            END_LAYER;
            !            !WaitTime(0.5);
            !            END_PROGRAM;





            !            VAR bool connected_temp:=FALSE;
            !            WHILE connected_temp=FALSE DO
            !!                  TPWrite "connected:"\Bool:=connected;
            !                  connected_temp:=connected;
            !                  IF connected_temp THEN
            !!                        TPWrite "connected_temp:"\Bool:=connected_temp;
            !                        !                  Load "HOME:" \File:="G_code_test_formatted_final.mod";
            !                        Load\Dynamic,diskhome\File:="G_code_test_formatted_final.mod";
            !!                        TPWrite "TASK_LOAD";
            !                        %"G_code_test_formatted_final:main_task"%;
            !                        !                        %"G_code_test_formatted_final:main"%;
            !                        !            %"main_1"%;
            !                        connected_temp:=FALSE;
            !                  ENDIF
            !            ENDWHILE
            !            UnLoad "HOME:"\File:="G_code_test_formatted_final.mod";
            !            TPWrite "TASK_UNLOAD";





            !            VAR bool connected_temp:=FALSE;
            !            WHILE connected_temp=FALSE DO
            !                  TPWrite "connected:"\Bool:=connected;
            !            connected_temp:=connected;
            !            IF connected_temp THEN
            !                        TPWrite "connected_temp:"\Bool:=connected_temp;
            ! Check load mod whether exist


            IF FileSize("HOME:"+mod_filename)>0 THEN
                  TPWrite "file already exist HOME";

                  ! Delete ".mod" suffix
                  modulePos:=StrMatch(mod_filename,1,".mod");
                  IF modulePos>0 THEN
                        module_name:=StrPart(mod_filename,1,modulePos-1);
                  ELSE
                        module_name:=mod_filename;
                  ENDIF

                  TPWrite "module_name: "+module_name;

                  MODFLAG:=ModExist(module_name);
                  !            TPWrite "MODGLAG:"\Bool:=MODFLAG;
                  IF MODFLAG THEN
                        !                  EraseModule "G_code_test_formatted_final.mod";
                        UnLoad "HOME:"\File:=mod_filename;
                        !                  TPWrite "Removed existing MOD file";
                  ENDIF
                  ! to load new path mod file
                  Load\Dynamic,diskhome\File:=mod_filename;
                  !            TPWrite "TASK_LOAD";
                  %module_name+":file_attributes"%;
                  %module_name+":main_task"%;

                  
                  ! mod execute finished
                  TPWrite "Mod execute finished";
                  UnLoad "HOME:"\File:=module_name+".mod";
                  !            TPWrite "TASK_UNLOAD";

            ELSE
                  TPWrite "HOME:"+mod_filename;
            ENDIF


            !            MODFLAG:=ModExist("G_code_test_formatted_final");
            !            !            TPWrite "MODGLAG:"\Bool:=MODFLAG;
            !            IF MODFLAG THEN
            !                  !                  EraseModule "G_code_test_formatted_final.mod";
            !                  UnLoad "HOME:"\File:="G_code_test_formatted_final.mod";
            !                  !                  TPWrite "Removed existing MOD file";
            !            ENDIF
            !            ! to load new path mod file
            !            Load\Dynamic,diskhome\File:="G_code_test_formatted_final.mod";
            !            !            TPWrite "TASK_LOAD";
            !            %"G_code_test_formatted_final:main_task"%;
            !            !                        %"G_code_test_formatted_final:main"%;
            !            !            %"main_1"%;
            !            !                  connected_temp:=FALSE;
            !            !            ENDIF
            !            !            ENDWHILE
            !            UnLoad "HOME:"\File:="G_code_test_formatted_final.mod";
            !            !            TPWrite "TASK_UNLOAD";
      ENDPROC
ENDMODULE