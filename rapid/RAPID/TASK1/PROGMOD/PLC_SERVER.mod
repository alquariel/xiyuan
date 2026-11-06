MODULE PLC_SERVER
      VAR robtarget ptmp;

      ! ===== PLC Command Handshake (Composite command mode) =====
      ! PLC -> Robot (unused area from MB219 / MD219):
      !   %MD219 : SendCmdCode (DINT)  (bytes: MB219..MB222)
      !   %MD223 : SendSeq     (DINT)  (bytes: MB223..MB226) (commit point; write this last)
      ! Robot -> PLC (free area MB145..MB155):
      !   %MD145 : RecvAckSeq  (DINT)  (latched/accepted)
      !   %MD149 : RecvDoneSeq (DINT)  (completed)
      !   %MB153 : RecvStatus  (BYTE)  bit0=Busy bit1=Error
      !   %MB154 : RecvErrCode (BYTE)
      !   %MB155 : RecvExecCmd (BYTE)  low 8-bit of cmd code

      CONST num CMD_NONE := 0;
      CONST num CMD_JOINT_ABSJ := 2;
      CONST num CMD_EXTAX_ABSJ := 34;
      CONST num CMD_COMPOSITE := 120;

      PERS num plc_cmd_code := 120;
      PERS num plc_cmd_seq := 1;
      PERS num plc_ack_seq := 1;
      PERS num plc_done_seq := 1;
      PERS num plc_exec_seq := 1;
      PERS num plc_busy := 0;
      PERS num plc_err_code := 0;
      PERS num plc_autodone := 0;   ! 1=???:???Done;0=???????Done

      ! Values received from PLC "Send*" (i.e. PC->PLC->Robot forwarding), scaled to real by /1000
      PERS num plc_send_pos_x:=0;
      PERS num plc_send_pos_y:=100;
      PERS num plc_send_pos_z:=500;
      PERS num plc_send_pos_q1:=0;
      PERS num plc_send_pos_q2:=0.259;
      PERS num plc_send_pos_q3:=-0.966;
      PERS num plc_send_pos_q4:=0;
      PERS num plc_send_ext_inc:=0;
      PERS num plc_send_ext_spin:=0.001;
      PERS num plc_send_ext_gr:=6.002;
      PERS num plc_send_speed_vtcp:=200;
      PERS num plc_send_speed_vori:=200;
      PERS num plc_send_speed_vleax:=200;
      PERS num plc_send_speed_vreax:=100;

      FUNC num getdata(\switch Float\switch DInt\switch Int\switch SInt,VAR signalgi ginput)
            ! Receive data and convert
            ! If the data to be converted exceeds the limit of type num, see Table 1-3, it is recommended to set the return value type to dnum
            ! Int : signed 2-byte integer
            ! DInt: signed 4-byte integer
            ! Float: 4-byte real number
            ! SInt : signed 1-byte integer
            VAR num result;
            ! If the data to be converted exceeds the limit of type num, it is recommended to set result as dnum
            VAR rawbytes rawbyte1;
            ClearRawBytes rawbyte1;
            PackRawBytes GInputDnum(ginput),rawbyte1,1\IntX:=UDINT;
            ! Pack the received data into the binary data buffer rawbyte1,
            ! Store in 4 consecutive bytes starting from the first byte of rawbyte1

            IF Present(Float) THEN
                  ! If the optional parameter Float is selected
                  UnpackRawBytes rawbyte1,1,result\Float4;
                  ! Convert the data stored in rawbyte1 to Float type and store it in the num variable result
            ENDIF

            IF Present(Int) THEN
                  ! If the optional parameter Int is selected
                  UnpackRawBytes rawbyte1,1,result\IntX:=-2;
                  ! Convert the data stored in rawbyte1 to INT type and store it in the num variable result
            ENDIF

            IF Present(DInt) THEN
                  ! If the optional parameter DInt is selected
                  UnpackRawBytes rawbyte1,1,result\IntX:=-4;
                  ! Convert the data stored in rawbyte1 to DINT type and store it in the num variable result
            ENDIF

            IF Present(SInt) THEN
                  ! If the optional parameter SInt is selected
                  UnpackRawBytes rawbyte1,1,result\IntX:=-1;
                  ! Convert the data stored in rawbyte1 to SINT type and store it in the num variable result
            ENDIF
            RETURN result;
      ENDFUNC

      FUNC num bytes4_to_dint(num b1,num b2,num b3,num b4)
            ! b1..b4 are 0..255, where b1 is MSB and b4 is LSB (Siemens MD byte order)
            VAR num u;
            u := b1*16777216 + b2*65536 + b3*256 + b4;
            IF u > 2147483647 THEN
                  u := u - 4294967296;
            ENDIF
            RETURN u;
      ENDFUNC

      FUNC num get_dint_from_bytes(VAR signalgi gi_b1,VAR signalgi gi_b2,VAR signalgi gi_b3,VAR signalgi gi_b4)
            VAR num b1;
            VAR num b2;
            VAR num b3;
            VAR num b4;
            ! Read 8-bit Group Inputs (0..255). For signalgi, use GInput() to avoid type-mismatch with dnum.
            b1 := GInput(gi_b1);
            b2 := GInput(gi_b2);
            b3 := GInput(gi_b3);
            b4 := GInput(gi_b4);
            RETURN bytes4_to_dint(b1,b2,b3,b4);
      ENDFUNC

      PROC plc_cmd_finish(num err)
            ! Call this from Motion/main program when the actual motion is finished.
            plc_busy := 0;
            plc_err_code := err;
            plc_done_seq := plc_ack_seq;
      ENDPROC

      PROC write_plc_cmd_status()
            VAR num status_b;
            status_b := 0;
            IF plc_busy <> 0 THEN
                  status_b := status_b + 1;
            ENDIF
            IF plc_err_code <> 0 THEN
                  status_b := status_b + 2;
            ENDIF

            ! DINT outputs
            setdata\DInt,ack_seq,plc_ack_seq;
            setdata\DInt,done_seq,plc_done_seq;

            ! BYTE outputs
            SetGO status_byte,status_b;
            SetGO err_code_byte,(plc_err_code MOD 256);
            SetGO exec_cmd_byte,(plc_cmd_code MOD 256);
      ENDPROC

      PROC plc_cmd_poll()
            ! Read cmd+seq from PLC. When seq changes, latch the whole packet and ack it.
            VAR num new_cmd;
            VAR num new_seq;

            new_cmd := getdata(\DInt,cmd_code);
            new_seq := getdata(\DInt,cmd_seq);

            ! Protect against overlap: only accept a new command when not busy.
            IF plc_busy = 0 AND new_seq <> plc_ack_seq THEN
                  ! Latch parameters first (Send* region)
                  read_plc_send_values;

                  plc_cmd_code := new_cmd;
                  plc_cmd_seq := new_seq;

                  ! Accept
                  plc_ack_seq := new_seq;
                  plc_busy := 1;
                  plc_err_code := 0;

                  ! For dry-run debugging, complete immediately.
                  IF plc_autodone <> 0 THEN
                        plc_cmd_finish 0;
                  ENDIF
            ENDIF
      ENDPROC

      PROC plc_cmd_execute_if_ready()
            ! Execute the accepted command exactly once per seq.
            VAR num err;
            TPWrite "INTER plc_cmd_execute_if_ready";
            TPWrite "plc_busy=="+NumToStr(plc_busy,0)+"    plc_exec_seq=="+NumToStr(plc_exec_seq,0)+"    plc_ack_seq=="+NumToStr(plc_ack_seq,0);
            IF plc_busy <> 0 AND plc_exec_seq <> plc_ack_seq THEN
                  plc_exec_seq := plc_ack_seq;
                  err := 0;

                  TEST plc_cmd_code
                  CASE CMD_JOINT_ABSJ:
                        exec_joint_absj_2;
                  CASE CMD_EXTAX_ABSJ:
                        exec_extax_absj_34;
                  CASE CMD_COMPOSITE:
                        TPWrite "INTER CMD_COMPOSITE";
                        exec_composite_120;
                  DEFAULT:
                        err := 10;
                  ENDTEST

                  plc_cmd_finish err;
            ENDIF
      ENDPROC

      PROC exec_composite_120()
            ! Build target+speed from latched PLC values, then run motion.
            VAR robtarget tgt;
            VAR speeddata spd;
            VAR extjoint ext;
            TPWrite "INTER exec_composite_120";
            ! extax mapping used in get_pos(): eax_a=GR, eax_b=INC, eax_c=SPIN
            ext := [plc_send_ext_gr, plc_send_ext_inc, plc_send_ext_spin, 0, 0, 0];

            tgt := [[plc_send_pos_x, plc_send_pos_y, plc_send_pos_z],
                    [plc_send_pos_q1, plc_send_pos_q2, plc_send_pos_q3, plc_send_pos_q4],
                    [0,0,1,0],
                    ext];

            spd := [plc_send_speed_vtcp, plc_send_speed_vori, plc_send_speed_vleax, plc_send_speed_vreax];

            ! Use Motion module's current zone/tool/wobj to stay consistent with existing programs.
            MoveL tgt,spd,currentZone,currentTool\WObj:=currentWobj;
      ENDPROC

      PROC exec_joint_absj_2()
            ! Joint MoveAbsJ.
            ! Mapping for cmd=2:
            !   plc_send_pos_x/y/z/q1/q2/q3 => rax_1..rax_6 (degrees)
            ! External axes are kept as CURRENT (avoid unintended ext motion when UI only sets joints).
            VAR jointtarget jt;
            VAR speeddata spd;
            VAR jointtarget curjt;

            curjt := CJointT();
            jt := [[plc_send_pos_x, plc_send_pos_y, plc_send_pos_z, plc_send_pos_q1, plc_send_pos_q2, plc_send_pos_q3], curjt.extax];
            spd := [plc_send_speed_vtcp, plc_send_speed_vori, plc_send_speed_vleax, plc_send_speed_vreax];

            MoveAbsJ jt,spd,currentZone,currentTool\WObj:=currentWobj;
      ENDPROC

      PROC exec_extax_absj_34()
            ! External axis MoveAbsJ.
            ! Mapping for cmd=34:
            !   plc_send_ext_gr/inc/spin => extax.eax_a/eax_b/eax_c
            ! Robot axes are kept as CURRENT.
            VAR jointtarget jt;
            VAR speeddata spd;
            VAR jointtarget curjt;
            VAR extjoint ext;

            curjt := CJointT();
            jt := curjt;

            ! extax mapping used in get_pos(): eax_a=GR, eax_b=INC, eax_c=SPIN
            ext := [plc_send_ext_gr, plc_send_ext_inc, plc_send_ext_spin, curjt.extax.eax_d, curjt.extax.eax_e, curjt.extax.eax_f];
            jt.extax := ext;

            spd := [plc_send_speed_vtcp, plc_send_speed_vori, plc_send_speed_vleax, plc_send_speed_vreax];
            MoveAbsJ jt,spd,currentZone,currentTool\WObj:=currentWobj;
      ENDPROC


      PROC setdata(\switch Float\switch DInt\switch Int\switch SInt,VAR signalgo goutput,num data)
            ! Send data of different types
            ! If the data to be converted exceeds the limit of type num, see Table 1-3, it is recommended that the input parameter data be of type dnum
            ! Int : signed 2-byte integer
            ! DInt : signed 4-byte integer
            ! Float: 4-byte real number
            ! SInt : signed 1-byte integer
            VAR dnum result;
            VAR rawbytes rawbyte1;
            ClearRawBytes rawbyte1;
            IF Present(Float) THEN
                  ! If the optional parameter Float is selected
                  PackRawBytes data,rawbyte1,1\Float4;
                  ! Pack the data into rawbyte1 in Float4 format,
                  UnpackRawBytes rawbyte1,1,result\IntX:=UDINT;
                  ! Convert the data in rawbyte1 to UDINT format and store it in result
            ENDIF

            IF Present(Int) THEN
                  ! If the optional parameter Int is selected
                  PackRawBytes data,rawbyte1,1\IntX:=-2;
                  ! Pack the data into rawbyte1 in INT format
                  UnpackRawBytes rawbyte1,1,result\IntX:=UDINT;
                  ! Convert the data in rawbyte1 to UDINT format and store it in result
            ENDIF

            IF Present(DInt) THEN
                  !                  TPWrite "inter present dint===data:"+NumToStr(data,0);
                  ! If the optional parameter DInt is selected
                  PackRawBytes data,rawbyte1,1\IntX:=-4;
                  ! Pack the data into rawbyte1 in DINT format
                  UnpackRawBytes rawbyte1,1,result\IntX:=UDINT;
                  ! Convert the data in rawbyte1 to UDINT format and store it in result
            ENDIF

            IF Present(SInt) THEN
                  ! If the optional parameter SInt is selected
                  PackRawBytes data,rawbyte1,1\IntX:=-1;
                  UnpackRawBytes rawbyte1,1,result\IntX:=UDINT;
            ENDIF

            SetGO goutput,result;
            ! Assign the converted binary number to the group output signal goutput
      ENDPROC

      PROC read_plc_send_values()
          ! PLC side: Send* are DWord at MD163..MD215
          ! Robot side: these are mapped as 4?bit group inputs (see csv: recv_*)
          VAR num dint_v;
          
          dint_v := getdata(\DInt,recv_pos_x);
          plc_send_pos_x := dint_v/1000;
