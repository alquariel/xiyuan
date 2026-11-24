MODULE PLC_ROBOT_COMM_GI_GO

      ! ===== Command and state constants =====
      CONST num CMD_NONE:=0;
      CONST num CMD_START:=10;
      CONST num CMD_PAUSE:=11;
      CONST num CMD_RESUME:=12;
      CONST num CMD_STOP:=13;

      CONST num RSTATE_IDLE:=0;
      CONST num RSTATE_PRINTING:=1;
      CONST num RSTATE_PAUSED:=2;
      CONST num RSTATE_STOPPED:=3;
      CONST num RSTATE_ERROR:=4;

      CONST num ERR_NONE:=0;
      CONST num ERR_INVALID_CMD:=10;
      CONST num ERR_INTERNAL:=11;

      ! ===== Robot status variables =====
      VAR num gRobotState:=RSTATE_IDLE;
      VAR num gRobotErrorCode:=ERR_NONE;
      VAR num gProgress:=0;

      ! Last handled command information
      VAR num gLastHandledSeq:=0;
      VAR num gLastCmd:=CMD_NONE;

      ! ===== Per-cycle received command fields =====
      VAR num rCmd:=0;
      VAR num rSeq:=0;
      VAR num rChecksum:=0;
      VAR num rHeartbeat:=0;

      VAR bool rChecksumOK:=FALSE;
      VAR bool rIsNewCmd:=FALSE;

      VAR num rAckSeqToSend:=0;

      ! =========================================================
      !   Process stubs - replace these with your real logic
      ! =========================================================

      PROC DoStartPrint()
            ! Start printing process here
            TPWrite "DoStartPrint=========================";
            gRobotState:=RSTATE_PRINTING;
            gRobotErrorCode:=ERR_NONE;
      ENDPROC

      PROC DoPausePrint()
            ! Pause printing here
            TPWrite "DoPausePrint=========================";
            StopMove\AllMotionTasks;
            IF gRobotState=RSTATE_PRINTING THEN
                  gRobotState:=RSTATE_PAUSED;
            ENDIF
      ENDPROC

      PROC DoResumePrint()
            ! Resume printing here
            TPWrite "DoResumePrint=========================";
            StartMove\AllMotionTasks;
            IF gRobotState=RSTATE_PAUSED THEN
                  gRobotState:=RSTATE_PRINTING;
            ENDIF
      ENDPROC

      PROC DoStopPrint()
            ! Stop printing here
            TPWrite "DoStopPrint=========================";
            Stop\AllMoveTasks;
            gRobotState:=RSTATE_STOPPED;
      ENDPROC
      
      
      
      ! ===== Read 4 bytes from PLC via GI1..GI4 =====
      PROC ReadFromPLC()
            rCmd:=GI1;
            ! command
            rSeq:=GI2;
            ! sequence
            rChecksum:=GI3;
            ! checksum = Cmd BXOR Seq
            rHeartbeat:=GI4;
            ! heartbeat

            rChecksumOK:=(rChecksum=BitXOr(rCmd,rSeq));
            rIsNewCmd:=FALSE;
!            ! test tpwrite
!            TPWrite "rCmd:"+ValToStr(rCmd);
!            TPWrite "rSeq:"+ValToStr(rSeq);
!            TPWrite "rChecksum:"+ValToStr(rChecksum);
!            TPWrite "rHeartbeat:"+ValToStr(rHeartbeat);
!            TPWrite "rChecksumOK:"+ValToStr(rChecksumOK);

            IF rCmd<>CMD_NONE AND rChecksumOK THEN
                  IF rSeq<>gLastHandledSeq THEN
                        rIsNewCmd:=TRUE;
                  ENDIF
            ENDIF
      ENDPROC

      ! ===== Handle command once per new sequence =====
      PROC ProcessCommand()
!            TPWrite "INTER ProcessCommand";
            ! Invalid checksum: set error code, do not execute
            IF NOT rChecksumOK THEN
