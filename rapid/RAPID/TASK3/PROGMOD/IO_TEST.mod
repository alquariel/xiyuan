MODULE IO_TEST
      ! ==========================================
      ! SIMPLE PROFINET IO TEST FOR ROBOT5
      ! Function: write and read 64 bits (2x32-bit words)
      ! using existing group signals and print them.
      ! No verification or extra logic.
      ! ==========================================

      ! We treat two 32-bit group signals as one 64-bit data block

      VAR num rxWord1:=0;
      ! first 32 bits to PLC  (GO: txBuffer_1)
      VAR num rxWord2:=0;
      ! second 32 bits to PLC (GO: txBuffer_2)
      VAR num rxWord3:=0;
      VAR num rxWord4:=0;
      VAR num rxWord5:=0;
      VAR num testWord:=0;
      
      VAR num dir:=1;
      ! direction: +1 up, -1 down (0..255 sawtooth)

      VAR num txWord1:=0;
      ! first 32 bits from PLC  (GI: rxBuffer_1)
      VAR num txWord2:=0;
      ! second 32 bits from PLC (GI: rxBuffer_2)
      VAR num txWord3:=0;
      VAR num txWord4:=0;
      ! ========== MAIN TEST LOOP ==========

      PROC main_io()
            TPWrite "========================================";
            TPWrite "Simple 64-bit IO test (2x32-bit words)";
            TPWrite "RX -> txBuffer_1/2, TX <- rxBuffer_1/2";
            TPWrite "========================================";

            WHILE TRUE DO
                  ! 1. Prepare data to send: sawtooth 0..255..0
                  rxWord1:=rxWord1+dir*1;
                  rxWord2:=rxWord2+dir*2;
                  rxWord3:=rxWord3+dir*3;
                  rxWord4:=rxWord4+dir*4;
                  rxWord5:=rxWord4+dir*5;

                  ! Clamp and flip direction at bounds
                  IF rxWord1>=255 OR rxWord2>=255 OR rxWord3>=255 OR rxWord4>=255 OR rxWord5>=255 THEN
                        dir:=-1;
                        rxWord1:=255;
                        rxWord2:=255;
                        rxWord3:=255;
                        rxWord4:=255;
                        rxWord5:=255;
                  ENDIF
                  IF rxWord1<=0 OR rxWord2<=0 OR rxWord3<=0 OR rxWord4<=0 OR rxWord5<=0 THEN
                        dir:=1;
                        rxWord1:=0;
                        rxWord2:=0;
                        rxWord3:=0;
                        rxWord4:=0;
                        rxWord5:=0;
                  ENDIF

                  !            ! 2. Write to group outputs (robot -> PLC)
                  !???MB92,???PLC??GT????,GI92-155,??MB92-155
                  SetGO GO1,rxWord1;
                  SetGO GO2,rxWord2;
                  SetGO GO3,testWord;
                  SetGO GO4,rxWord4;
                  SetGO GO5,rxWord5;

                  ! 3. Read from group inputs (PLC -> robot)
                  ! NOTE: Direct assignment from 32-bit group inputs may
                  ! generate a "signal exceeds maximum bit width" warning
                  ! on some controllers, but the value can still be read.
                  txWord1:=GI1;
                  txWord2:=GI2;
                  txWord3:=GI3;
                  txWord4:=GI4;


                  ! 4. Print current values
                  TPWrite "IB92-QB92:RX1="+ValToStr(txWord1)+"  TX1="+ValToStr(rxWord1);
                  ! IB92-QB92
                  TPWrite "IB93-QB93:RX2="+ValToStr(txWord2)+"  TX2="+ValToStr(rxWord2);
                  TPWrite "IB94-QB94:RX3="+ValToStr(txWord3)+"  TX3="+ValToStr(testWord);
                  TPWrite "IB95-QB95:RX4="+ValToStr(txWord4)+"  TX4="+ValToStr(rxWord4);
                  TPWrite "IB96:TX5="+ValToStr(rxWord5);

                  !            TPWrite "RX1=" + ValToStr(txWord1) + "  RX2=" + ValToStr(txWord2);


                  ! 5. Small delay between cycles
                  WaitTime 0.5;
            ENDWHILE

      ENDPROC

ENDMODULE