!          TPWrite "pos_x: dint_v="+NumToStr(dint_v,0)+" plc_send_pos_x="+NumToStr(plc_send_pos_x,3);
          
          dint_v := getdata(\DInt,recv_pos_y);
          plc_send_pos_y := dint_v/1000;
!          TPWrite "pos_y: dint_v="+NumToStr(dint_v,0)+" plc_send_pos_y="+NumToStr(plc_send_pos_y,3);
          
          dint_v := getdata(\DInt,recv_pos_z);
          plc_send_pos_z := dint_v/1000;
!          TPWrite "pos_z: dint_v="+NumToStr(dint_v,0)+" plc_send_pos_z="+NumToStr(plc_send_pos_z,3);
          
          dint_v := getdata(\DInt,recv_pos_q1);
          plc_send_pos_q1 := dint_v/1000;
!          TPWrite "pos_q1: dint_v="+NumToStr(dint_v,0)+" plc_send_pos_q1="+NumToStr(plc_send_pos_q1,3);
          
          dint_v := getdata(\DInt,recv_pos_q2);
          plc_send_pos_q2 := dint_v/1000;
!          TPWrite "pos_q2: dint_v="+NumToStr(dint_v,0)+" plc_send_pos_q2="+NumToStr(plc_send_pos_q2,3);
          
          dint_v := getdata(\DInt,recv_pos_q3);
          plc_send_pos_q3 := dint_v/1000;