!                  TPWrite "inter NOT rChecksumOK";
                  gRobotErrorCode:=ERR_INVALID_CMD;
                  RETURN ;
            ENDIF

            ! Not a new command: nothing to do
            IF NOT rIsNewCmd THEN
!                  TPWrite "inter NOT rIsNewCmd";
                  RETURN ;
            ENDIF

            gRobotErrorCode:=ERR_NONE;

!            TPWrite "ProcessCommand rCmd:"+ValToStr(rCmd);
            TEST rCmd
            CASE CMD_START:
                  DoStartPrint;
            CASE CMD_PAUSE:
                  DoPausePrint;
            CASE CMD_RESUME:
                  DoResumePrint;
            CASE CMD_STOP:
                  DoStopPrint;
            DEFAULT:
                  gRobotErrorCode:=ERR_INVALID_CMD;
                  TPWrite "ErrorCode:"+ValToStr(gRobotErrorCode);
            ENDTEST

            gLastCmd:=rCmd;
            gLastHandledSeq:=rSeq;
            rAckSeqToSend:=rSeq;
      ENDPROC

      ! ===== Simple progress demo =====
      PROC UpdateProgress()
            IF gRobotState=RSTATE_PRINTING THEN
                  gProgress:=gProgress+1;
                  IF gProgress>100 THEN
                        gProgress:=100;
                  ENDIF
            ELSE
                  IF gRobotState=RSTATE_IDLE THEN
                        gProgress:=0;
                  ENDIF
            ENDIF
      ENDPROC

      ! ===== Write status bytes back to PLC via GO1..GO5 =====
      PROC WriteToPLC()
            !            TPWrite "Write GO1 gRobotState:"+ValToStr(gRobotState);
            !            TPWrite "Write GO2 rAckSeqToSend:"+ValToStr(rAckSeqToSend);
            !            TPWrite "Write GO3 gRobotErrorCode:"+ValToStr(gRobotErrorCode);
            !            TPWrite "Write GO4 rHeartbeat:"+ValToStr(rHeartbeat);
            !            TPWrite "Write GO5 gProgress:"+ValToStr(gProgress);

            SetGO GO1,gRobotState;
            WaitGO GO1,gRobotState;
            ! Robot state
            SetGO GO2,rAckSeqToSend;
            WaitGO GO2,rAckSeqToSend;
            ! Ack sequence
            SetGO GO3,gRobotErrorCode;
            WaitGO GO3,gRobotErrorCode;
            ! Error code
            SetGO GO4,rHeartbeat;
            WaitGO GO4,rHeartbeat;
            ! Heartbeat echo
            SetGO GO5,gProgress;
            WaitGO GO5,gProgress;
            ! Progress
      ENDPROC

      PROC ClearGOX()
            SetGO GO1,0;
            WaitGO GO1,0;
            ! Robot state
            SetGO GO2,0;
            WaitGO GO2,0;
            ! Ack sequence
            SetGO GO3,0;
            WaitGO GO3,0;
            ! Error code
            SetGO GO4,0;
            WaitGO GO4,0;
            ! Heartbeat echo
            SetGO GO5,0;
            WaitGO GO5,0;
            ! Progress
      ENDPROC

      
      
      
      ! ===== Main cyclic task =====
      PROC main()
            TPWrite "==========PLC_ROBOT_COMM_GI_GO started";
            !After resetting the program pointer and initializing the program, all the analog output variables will be set to 0, and the IO status of the controller will be updated. Otherwise, subsequent writes will not be possible.
            ClearGOX;
            WaitTime(1);
            WHILE TRUE DO
                  ReadFromPLC;
                  ProcessCommand;
                  UpdateProgress;
                  WriteToPLC;

!                  WaitTime(0.05);
                  ! 20 Hz loop
            ENDWHILE
      ENDPROC

ENDMODULE