!          TPWrite "pos_q3: dint_v="+NumToStr(dint_v,0)+" plc_send_pos_q3="+NumToStr(plc_send_pos_q3,3);
          
          dint_v := getdata(\DInt,recv_pos_q4);
          plc_send_pos_q4 := dint_v/1000;
!          TPWrite "pos_q4: dint_v="+NumToStr(dint_v,0)+" plc_send_pos_q4="+NumToStr(plc_send_pos_q4,3);
          
          dint_v := getdata(\DInt,recv_ext_inc);
          plc_send_ext_inc := dint_v/1000;
!          TPWrite "ext_inc: dint_v="+NumToStr(dint_v,0)+" plc_send_ext_inc="+NumToStr(plc_send_ext_inc,3);
          
          dint_v := getdata(\DInt,recv_ext_spin);
          plc_send_ext_spin := dint_v/1000;
!          TPWrite "ext_spin: dint_v="+NumToStr(dint_v,0)+" plc_send_ext_spin="+NumToStr(plc_send_ext_spin,3);
          
          dint_v := getdata(\DInt,recv_ext_gr);
          plc_send_ext_gr := dint_v/1000;
!          TPWrite "ext_gr: dint_v="+NumToStr(dint_v,0)+" plc_send_ext_gr="+NumToStr(plc_send_ext_gr,3);
          
          dint_v := getdata(\DInt,recv_speed_vtcp);
          plc_send_speed_vtcp := dint_v/1000;
!          TPWrite "speed_vtcp: dint_v="+NumToStr(dint_v,0)+" plc_send_speed_vtcp="+NumToStr(plc_send_speed_vtcp,3);
          
          dint_v := getdata(\DInt,recv_speed_vori);
          plc_send_speed_vori := dint_v/1000;
!          TPWrite "speed_vori: dint_v="+NumToStr(dint_v,0)+" plc_send_speed_vori="+NumToStr(plc_send_speed_vori,3);
          
          dint_v := getdata(\DInt,recv_speed_vleax);
          plc_send_speed_vleax := dint_v/1000;
!          TPWrite "speed_vleax: dint_v="+NumToStr(dint_v,0)+" plc_send_speed_vleax="+NumToStr(plc_send_speed_vleax,3);
          
          dint_v := getdata(\DInt,recv_speed_vreax);
          plc_send_speed_vreax := dint_v/1000;
!          TPWrite "speed_vreax: dint_v="+NumToStr(dint_v,0)+" plc_send_speed_vreax="+NumToStr(plc_send_speed_vreax,3);
!            WaitTime 1;
      ENDPROC

      PROC ClearPos()
            setdata\DInt,g_pos_x,0;
            setdata\DInt,g_pos_y,0;
            setdata\DInt,g_pos_z,0;
            setdata\DInt,g_pos_q1,0;
            setdata\DInt,g_pos_q2,0;
            setdata\DInt,g_pos_q3,0;
            setdata\DInt,g_pos_q4,0;

            setdata\DInt,send_ext_inc,0;
            setdata\DInt,send_ext_spin,0;
            setdata\DInt,send_ext_gr,0;
      ENDPROC

      PROC get_pos()
            !Get Cartesian Coordinates (with current tool and workobject)
            ptmp:=CRobT(\Tool:=currentTool\WObj:=currentWobj);
!            TPWrite "Round(1000*ptmp.trans.x):"+NumToStr(Round(1000*ptmp.trans.x),0);
!            TPWrite "Round(1000*ptmp.trans.y):"+NumToStr(Round(1000*ptmp.trans.y),0);
!            TPWrite "Round(1000*ptmp.trans.z):"+NumToStr(Round(1000*ptmp.trans.z),0);
!            TPWrite "Round(1000*ptmp.trans.q1):"+NumToStr(Round(1000*ptmp.rot.q1),0);
!            TPWrite "Round(1000*ptmp.trans.q2):"+NumToStr(Round(1000*ptmp.rot.q2),0);
!            TPWrite "Round(1000*ptmp.trans.q3):"+NumToStr(Round(1000*ptmp.rot.q3),0);
!            TPWrite "Round(1000*ptmp.trans.q4):"+NumToStr(Round(1000*ptmp.rot.q4),0);
!            TPWrite " ";
            setdata\DInt,g_pos_x,Round(1000*ptmp.trans.x);
!            SetGO GO6_TEST,1000;
            SetGO GO6_TEST,100;
!            setdata\DINT,g_pos_x,Round(1000*ptmp.trans.x);
            setdata\DInt,g_pos_y,Round(1000*ptmp.trans.y);
            setdata\DInt,g_pos_z,Round(1000*ptmp.trans.z);
            setdata\DInt,g_pos_q1,Round(1000*ptmp.rot.q1);
            setdata\DInt,g_pos_q2,Round(1000*ptmp.rot.q2);
            setdata\DInt,g_pos_q3,Round(1000*ptmp.rot.q3);
            setdata\DInt,g_pos_q4,Round(1000*ptmp.rot.q4);

            ! External axes: last bracket of robtarget (ptmp.extax) first three values
            ! According to your note: [GR, INC, SPIN, ...]
            setdata\DInt,send_ext_gr,Round(1000*ptmp.extax.eax_a);
            setdata\DInt,send_ext_inc,Round(1000*ptmp.extax.eax_b);
            setdata\DInt,send_ext_spin,Round(1000*ptmp.extax.eax_c);

            !            addString:=addString+NumToStr(ptmp.trans.y,2)+" ";
            !            addString:=addString+NumToStr(ptmp.trans.z,2)+" ";
            !            addString:=addString+NumToStr(ptmp.rot.q1,3)+" ";
            !            addString:=addString+NumToStr(ptmp.rot.q2,3)+" ";
            !            addString:=addString+NumToStr(ptmp.rot.q3,3)+" ";
            !            addString:=addString+NumToStr(ptmp.rot.q4,3);


      ENDPROC

      PROC main_plc()
            TPWrite "Clear POS";
            ClearPos;
            WaitTime(2);
            TPWrite "robot_task_4";
            ! ????????:??? Ack/Done ????????
            plc_autodone := 0;
            plc_exec_seq := 0;
            plc_busy := 0;
            plc_err_code := 0;
            WHILE TRUE DO
                  plc_step;
                  WaitTime (0.1);
            ENDWHILE

      ENDPROC

      PROC plc_step()
            ! One iteration of PLC polling + optional execution + status/pose writeback.
            ! Safety recovery: if BUSY is latched but there is no pending seq
            ! (exec == ack), we are effectively idle. This can happen if PERS
            ! variables were left at 1 on a previous run / module reload.
            IF plc_busy <> 0 AND plc_exec_seq = plc_ack_seq THEN
                  plc_busy := 0;
            ENDIF
            get_pos;
            plc_cmd_poll;
            plc_cmd_execute_if_ready;
            write_plc_cmd_status;
      ENDPROC
ENDMODULE