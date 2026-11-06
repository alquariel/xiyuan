MODULE Motion

      !////////////////
      !GLOBAL VARIABLES
      !////////////////

      !//Robot configuration
      !PERS tooldata currentTool := [TRUE,[[338.402,18.0101,-541.55],[0.997552,0.00335155,0.000006229,-0.0698543]],[0.001,[0,0,0.001],[1,0,0,0],0,0,0]];
      !PERS tooldata currentTool:=[TRUE,[[-0.55,0,490.325],[0.707107,0,0,-0.707107]],[0.001,[0,0,0.001],[1,0,0,0],0,0,0]];
      !PERS tooldata currentTool:= [TRUE,[[-1.99776,1.78086,494.406],[0.55,0.002,-0.00057903,0.83516]],[35,[0,0,0.001],[1,0,0,0],0,0,0]];
      PERS tooldata currentTool:=[TRUE,[[-1.99776,1.78086,494.406],[0.707107,0,0,0.707107]],[35,[0,0,0.001],[1,0,0,0],0,0,0]];
!      PERS tooldata currentTool:=[TRUE,[[-1.99776,1.78086,494.406],[0.501036,0.00159329,-0.000922433,0.865424]],[39.5,[127.7,-195.9,60.5],[1,0,0,0],6.486,4.012,2.837]];
      !PERS wobjdata currentWobj := [FALSE,TRUE,"",[[734.201,20.8803,828.431],[0.713319,0.00336855,-0.00319022,-0.700824]],[[0,0,0],[1,0,0,0]]];
      !currentWobj = hhn1
      !PERS wobjdata currentWobj:=[FALSE,TRUE,"",[[1605.59,650.555,-284.466],[0.708248,0.000678772,-0.00170833,-0.705961]],[[0,0,0],[1,0,0,0]]];
      !Manual correction
      !PERS wobjdata currentWobj:=[FALSE,TRUE,"",[[1605.59,650.555,-180],[0.708248,0.000678772,-0.00170833,-0.705961]],[[0,0,0],[1,0,0,0]]];
!      PERS wobjdata currentWobj:=[FALSE,TRUE,"STN",[[1600,0,-180],[0.707107,0,0,-0.707107]],[[0,0,0],[1,0,0,0]]];
      PERS wobjdata currentWobj:=[FALSE,TRUE,"",[[1606.75,-21.2442,-285.415],[0.706764,0.00126541,-0.00379744,-0.707438]],[[0,0,0],[1,0,0,0]]];
      !PERS tooldata xyk0507 := [TRUE,[[-1.99776,1.78086,494.406],[0.55,0.002,-0.00057903,0.83516]],[35,[0,0,0.001],[1,0,0,0],0,0,0]];
      !PERS tooldata xykTool:=[TRUE,[[-0.55,0,490.325],[0.707106781,0,0,-0.707106781]],[0.001,[0,0,0.001],[1,0,0,0],0,0,0]];
      !PERS wobjdata xykWobj := [FALSE,TRUE,"STN",[[1600,0,-180],[0.707106781,0,0,-0.707106781]],[[0,0,0],[1,0,0,0]]];


      PERS speeddata currentSpeed;
      PERS zonedata currentZone;

      !// Clock Synchronization
      PERS bool startLog:=TRUE;
      PERS bool startRob:=TRUE;

      !// Mutex between logger and changing the tool and work objects
      PERS bool frameMutex:=FALSE;

      !// Transport selection (Socket vs PLC forwarding)
      !// TRUE  -> run PLC_SERVER\plc_step loop (PC->PLC->Robot)
      !// FALSE -> keep legacy socket server loop
      PERS bool use_plc_forwarding:=TRUE;

      !//PC communication
      VAR socketdev clientSocket_motion;
      VAR socketdev serverSocket_motion;
      VAR num instructionCode;
      CONST num MAX_PARAMS:=1024;
      VAR num params{MAX_PARAMS};
      ! aline_datapackage,ori,params{10}
      VAR num nParams;
      ! pc byte receive
      VAR socketdev sockte2;
      VAR byte receiveByte{1024};
      VAR string sendStr;
      VAR num receiveByteNum:=1024;
      VAR string str_Part{1024};
      VAR string str_Var:="";
      VAR string Data{1024};
      VAR num j;
      VAR string tempStr:="";

      !PERS string ipController:= "192.168.8.100"; !robot default IP
      PERS string ipController:="192.168.0.104";
      !local IP for testing in simulation
      PERS num serverPort:=5010;

      !//Motion of the robot
      VAR robtarget cartesianTarget;
      VAR jointtarget jointsTarget;
      VAR bool moveCompleted;
      !Set to true after finishing a Move instruction.

      !//Buffered move variables
      CONST num MAX_BUFFER:=20000;
      VAR num BUFFER_POS:=0;
      VAR robtarget bufferTargets{MAX_BUFFER};
      VAR speeddata bufferSpeeds{MAX_BUFFER};
      ! new plc
      VAR num plc_motor_speed:=0;
      VAR num plc_cutter:=0;
      VAR num plc_push:=0;
      VAR num plc_temp:=0;
      VAR num plc_mb143_byte:=0;
      PERS bool plc_laser_state:=TRUE;
      VAR num bufferPLC_motor{MAX_BUFFER};
      VAR num bufferPLC_cutter{MAX_BUFFER};
      VAR num bufferPLC_push{MAX_BUFFER};
      VAR num bufferPLC_temp{MAX_BUFFER};

      !//External axis position variables
      VAR extjoint externalAxis;

      !//Circular move buffer
      VAR robtarget circPoint;

      !//Correct Instruction Execution and possible return values
      VAR num ok;
      CONST num SERVER_BAD_MSG:=0;
      CONST num SERVER_OK:=1;


      CONST robtarget p10:=[[851.36,-636.03,885.80],[0.00705459,-0.132877,0.990988,-0.0153915],[-1,-1,-1,0],[4732.84,-10.9257,1470.82,9E+09,9E+09,9E+09]];
      CONST robtarget p20:=[[901.50,-693.70,885.78],[0.0070311,-0.132883,0.990988,-0.0153711],[-1,-1,-1,0],[4732.84,-10.9262,1470.82,9E+09,9E+09,9E+09]];
      CONST robtarget p30:=[[817.79,-706.95,909.14],[0.00705406,-0.132848,0.990992,-0.0153789],[-1,-1,-1,0],[4732.94,-10.9213,1470.82,9E+09,9E+09,9E+09]];
      CONST robtarget p40:=[[670.5612,-112.4481,451.6474],[0.1908088,-1.354618E-08,0.9816272,9.520837E-09],[-1,0,-1,0],[9E+09,9E+09,9E+09,9E+09,9E+09,9E+09]];
      TASK PERS tooldata tool1:=[TRUE,[[0,0,0],[1,0,0,0]],[-1,[0,0,0],[1,0,0,0],0,0,0]];
      TASK PERS wobjdata wobj1:=[FALSE,TRUE,"",[[0,0,0],[1,0,0,0]],[[0,0,0],[1,0,0,0]]];

      CONST robtarget p80:=[[863.14,-780.05,909.09],[0.00703553,-0.132801,0.990999,-0.0153575],[-1,-1,-1,0],[4732.94,-10.9205,1470.82,9E+09,9E+09,9E+09]];
      CONST robtarget p90:=[[821.52,-782.06,909.14],[0.00704569,-0.132804,0.990998,-0.0153617],[-1,-1,-1,0],[4732.94,-10.9213,1470.82,9E+09,9E+09,9E+09]];
      CONST robtarget p100:=[[860.89,-704.56,909.13],[0.00706562,-0.132799,0.990998,-0.0153811],[-1,-1,-1,0],[4732.94,-10.9205,1470.82,9E+09,9E+09,9E+09]];
      CONST robtarget p110:=[[860.89,-704.56,909.13],[0.00706776,-0.132798,0.990999,-0.0153827],[-1,-1,-1,0],[4732.93,-10.9205,1470.82,9E+09,9E+09,9E+09]];
      CONST robtarget p120:=[[-654.183,484.988,130.113],[0.0647012,0.704141,-0.70414,0.0647022],[0,0,1,0],[0,0,0,0,9E+09,9E+09]];


      !PERS wobjdata xykWobj_2:=[FALSE,TRUE,"STN",[[1600,0,-180],[0.707106781,0,0,-0.707106781]],[[0,0,0],[0.707106781,0,0,0.707106781]]];
      !PERS tooldata xykTool_2:=[TRUE,[[-0.549999548,-0.000000278,490.324999827],[1,0,0,0]],[0.001,[0,0,0.001],[1,0,0,0],0,0,0]];
      !PERS tooldata xykTool_one:=[TRUE,[[-0.55,0,490.325],[0.707106781,0,0,-0.707106781]],[0.001,[0,0,0.001],[1,0,0,0],0,0,0]];
      !PERS wobjdata xykWobj_one:=[FALSE,TRUE,"STN",[[1600,0,-180],[0.707106781,0,0,-0.707106781]],[[0,0,0],[1,0,0,0]]];
      !PERS tooldata xykTool_1:=[TRUE,[[-0.549999548,-0.000000278,490.324999827],[1,0,0,0]],[0.001,[0,0,0.001],[1,0,0,0],0,0,0]];
      !PERS wobjdata xykWobj_1:=[FALSE,TRUE,"STN",[[1600,0,-180],[0.707106781,0,0,-0.707106781]],[[0,0,0],[0.707106781,0,0,0.707106781]]];
      !PERS wobjdata x_1m2_1m2:=[FALSE,TRUE,"",[[1606.28,-115.459,-189.589],[0.706983,0.00169797,-0.00369724,-0.707218]],[[0,0,0],[1,0,0,0]]];
      !PERS tooldata lqf0108:=[TRUE,[[-1.99776,1.78086,494.406],[0.55,0.002,-0.00057903,0.95]],[0.001,[0,0,0.001],[1,0,0,0],0,0,0]];
      !PERS wobjdata hhn:=[FALSE,TRUE,"",[[1595.56,1296.36,245.683],[0.70802,0.00537514,-0.00521913,-0.706153]],[[0,0,0],[1,0,0,0]]];
      !PERS wobjdata hhn1:=[FALSE,TRUE,"",[[1605.59,650.555,-284.466],[0.708248,0.000678772,-0.00170833,-0.705961]],[[0,0,0],[1,0,0,0]]];




      PERS bool cmdReady;
      PERS num cmdType;
      PERS string cmdPayload;
      PERS bool cmdSent;
      PERS bool cmdVerified;
      PERS bool cmdWhileFlag;
      PERS bool BeginProgramFlag;

      ! upload mod file name
      VAR string mod_filename:="";
      ! upload whether success
      VAR bool load_flag:=FALSE;
      VAR num colonPos;
      VAR num load_mod_message_flag;
      ! ============================================================
      ! ??????(cmdType?)
      ! ============================================================
      CONST num CMD_BEGIN_PROGRAM:=1;
      CONST num CMD_MATERIAL:=2;
      CONST num CMD_NOMINAL_WIDTH:=3;
      CONST num CMD_LAYING_SPEED:=4;
      CONST num CMD_EXTERNAL_AXIS:=5;
      CONST num CMD_TOW_TENSION:=6;
      CONST num CMD_MOVEL_TO:=7;
      CONST num CMD_MOVEL:=8;
      CONST num CMD_PROGRAM_ID:=9;
      CONST num CMD_NUMBER_OF_TOWS:=10;
      CONST num CMD_WATER_COOLING_TEMP:=11;
      CONST num CMD_LAYER_THICKNESS:=12;
      CONST num CMD_FIBER_ANGLE:=13;
      CONST num CMD_LAYER_NUMBER:=14;
      CONST num CMD_BEGIN_LAYER:=15;
      CONST num CMD_LAYER_ID:=16;
      CONST num CMD_BEGIN_TRACE:=17;
      CONST num CMD_TRACE_ID:=18;
      CONST num CMD_COMPACTION_PRESSURE:=19;
      CONST num CMD_LASER_EMISSION:=20;
      CONST num CMD_LASER_POWER:=21;
      CONST num CMD_CUTTER_STATE:=22;
      CONST num CMD_COMPACTION_ROLLER_STATE:=23;
      CONST num CMD_PINCH_ROLLER_STATE:=24;
      CONST num CMD_HEAD_ANGLE:=25;
      CONST num CMD_FEED_LENGTH:=26;
      CONST num CMD_FEED_SPEED:=27;
      CONST num CMD_PLC_FEED_SPEED:=28;
      CONST num CMD_POST_FEED_SPEED:=29;
      CONST num CMD_END_TRACE:=30;
      CONST num CMD_END_LAYER:=31;
      CONST num CMD_END_PROGRAM:=32;

      
            ! ============================================================
      ! PLC output helpers (Robot -> PLC)
      ! ============================================================
      ! MB143 bit mapping (keep within existing 8-bit output, no new signals):
      !   bit0 (1)   : motor_execute pulse        -> old DB5.DBX40.0
      !   bit1 (2)   : laser_power_confirm pulse  -> old DB5.DBX4.1
      !   bit2 (4)   : laser_on pulse             -> old DB5.DBX14.2
      !   bit3 (8)   : laser_off pulse            -> old DB5.DBX14.3
      !   bit4 (16)  : motor_state bit0           -> old DB5.DBW38 (state=3)
      !   bit5 (32)  : cutter_cmd                 -> old DB5.DBX14.5
      !   bit6 (64)  : motor_state bit1
      !   bit7 (128) : push_cmd (pinch roller)    -> old DB5.DBX14.7
      CONST num PLC_BIT_MOTOR_EXEC:=1;
      CONST num PLC_BIT_LASER_CONFIRM:=2;
      CONST num PLC_BIT_LASER_ON:=4;
      CONST num PLC_BIT_LASER_OFF:=8;
      CONST num PLC_BIT_MOTOR_STATE0:=16;
      CONST num PLC_BIT_MOTOR_STATE1:=32;
      CONST num PLC_BIT_CUTTER:=64;
      CONST num PLC_BIT_PUSH:=128;
      
      
      !////////////////
      !LOCAL METHODS
      !////////////////

      !//Method to parse the message received from a PC
      !// If correct message, loads values on:
      !// - instructionCode.
      !// - nParams: Number of received parameters.
      !// - params{nParams}: Vector of received params.


      !PROC ParseMsg(string msg)
      !    !//Local variables
      !    VAR bool auxOk;
      !    VAR num ind:=1;
      !    VAR num newInd;
      !    VAR num length;
      !    VAR num indParam:=1;
      !    VAR string subString;
      !    VAR bool end := FALSE;

      !    !//Find the end character
      !    length := StrMatch(msg,1,"#");
      !    IF length > StrLen(msg) THEN
      !        !//Corrupt message
      !        nParams := -1;
      !    ELSE
      !        !//Read Instruction code
      !        newInd := StrMatch(msg,ind," ") + 1;
      !        subString := StrPart(msg,ind,newInd - ind - 1);
      !        auxOk:= StrToVal(subString, instructionCode);
      !        IF auxOk = FALSE THEN
      !            !//Impossible to read instruction code
      !            nParams := -1;
      !        ELSE
      !            ind := newInd;
      !            !//Read all instruction parameters (maximum of 8)
      !            WHILE end = FALSE DO
      !                newInd := StrMatch(msg,ind," ") + 1;
      !                IF newInd > length THEN
      !                    end := TRUE;
      !                ELSE
      !                    subString := StrPart(msg,ind,newInd - ind - 1);
      !                    auxOk := StrToVal(subString, params{indParam});
      !                    indParam := indParam + 1;
      !                    ind := newInd;
      !                ENDIF
      !            ENDWHILE
      !            nParams:= indParam - 1;
      !        ENDIF
      !    ENDIF
      !ENDPROC

      !//Method to parse the message received from a PC
      PROC ParseMsg(string msg)
            !//Local variables
            VAR bool auxOk;
            VAR num ind:=1;
            VAR num newInd;
            VAR num length;
            VAR num indParam:=1;
            VAR string subString;
            VAR bool end:=FALSE;

            !//??????????
            !    TPWrite "or_data: " + msg;

            !//Find the end character
            length:=StrMatch(msg,1,"#");
            IF length>StrLen(msg) THEN
                  !//Corrupt message
                  nParams:=-1;
                  TPWrite "mistake: format incorrect, lack of termination character:#";
            ELSE
                  !//Read Instruction code
                  newInd:=StrMatch(msg,ind," ")+1;
                  subString:=StrPart(msg,ind,newInd-ind-1);
                  auxOk:=StrToVal(subString,instructionCode);
                  IF auxOk=FALSE THEN
                        !//Impossible to read instruction code
                        nParams:=-1;
                        TPWrite "mistake:don't read instruction code";
                  ELSE
                        !            TPWrite "instruction code: " + NumToStr(instructionCode,0);
                        ind:=newInd;
                        !//Read all instruction parameters (maximum of 8)
                        WHILE end=FALSE DO
                              newInd:=StrMatch(msg,ind," ")+1;
                              IF newInd>length THEN
                                    end:=TRUE;
                              ELSE
                                    subString:=StrPart(msg,ind,newInd-ind-1);
                                    auxOk:=StrToVal(subString,params{indParam});
                                    !//??????
                                    !                    TPWrite "chanshu" + NumToStr(indParam,0) + ": " + subString;
                                    indParam:=indParam+1;
                                    ind:=newInd;
                              ENDIF
                        ENDWHILE
                        nParams:=indParam-1;
                        !            TPWrite "Total number of parameters: " + NumToStr(nParams,0);
                  ENDIF
            ENDIF
      ENDPROC


      PROC SendPLCParamsToPC(num motor,num cutter,num push,num temp)
            VAR string msg;
            msg:="PLC "+NumToStr(motor,2)+" "+NumToStr(cutter,2)+" "+NumToStr(push,2)+" "+NumToStr(temp,2)+" A";
            SocketSend clientSocket_motion\Str:=msg;
      ENDPROC



      PROC SafePrint(string prefix,string msg)
            VAR num len;
            VAR num pos;
            VAR string chunk;

            len:=StrLen(msg);
            TPWrite prefix+" (total length:"+NumToStr(len,0)+")";

            IF len<=70 THEN
                  TPWrite msg;
            ELSE
                  pos:=1;
                  WHILE pos<len DO
                        IF pos+69<=len THEN
                              chunk:=StrPart(msg,pos,70);
                        ELSE
                              chunk:=StrPart(msg,pos,len-pos+1);
                        ENDIF
                        TPWrite "  ["+NumToStr(pos,0)+"]: "+chunk;
                        pos:=pos+70;
                  ENDWHILE
            ENDIF
      ENDPROC



      !//Handshake between server and client:
      !// - Creates socket.
      !// - Waits for incoming TCP connection.
      PROC ServerCreateAndConnect_1(string ip,num port)
            VAR string clientIP;

            SocketCreate serverSocket_motion;
            SocketBind serverSocket_motion,ip,port;
            SocketListen serverSocket_motion;
            TPWrite "SERVER: Server waiting for incoming connections ...";
            WHILE SocketGetStatus(clientSocket_motion)<>SOCKET_CONNECTED DO
                  SocketAccept serverSocket_motion,clientSocket_motion\ClientAddress:=clientIP\Time:=WAIT_MAX;
                  IF SocketGetStatus(clientSocket_motion)<>SOCKET_CONNECTED THEN
                        TPWrite "SERVER: Problem serving an incoming connection.";
                        TPWrite "SERVER: Try reconnecting.";
                  ENDIF
                  !//Wait 0.5 seconds for the next reconnection
                  WaitTime 0.5;
            ENDWHILE
            TPWrite "SERVER: Connected to IP "+clientIP;
      ENDPROC


      !//Parameter initialization
      !// Loads default values for
      !// - Tool.
      !// - WorkObject.
      !// - Zone.
      !// - Speed.
      !PROC Initialize()
      !currentTool := [TRUE,[[0,0,0],[1,0,0,0]],[0.001,[0,0,0.001],[1,0,0,0],0,0,0]];
      !currentWobj := [FALSE,TRUE,"",[[0,0,0],[1,0,0,0]],[[0,0,0],[1,0,0,0]]];
      !currentSpeed := [100, 50, 0, 0];
      !currentZone := [FALSE, 0.3, 0.3,0.3,0.03,0.3,0.03]; !z0

      !Find the current external axis values so they don't move when we start
      !jointsTarget := CJointT();
      !externalAxis := jointsTarget.extax;
      !ENDPROC



      ! ============================================================
      ! BEGIN_PROGRAM
      ! ============================================================
      PROC BEGIN_PROGRAM()
            WHILE NOT cmdWhileFlag DO
                  !                  TPWrite "Wait PROGRAM_ID return";
            ENDWHILE
            cmdVerified:=FALSE;
            !            TPWrite "set begin_program before cmdVerified:"\Bool:=cmdVerified;
            !            TPWrite "BEGIN_PROGRAM before cmdVerified:"\Bool:=cmdVerified;
            !            TPWrite "BEGIN_PROGRAM before cmdReady:"\Bool:=cmdReady;
            !            TPWrite "BEGIN_PROGRAM before cmdType:"\Num:=cmdType;
            cmdType:=CMD_BEGIN_PROGRAM;
            cmdPayload:="";
            cmdReady:=TRUE;
            WHILE NOT cmdVerified DO
                  !                  TPWrite "Wait BEGIN_PROGERAM return";
            ENDWHILE
            !            TPWrite "Execute BEGIN_PROGRAM WaitUntil";
            cmdType:=0;
            !            TPWrite "BEGIN_PROGRAM cmdVerified:"\Bool:=cmdVerified;
            !            TPWrite "BEGIN_PROGRAM cmdReady:"\Bool:=cmdReady;
            !            TPWrite "BEGIN_PROGRAM cmdType:"\Num:=cmdType;

      ENDPROC

      ! ============================================================
      ! ??ID
      ! ============================================================
      PROC PROGRAM_ID(string id)
            WHILE NOT cmdWhileFlag DO
                  !                  TPWrite "Wait PROGRAM_ID return";
            ENDWHILE
            cmdVerified:=FALSE;
            !            TPWrite "set program_id before cmdVerified:"\Bool:=cmdVerified;
            cmdType:=CMD_PROGRAM_ID;
            cmdPayload:=id;
            cmdReady:=TRUE;
            !        WaitUntil cmdVerified = TRUE;
            !            TPWrite "PROGRAM_ID before cmdVerified:"\Bool:=cmdVerified;
            WHILE NOT cmdVerified DO
                  !                  TPWrite "Wait PROGRAM_ID return";
            ENDWHILE
            !            TPWrite "set program_id later cmdVerified:"\Bool:=cmdVerified;
            cmdType:=0;
            !            TPWrite "PROGRAM_ID later cmdVerified:"\Bool:=cmdVerified;
            !            TPWrite "Execute PROGRAM_ID WaitUntil";
      ENDPROC

      ! ============================================================
      ! ????
      ! ============================================================
      PROC MATERIAL(string material)
            WHILE NOT cmdWhileFlag DO
                  !                  TPWrite "Wait PROGRAM_ID return";
            ENDWHILE
            cmdVerified:=FALSE;
            cmdType:=CMD_MATERIAL;
            cmdPayload:=material;
            cmdReady:=TRUE;
            !        WaitUntil cmdVerified = TRUE;
            !            TPWrite "MATERIAL before cmdVerified:"\Bool:=cmdVerified;
            WHILE NOT cmdVerified DO
                  !                  TPWrite "Wait MATERIAL return";
            ENDWHILE
            cmdType:=0;
            !            TPWrite "MATERIAL later cmdVerified:"\Bool:=cmdVerified;
            !            !        cmdVerified:=FALSE;
            !            TPWrite "Execute MATERIAL WaitUntil";
      ENDPROC

      ! ============================================================
      ! ????
      ! ============================================================
      PROC NOMINAL_WIDTH(num width)
            WHILE NOT cmdWhileFlag DO
                  !                  TPWrite "Wait PROGRAM_ID return";
            ENDWHILE
            cmdVerified:=FALSE;
            cmdType:=CMD_NOMINAL_WIDTH;
            cmdPayload:=NumToStr(width,1);
            cmdReady:=TRUE;
            !            WaitUntil cmdVerified=TRUE;
            WHILE NOT cmdVerified DO
                  !                  TPWrite "Wait MATERIAL return";
            ENDWHILE
            cmdType:=0;
      ENDPROC

      ! ============================================================
      ! ????
      ! ============================================================
      PROC NUMBER_OF_TOWS(num tows)
            WHILE NOT cmdWhileFlag DO
                  !                  TPWrite "Wait PROGRAM_ID return";
            ENDWHILE
            cmdVerified:=FALSE;
            cmdType:=CMD_NUMBER_OF_TOWS;
            cmdPayload:=NumToStr(tows,0);
            cmdReady:=TRUE;
            !            WaitUntil cmdVerified=TRUE;
            WHILE NOT cmdVerified DO
                  !                  TPWrite "Wait MATERIAL return";
            ENDWHILE
            cmdType:=0;
      ENDPROC

      ! ============================================================
      ! ????
      ! ============================================================
      PROC WATER_COOLING_TEMP(num temp)
            WHILE NOT cmdWhileFlag DO
                  !                  TPWrite "Wait PROGRAM_ID return";
            ENDWHILE
            cmdVerified:=FALSE;
            cmdType:=CMD_WATER_COOLING_TEMP;
            cmdPayload:=NumToStr(temp,0);
            cmdReady:=TRUE;
            !            WaitUntil cmdVerified=TRUE;
            WHILE NOT cmdVerified DO
                  !                  TPWrite "Wait MATERIAL return";
            ENDWHILE
            cmdType:=0;
      ENDPROC

      ! ============================================================
      ! ???
      ! ============================================================
      PROC LAYER_Thickness(num thickness)
            WHILE NOT cmdWhileFlag DO
                  !                  TPWrite "Wait PROGRAM_ID return";
            ENDWHILE
            cmdVerified:=FALSE;
            cmdType:=CMD_LAYER_THICKNESS;
            cmdPayload:=NumToStr(thickness,2);
            cmdReady:=TRUE;
            !            WaitUntil cmdVerified=TRUE;
            WHILE NOT cmdVerified DO
                  !                  TPWrite "Wait MATERIAL return";
            ENDWHILE
            cmdType:=0;
      ENDPROC

      ! ============================================================
      ! ????(?????4???)
      ! ============================================================
      PROC FIBER_ANGLE(num a1,num a2,num a3,num a4)
            WHILE NOT cmdWhileFlag DO
                  !                  TPWrite "Wait PROGRAM_ID return";
            ENDWHILE
            cmdVerified:=FALSE;
            cmdType:=CMD_FIBER_ANGLE;
            cmdPayload:=NumToStr(a1,0)+" "+NumToStr(a2,0)+" "+NumToStr(a3,0)+" "+NumToStr(a4,0);
            cmdReady:=TRUE;
            !            WaitUntil cmdVerified=TRUE;
            WHILE NOT cmdVerified DO
                  !                  TPWrite "Wait MATERIAL return";
            ENDWHILE
            cmdType:=0;
      ENDPROC

      ! ============================================================
      ! ??
      ! ============================================================
      PROC LAYER_NUMBER(num layerNum)
            WHILE NOT cmdWhileFlag DO
                  !                  TPWrite "Wait PROGRAM_ID return";
            ENDWHILE
            cmdVerified:=FALSE;
            cmdType:=CMD_LAYER_NUMBER;
            cmdPayload:=NumToStr(layerNum,0);
            cmdReady:=TRUE;
            !            WaitUntil cmdVerified=TRUE;
            WHILE NOT cmdVerified DO
                  !                  TPWrite "Wait MATERIAL return";
            ENDWHILE
            cmdType:=0;
      ENDPROC

      ! ============================================================
      ! ????
      ! ============================================================
      PROC BEGIN_LAYER()
            WHILE NOT cmdWhileFlag DO
                  !                  TPWrite "Wait PROGRAM_ID return";
            ENDWHILE
            cmdVerified:=FALSE;
            cmdType:=CMD_BEGIN_LAYER;
            cmdPayload:="";
            cmdReady:=TRUE;
            !            WaitUntil cmdVerified=TRUE;
            WHILE NOT cmdVerified DO
                  !                  TPWrite "Wait MATERIAL return";
            ENDWHILE
            cmdType:=0;
      ENDPROC

      ! ============================================================
      ! ?ID
      ! ============================================================
      PROC LAYER_ID(num layerID)
            WHILE NOT cmdWhileFlag DO
                  !                  TPWrite "Wait PROGRAM_ID return";
            ENDWHILE
            cmdVerified:=FALSE;
            cmdType:=CMD_LAYER_ID;
            cmdPayload:=NumToStr(layerID,0);
            cmdReady:=TRUE;
            !            WaitUntil cmdVerified=TRUE;
            WHILE NOT cmdVerified DO
                  !                  TPWrite "Wait MATERIAL return";
            ENDWHILE
            cmdType:=0;
      ENDPROC

      ! ============================================================
      ! ????
      ! ============================================================
      PROC BEGIN_TRACE()
            WHILE NOT cmdWhileFlag DO
                  !                  TPWrite "Wait PROGRAM_ID return";
            ENDWHILE
            cmdVerified:=FALSE;
            cmdType:=CMD_BEGIN_TRACE;
            cmdPayload:="";
            cmdReady:=TRUE;
            !            WaitUntil cmdVerified=TRUE;
            WHILE NOT cmdVerified DO
                  !                  TPWrite "Wait MATERIAL return";
            ENDWHILE
            cmdType:=0;
      ENDPROC

      ! ============================================================
      ! ??ID
      ! ============================================================
      PROC TRACE_ID(num traceID)
            WHILE NOT cmdWhileFlag DO
                  !                  TPWrite "Wait PROGRAM_ID return";
            ENDWHILE
            cmdVerified:=FALSE;
            cmdType:=CMD_TRACE_ID;
            cmdPayload:=NumToStr(traceID,0);
            cmdReady:=TRUE;
            !            WaitUntil cmdVerified=TRUE;
            WHILE NOT cmdVerified DO
                  !                  TPWrite "Wait MATERIAL return";
            ENDWHILE
            cmdType:=0;
      ENDPROC

      ! ============================================================
      ! ????
      ! ============================================================
      PROC COMPACTION_PRESSURE(num pressure)
            WHILE NOT cmdWhileFlag DO
                  !                  TPWrite "Wait PROGRAM_ID return";
            ENDWHILE
            cmdVerified:=FALSE;
            cmdType:=CMD_COMPACTION_PRESSURE;
            cmdPayload:=NumToStr(pressure,0);
            cmdReady:=TRUE;
            !            WaitUntil cmdVerified=TRUE;
            WHILE NOT cmdVerified DO
                  !                  TPWrite "Wait MATERIAL return";
            ENDWHILE
            cmdType:=0;
      ENDPROC

      ! ============================================================
      ! ????
      ! ============================================================
      PROC LASER_EMISSION(num emission)
            WHILE NOT cmdWhileFlag DO
                  !                  TPWrite "Wait PROGRAM_ID return";
            ENDWHILE
            cmdVerified:=FALSE;
            cmdType:=CMD_LASER_EMISSION;
            cmdPayload:=NumToStr(emission,0);
            cmdReady:=TRUE;
            !            WaitUntil cmdVerified=TRUE;
            WHILE NOT cmdVerified DO
                  !                  TPWrite "Wait MATERIAL return";
            ENDWHILE
            cmdType:=0;
      ENDPROC


      PROC set_dint_bytes_local(VAR signalgo go_b1,VAR signalgo go_b2,VAR signalgo go_b3,VAR signalgo go_b4,num dint_value)
            VAR rawbytes rawbyte1;
            VAR num b1;
            VAR num b2;
            VAR num b3;
            VAR num b4;

            ClearRawBytes rawbyte1;

            ! Pack signed DINT into 4 bytes
            PackRawBytes dint_value,rawbyte1,1\IntX:=-4;

            ! PackRawBytes stores bytes LSB->MSB in rawbytes index 1..4.
            ! Siemens MD expects MSB at the lowest MB address (e.g. MD99 uses MB99 as MSB).
            ! So we reverse the output order when writing to consecutive 8-bit GOs.
            UnpackRawBytes rawbyte1,1,b1\IntX:=-1;
            UnpackRawBytes rawbyte1,2,b2\IntX:=-1;
            UnpackRawBytes rawbyte1,3,b3\IntX:=-1;
            UnpackRawBytes rawbyte1,4,b4\IntX:=-1;

            IF b1 < 0 THEN b1 := b1 + 256; ENDIF
            IF b2 < 0 THEN b2 := b2 + 256; ENDIF
            IF b3 < 0 THEN b3 := b3 + 256; ENDIF
            IF b4 < 0 THEN b4 := b4 + 256; ENDIF

            SetGO go_b1,b4;
            SetGO go_b2,b3;
            SetGO go_b3,b2;
            SetGO go_b4,b1;
      ENDPROC

      PROC write_plc_motor_speed(num speed)
            VAR num dint_v;
            dint_v := Round(speed * 1000);
            SetGO send_motor,dint_v;
      ENDPROC

      PROC write_plc_laser_power(num power)
            VAR num p;
            p := power;
            IF p < 0 THEN p := 0; ENDIF
            IF p > 100 THEN p := 100; ENDIF
            SetGO send_laser_mb144,p;
      ENDPROC

      PROC write_plc_mb143_bit(num mask,num on)
            VAR num m2;
            m2 := mask * 2;

            IF on <> 0 THEN
                  IF (plc_mb143_byte MOD m2) < mask THEN
                        plc_mb143_byte := plc_mb143_byte + mask;
                  ENDIF
            ELSE
                  IF (plc_mb143_byte MOD m2) >= mask THEN
                        plc_mb143_byte := plc_mb143_byte - mask;
                  ENDIF
            ENDIF

            SetGO send_mb143,plc_mb143_byte;
      ENDPROC

      PROC pulse_plc_mb143_bit(num mask,num pulse_s)
            ! ???"????"??:??????,??????
            ! ????/????????????? write_plc_mb143_bit(mask,0)?
            write_plc_mb143_bit mask,1;
      ENDPROC

      ! ============================================================
      ! ????
      ! ============================================================
      PROC LASER_POWER(num power)
            ! Mirror the original PC->PLC logic (DB5) using existing robot->PLC outputs.
            ! Convention kept from original code:
            !   power=0   : do nothing
            !   power=1   : close laser (off)
            !   power>1   : set/update power; if laser was off, confirm + turn on
            IF power = 0 THEN
                  RETURN;
            ENDIF

            IF power = 1 THEN
                  ! Keep laser outputs mutually exclusive and state-consistent.
                  write_plc_mb143_bit PLC_BIT_LASER_ON,0;
                  write_plc_mb143_bit PLC_BIT_LASER_CONFIRM,0;
                  write_plc_mb143_bit PLC_BIT_LASER_OFF,1;
                  plc_laser_state := FALSE;
                  RETURN;
            ENDIF

            write_plc_laser_power power;
            ! Ensure OFF is cleared when we are commanding ON.
            write_plc_mb143_bit PLC_BIT_LASER_OFF,0;

            IF plc_laser_state = FALSE THEN
                  plc_laser_state := TRUE;
            ENDIF

            ! Expose ON-state as stable level signals.
            write_plc_mb143_bit PLC_BIT_LASER_CONFIRM,1;
            write_plc_mb143_bit PLC_BIT_LASER_ON,1;
      ENDPROC

      ! ============================================================
      ! ????
      ! ============================================================
      PROC CUTTER_STATE(num state)
            ! Write to PLC (MB143 bit0)
            write_plc_mb143_bit PLC_BIT_CUTTER,state;
      ENDPROC

      ! ============================================================
      ! ?????
      ! ============================================================
      PROC COMPACTION_ROLLER_STATE(num state)
            WHILE NOT cmdWhileFlag DO
                  !                  TPWrite "Wait PROGRAM_ID return";
            ENDWHILE
            cmdVerified:=FALSE;
            cmdType:=CMD_COMPACTION_ROLLER_STATE;
            cmdPayload:=NumToStr(state,0);
            cmdReady:=TRUE;
            !            WaitUntil cmdVerified=TRUE;
            WHILE NOT cmdVerified DO
                  !                  TPWrite "Wait MATERIAL return";
            ENDWHILE
            cmdType:=0;
      ENDPROC

      ! ============================================================
      ! ?????
      ! ============================================================
      PROC PINCH_ROLLER_STATE(num state)
            ! Write to PLC (MB143 bit1)
            write_plc_mb143_bit PLC_BIT_PUSH,state;
      ENDPROC

      ! ============================================================
      ! ????(?????4???)
      ! ============================================================
      PROC LAYING_SPEED(num s1,num s2,num s3,num s4)
            WHILE NOT cmdWhileFlag DO
                  !                  TPWrite "Wait PROGRAM_ID return";
            ENDWHILE
            cmdVerified:=FALSE;
            cmdType:=CMD_LAYING_SPEED;
            cmdPayload:=NumToStr(s1,0)+" "+NumToStr(s2,0)+" "+NumToStr(s3,0)+" "+NumToStr(s4,0);
            cmdReady:=TRUE;
            !            WaitUntil cmdVerified=TRUE;
            WHILE NOT cmdVerified DO
                  !                  TPWrite "Wait MATERIAL return";
            ENDWHILE
            cmdType:=0;
      ENDPROC

      ! ============================================================
      ! ?????(?????3???)
      ! ============================================================
      PROC EXTERNAL_AXIS_DISPLACEMENT(num ax1,num ax2,num ax3)
            WHILE NOT cmdWhileFlag DO
                  !                  TPWrite "Wait PROGRAM_ID return";
            ENDWHILE
            cmdVerified:=FALSE;
            cmdType:=CMD_EXTERNAL_AXIS;
            cmdPayload:=NumToStr(ax1,0)+" "+NumToStr(ax2,0)+" "+NumToStr(ax3,0);
            cmdReady:=TRUE;
            !            WaitUntil cmdVerified=TRUE;
            WHILE NOT cmdVerified DO
                  !                  TPWrite "Wait MATERIAL return";
            ENDWHILE
            cmdType:=0;
      ENDPROC

      ! ============================================================
      ! ????
      ! ============================================================
      PROC TOW_TENSION(string tension)
            WHILE NOT cmdWhileFlag DO
                  !                  TPWrite "Wait PROGRAM_ID return";
            ENDWHILE
            cmdVerified:=FALSE;
            cmdType:=CMD_TOW_TENSION;
            cmdPayload:=tension;
            cmdReady:=TRUE;
            !            WaitUntil cmdVerified=TRUE;
            WHILE NOT cmdVerified DO
                  !                  TPWrite "Wait MATERIAL return";
            ENDWHILE
            cmdType:=0;
      ENDPROC

      ! ============================================================
      ! ????
      ! ============================================================
      PROC HEAD_ANGLE(num angle)
            WHILE NOT cmdWhileFlag DO
                  !                  TPWrite "Wait PROGRAM_ID return";
            ENDWHILE
            cmdVerified:=FALSE;
            cmdType:=CMD_HEAD_ANGLE;
            cmdPayload:=NumToStr(angle,0);
            cmdReady:=TRUE;
            !            WaitUntil cmdVerified=TRUE;
            WHILE NOT cmdVerified DO
                  !                  TPWrite "Wait MATERIAL return";
            ENDWHILE
            cmdType:=0;
      ENDPROC

      ! ============================================================
      ! ?????
      ! ============================================================
      PROC FEED_LENGTH(num pre_length, num post_length)
            WHILE NOT cmdWhileFlag DO
                  !                  TPWrite "Wait PROGRAM_ID return";
            ENDWHILE
            cmdVerified:=FALSE;
            cmdType:=CMD_FEED_LENGTH;
            cmdPayload:=NumToStr(pre_length,1)+" "+NumToStr(post_length,1);
            cmdReady:=TRUE;
            !            WaitUntil cmdVerified=TRUE;
            WHILE NOT cmdVerified DO
                  !                  TPWrite "Wait MATERIAL return";
            ENDWHILE
            cmdType:=0;
      ENDPROC

      ! ============================================================
      ! ?????
      ! ============================================================
      PROC FEED_SPEED(num pre_speed, num post_speed)
            WHILE NOT cmdWhileFlag DO
                  !                  TPWrite "Wait PROGRAM_ID return";
            ENDWHILE
            cmdVerified:=FALSE;
            cmdType:=CMD_FEED_SPEED;
            cmdPayload:=NumToStr(pre_speed,1)+" "+NumToStr(post_speed,1);
            cmdReady:=TRUE;
            !            WaitUntil cmdVerified=TRUE;
            WHILE NOT cmdVerified DO
                  !                  TPWrite "Wait MATERIAL return";
            ENDWHILE
            cmdType:=0;
      ENDPROC

      ! ============================================================
      ! ?????
      ! ============================================================
      PROC PLC_FEED_SPEED(num v)
            ! Write to PLC (MD139 -> send_motor, DINT scaled by 1000)
            write_plc_motor_speed v;

            ! Motor state = 3, and execute pulse (matches original DB5 writes)
            write_plc_mb143_bit PLC_BIT_MOTOR_STATE0,1;
            write_plc_mb143_bit PLC_BIT_MOTOR_STATE1,1;
            pulse_plc_mb143_bit PLC_BIT_MOTOR_EXEC,0.02;
      ENDPROC

      ! ============================================================
      ! ?????
      ! ============================================================
      ! NOTE: CMD_POST_FEED_SPEED (29) kept for compatibility, but FEED_SPEED now carries both pre/post values.

      !    ! ============================================================
      !    ! ?????(MOVEL_TO)
      !    ! ============================================================
      !    PROC MOVEL_TO(string coords)
      !        cmdType := CMD_MOVEL_TO;
      !        cmdPayload := coords;
      !        cmdReady := TRUE;
      !    ENDPROC

      !    ! ============================================================
      !    ! ??(MOVEL)
      !    ! ============================================================
      !    PROC MOVEL(string coords)
      !        cmdType := CMD_MOVEL;
      !        cmdPayload := coords;
      !        cmdReady := TRUE;
      !    ENDPROC

      ! ============================================================
      ! ????
      ! ============================================================
      PROC END_TRACE()
            WHILE NOT cmdWhileFlag DO
                  !                  TPWrite "Wait PROGRAM_ID return";
            ENDWHILE
            cmdVerified:=FALSE;
            cmdType:=CMD_END_TRACE;
            cmdPayload:="";
            cmdReady:=TRUE;
            !            WaitUntil cmdVerified=TRUE;
            WHILE NOT cmdVerified DO
                  !                  TPWrite "Wait MATERIAL return";
            ENDWHILE
            cmdType:=0;
      ENDPROC

      ! ============================================================
      ! ????
      ! ============================================================
      PROC END_LAYER()
            WHILE NOT cmdWhileFlag DO
                  !                  TPWrite "Wait PROGRAM_ID return";
            ENDWHILE
            cmdVerified:=FALSE;
            cmdType:=CMD_END_LAYER;
            cmdPayload:="";
            cmdReady:=TRUE;
            !            WaitUntil cmdVerified=TRUE;
            WHILE NOT cmdVerified DO
                  !                  TPWrite "Wait MATERIAL return";
            ENDWHILE
            cmdType:=0;
      ENDPROC

      ! ============================================================
      ! ????
      ! ============================================================
      PROC END_PROGRAM()
            WHILE NOT cmdWhileFlag DO
                  !                  TPWrite "Wait PROGRAM_ID return";
            ENDWHILE
            cmdVerified:=FALSE;
            cmdType:=CMD_END_PROGRAM;
            cmdPayload:="";
            cmdReady:=TRUE;
            !            WaitUntil cmdVerified=TRUE;
            WHILE NOT cmdVerified DO
                  !                  TPWrite "Wait MATERIAL return";
            ENDWHILE
            cmdType:=0;
      ENDPROC


      !////////////////////////
      !//SERVER: Main procedure
      !////////////////////////
      PROC main()
            !//Local variables
            VAR string receivedString;
            !//Received string
            VAR string sendString;
            !//Reply string
            VAR string addString;
            !//String to add to the reply.
            VAR bool connected;
            !//Client connected
            VAR bool reconnected;
            !//Drop and reconnection happened during serving a command
            VAR robtarget cartesianPose;
            VAR jointtarget jointsPose;
            VAR bool socketTimedOut;


            !CASE40?
            VAR num batchSize_CASE_40;
            VAR num idx_CASE_40;

            ! test_abs__joint_zero
            VAR jointtarget joints_0:=[[0,0,0,0,0,0],[9E9,9E9,9E9,9E9,9E9,9E9]];

            VAR num pos_err;
            VAR num ori_err;
            VAR robtarget curPose;
            VAR num reached_idx:=0;

            !    Use an impossible initial value to ensure that the first group will definitely be sent.
            VAR num last_motor:=9E9;
            VAR num last_cutter:=9E9;
            VAR num last_push:=9E9;
            VAR num last_temp:=9E9;
            VAR bool first_plc_of_path;

            ! get string len;
            VAR num str_len;

            ! calculate socketsend and socketreceive speed time
            VAR num start_time;
            VAR num end_time;

            !//Motion configuration
            ConfL\Off;
            SingArea\Wrist;
            moveCompleted:=TRUE;

            IF use_plc_forwarding THEN
                  TPWrite "Motion.main: PLC forwarding enabled";
                  ! Keep running socket server for LOAD_MOD/logging/etc.
                  ! PLC forwarding motion is polled inside the server loop (non-blocking).
            ENDIF

            !//Initialization of WorkObject, Tool, Speed and Zone
            !Initialize;

            !//Socket connection
            connected:=FALSE;
            ServerCreateAndConnect_1 ipController,serverPort;
            connected:=TRUE;

            !            start_time:=GetTime(\Sec);
            !            !          WaitTime 1;
            !            !          TPWrite "reading_start_start_time:"\Num:=start_time;
            !            FOR i FROM 1 TO 1 DO
            !                   main_load;
            !            ENDFOR
            !            end_time:=GetTime(\Sec);
            !            !          TPWrite "reading_finished_end_time:"\Num:=end_time;
            !            TPWrite "enduring:"+ValToStr((end_time-start_time)*1000)+" ms";
            !            TPWrite "average:"+ValToStr((end_time-start_time)*10)+" ms";
            !!            Stop;




            !//Server Loop
            WHILE TRUE DO
LoopTop:
                  IF use_plc_forwarding THEN
                        plc_step;
!                        read_plc_send_values;
                  ENDIF
                  !//Initialization of program flow variables
                  ok:=SERVER_OK;
                  !//Correctness of executed instruction.
                  reconnected:=FALSE;
                  !//Has communication dropped after receiving a command?
                  addString:="";

                  !//Wait for a command
                  !        SocketReceive clientSocket_motion \Str:=receivedString \Time:=WAIT_MAX;
                  !         SocketReceive sockte2\Data:=receiveByte\NoRecBytes:=receiveByteNum;
                  !                  SocketReceive clientSocket_motion\Data:=receiveByte\NoRecBytes:=receiveByteNum\Time:=WAIT_MAX;

                  !      FOR i FROM 1 TO receiveByteNum DO
                  !        TPWrite ByteToStr(receiveByte{i}\Char);
                  !      ENDFOR
                  !                  j:=1;
                  !                str_Var := "";
                  !                FOR strP FROM 1 TO 1024 DO
                  !                    str_Part{strP} := "";
                  !                ENDFOR
                  !                FOR i FROM 1 TO receiveByteNum DO
                  !                    str_Part{i}:=ByteToStr(receiveByte{i}\Char);
                  !                    IF str_Part{i}<>";" THEN
                  !                        str_Var:=str_Var+str_Part{i};
                  !                    ELSEIF str_Part{i}=";" THEN
                  !                        Data{j}:=str_Var;
                  !                        TPWrite "??? "+NumToStr(j,0)+" ?: "+str_Var;
                  !                        str_Var:="";
                  !                        j:=j+1;
                  !                    ENDIF
                  !                ENDFOR
                  !      FOR i FROM 1 TO 1024 DO
                  !        TPWrite "str_part: "+ str_Part{i};
                  !        TPWrite "str_var: "+ str_Var;
                  !        TPWrite "data{}: "+ Data{i};
                  !      ENDFOR
                  !        SafePrint "receive:", receivedString;
                  !        TPWrite "======================================";
                  !        TPWrite "Data reception time time: " + CTime();

                  socketTimedOut:=FALSE;
                  SocketReceive clientSocket_motion\Data:=receiveByte\NoRecBytes:=receiveByteNum\Time:=0.05;
                  IF socketTimedOut THEN
                        WaitTime 0.01;
                        socketTimedOut:=FALSE;
                        GOTO LoopTop;
                  ENDIF
                  tempStr:="";
                  !                  TPWrite "receiveByteNum: "\Num:=receiveByteNum;
                  FOR i FROM 1 TO receiveByteNum DO
                        tempStr:=tempStr+ByteToStr(receiveByte{i}\Char);
                        IF StrPart(tempStr,StrLen(tempStr),1)="#" THEN
                              receivedString:=tempStr;
!                              TPWrite "re:"+receivedString;
                              tempStr:="";
                              !                    EXIT
                        ENDIF
                  ENDFOR
                  TPWrite "te:"+tempStr;
                  TPWrite "re:"+receivedString;
                  str_len:=StrLen(tempStr);
                  !                  TPWrite "tempStr_leng: "\Num:=str_len;
                  !                  TPWrite tempStr;
                  !        TPWrite "receivedString: " + receivedString;


                  load_mod_message_flag:=StrMatch(receivedString,1,"LOAD_MOD:");
                  colonPos:=StrFind(receivedString,1,":");
                  TPWrite receivedString;
                  TPWrite "load_mod_message_flag:"\Num:=load_mod_message_flag;
                  TPWrite "colonPos:"\Num:=colonPos;
                  IF load_mod_message_flag=1 AND colonPos=9 THEN
                        ! ??????????
                        mod_filename:=StrPart(receivedString,StrLen("LOAD_MOD:")+1,StrLen(receivedString)-StrLen("LOAD_MOD:")-1);
                        TPWrite "Analysis to filename: "+mod_filename;
                        ! ??????
                        load_flag:=TRUE;
                        ! invoking 
                        main_load;
                  ELSE
                        TPWrite "Receive is not 'LOAD_MOD:'";
                  ENDIF
                  
                  
                  ParseMsg receivedString;


                  !//Execution of the command
                  TEST instructionCode
                  CASE 0:
                        !Ping
                        TPWrite "CASE 0 - Ping";
                        IF nParams=0 THEN
                              ok:=SERVER_OK;
                        ELSE
                              ok:=SERVER_BAD_MSG;
                        ENDIF

                  CASE 101:
                        !PLC_data
                        TPWrite " CASE 101 - PLC_DATA";
                        IF nParams=4 THEN
                              !                    TPWrite "PLC: motor=" + NumToStr(params{1},2) +
                              !                            ", cutter=" + NumToStr(params{2},2) +
                              !                            ", push=" + NumToStr(params{3},2) +
                              !                            ", temp=" + NumToStr(params{4},2);
                              plc_motor_speed:=params{1};
                              plc_cutter:=params{2};
                              plc_push:=params{3};
                              plc_temp:=params{4};
                              ok:=SERVER_OK;
                        ELSE
                              !                          TPWrite "??: PLC??????, ?4?, ??" + NumToStr(nParams,0) + "?";
                              ok:=SERVER_BAD_MSG;
                        ENDIF

                        !            CASE 100:
                        !                 IF nParams = 6 THEN
                        !                    externalAxis :=[params{1},params{2},params{3},params{4},params{5},params{6}];
                        !                    jointsTarget := CJointT();
                        !                    jointsTarget.extax := externalAxis;
                        !                    ok := SERVER_OK;
                        !                ELSE
                        !                    ok :=SERVER_BAD_MSG;
                        !                ENDIF

                  CASE 100:
                        !External Axis Position
                        TPWrite "????: CASE 100 - ???????";
                        IF nParams=6 THEN
                              !                    TPWrite "?????: eax_a=" + NumToStr(params{1},2) +
                              !                            ", eax_b=" + NumToStr(params{2},2) +
                              !                            ", eax_c=" + NumToStr(params{3},2) +
                              !                            ", eax_d=" + NumToStr(params{4},2) +
                              !                            ", eax_e=" + NumToStr(params{5},2) +
                              !                            ", eax_f=" + NumToStr(params{6},2);
                              externalAxis:=[params{1},params{2},params{3},params{4},params{5},params{6}];
                              jointsTarget:=CJointT();
                              jointsTarget.extax:=externalAxis;
                              ok:=SERVER_OK;
                        ELSE
                              !                    TPWrite "??: ???????,??6?,??" + NumToStr(nParams,0) + "?";
                              ok:=SERVER_BAD_MSG;
                        ENDIF


                        !            CASE 1: !Cartesian Move,case100????????,?????case1d cartesiantarget??
                        !                IF nParams = 7 THEN
                        !                    cartesianTarget :=[[params{1},params{2},params{3}],
                        !                                       [params{4},params{5},params{6},params{7}],
                        !                                       [0,0,0,0],
                        !                                       externalAxis];
                        !                    ok := SERVER_OK;
                        !                    moveCompleted := FALSE;
                        !                    MoveL cartesianTarget,currentSpeed,currentZone,xykTool\WObj:=xykWobj;    !??Movel????
                        !                    moveCompleted := TRUE;
                        !                ELSE
                        !                    ok := SERVER_BAD_MSG;
                        !                ENDIF

                  CASE 1:
                        !Cartesian Move,case100????????,?????case1d cartesiantarget??
                        TPWrite "????: CASE 1 - ?????";
                        IF nParams=7 THEN
                              TPWrite "????: X="+NumToStr(params{1},2)+
                            ", Y="+NumToStr(params{2},2)+
                            ", Z="+NumToStr(params{3},2);
                              TPWrite "????: q1="+NumToStr(params{4},5)+
                            ", q2="+NumToStr(params{5},5)+
                            ", q3="+NumToStr(params{6},5)+
                            ", q4="+NumToStr(params{7},5);
                              cartesianTarget:=[[params{1},params{2},params{3}],
                                       [params{4},params{5},params{6},params{7}],
                                       [0,0,1,0],
                                       externalAxis];
                              ok:=SERVER_OK;
                              moveCompleted:=FALSE;
                              MoveL cartesianTarget,currentSpeed,currentZone,currentTool\WObj:=currentWobj;
                              !??Movel????
                              !                   MoveL cartesianTarget,currentSpeed,currentZone,xykTool\WObj:=hhn1;    !??Movel????
                              moveCompleted:=TRUE;
                        ELSE
                              TPWrite "??: ???????,??7?,??"+NumToStr(nParams,0)+"?";
                              ok:=SERVER_BAD_MSG;
                        ENDIF


                  CASE 2:
                        !Joint Move
                        IF nParams=6 THEN
                              jointsTarget:=[[params{1},params{2},params{3},params{4},params{5},params{6}],externalAxis];
                              ok:=SERVER_OK;
                              moveCompleted:=FALSE;
                              MoveAbsJ jointsTarget,currentSpeed,currentZone,currentTool\Wobj:=currentWobj;
                              moveCompleted:=TRUE;
                        ELSE
                              ok:=SERVER_BAD_MSG;
                        ENDIF

                  CASE 3:
                        !Get Cartesian Coordinates (with current tool and workobject)
                        IF nParams=0 THEN
                              cartesianPose:=CRobT(\Tool:=currentTool\WObj:=currentWObj);
                              !                    cartesianPose := CRobT(\Tool:=xykTool\WObj:=hhn1);
                              addString:=NumToStr(cartesianPose.trans.x,2)+" ";
                              addString:=addString+NumToStr(cartesianPose.trans.y,2)+" ";
                              addString:=addString+NumToStr(cartesianPose.trans.z,2)+" ";
                              addString:=addString+NumToStr(cartesianPose.rot.q1,3)+" ";
                              addString:=addString+NumToStr(cartesianPose.rot.q2,3)+" ";
                              addString:=addString+NumToStr(cartesianPose.rot.q3,3)+" ";
                              addString:=addString+NumToStr(cartesianPose.rot.q4,3);
                              !End of string
                              ok:=SERVER_OK;
                        ELSE
                              ok:=SERVER_BAD_MSG;
                        ENDIF

                  CASE 4:
                        !Get Joint Coordinates
                        IF nParams=0 THEN
                              jointsPose:=CJointT();
                              addString:=NumToStr(jointsPose.robax.rax_1,2)+" ";
                              addString:=addString+NumToStr(jointsPose.robax.rax_2,2)+" ";
                              addString:=addString+NumToStr(jointsPose.robax.rax_3,2)+" ";
                              addString:=addString+NumToStr(jointsPose.robax.rax_4,2)+" ";
                              addString:=addString+NumToStr(jointsPose.robax.rax_5,2)+" ";
                              addString:=addString+NumToStr(jointsPose.robax.rax_6,2);
                              !End of string
                              ok:=SERVER_OK;
                        ELSE
                              ok:=SERVER_BAD_MSG;
                        ENDIF
                  CASE 5:
                        !Get external axis positions
                        IF nParams=0 THEN
                              jointsPose:=CJointT();
                              addString:=StrPart(NumToStr(jointsTarget.extax.eax_a,2),1,8)+" ";
                              addString:=addString+StrPart(NumToStr(jointsTarget.extax.eax_b,2),1,8)+" ";
                              addString:=addString+StrPart(NumToStr(jointsTarget.extax.eax_c,2),1,8)+" ";
                              addString:=addString+StrPart(NumToStr(jointsTarget.extax.eax_d,2),1,8)+" ";
                              addString:=addString+StrPart(NumToStr(jointsTarget.extax.eax_e,2),1,8)+" ";
                              addString:=addString+StrPart(NumToStr(jointsTarget.extax.eax_f,2),1,8);
                              !End of string
                              ok:=SERVER_OK;
                        ELSE
                              ok:=SERVER_BAD_MSG;
                        ENDIF

                  CASE 6:
                        !Set Tool
                        IF nParams=7 THEN
                              WHILE (frameMutex) DO
                                    WaitTime .01;
                                    !// If the frame is being used by logger, wait here
                              ENDWHILE
                              frameMutex:=TRUE;
                              currentTool.tframe.trans.x:=params{1};
                              currentTool.tframe.trans.y:=params{2};
                              currentTool.tframe.trans.z:=params{3};
                              currentTool.tframe.rot.q1:=params{4};
                              currentTool.tframe.rot.q2:=params{5};
                              currentTool.tframe.rot.q3:=params{6};
                              currentTool.tframe.rot.q4:=params{7};
                              ok:=SERVER_OK;
                              frameMutex:=FALSE;
                        ELSE
                              ok:=SERVER_BAD_MSG;
                        ENDIF

                  CASE 7:
                        !Set Work Object
                        IF nParams=7 THEN
                              currentWobj.oframe.trans.x:=params{1};
                              currentWobj.oframe.trans.y:=params{2};
                              currentWobj.oframe.trans.z:=params{3};
                              currentWobj.oframe.rot.q1:=params{4};
                              currentWobj.oframe.rot.q2:=params{5};
                              currentWobj.oframe.rot.q3:=params{6};
                              currentWobj.oframe.rot.q4:=params{7};
                              ok:=SERVER_OK;
                        ELSE
                              ok:=SERVER_BAD_MSG;
                        ENDIF

                        !            CASE 8: !Set Speed of the Robot
                        !                IF nParams = 4 THEN
                        !                    currentSpeed.v_tcp:=params{1};
                        !                    currentSpeed.v_ori:=params{2};
                        !                    currentSpeed.v_leax:=params{3};
                        !                    currentSpeed.v_reax:=params{4};
                        !                    ok := SERVER_OK;
                        !                ELSEIF nParams = 2 THEN
                        !					currentSpeed.v_tcp:=params{1};
                        !					currentSpeed.v_ori:=params{2};
                        !					ok := SERVER_OK;
                        !				ELSE
                        !                    ok:=SERVER_BAD_MSG;
                        !                ENDIF

                  CASE 8:
                        !Set Speed of the Robot
                        TPWrite "????: CASE 8 - ????";
                        IF nParams=4 THEN
                              !                      TPWrite "????: v_tcp=" + NumToStr(params{1},2) +
                              !                            ", v_ori=" + NumToStr(params{2},2) +
                              !                            ", v_leax=" + NumToStr(params{3},2) +
                              !                            ", v_reax=" + NumToStr(params{4},2);
                              currentSpeed.v_tcp:=params{1};
                              currentSpeed.v_ori:=params{2};
                              currentSpeed.v_leax:=params{3};
                              currentSpeed.v_reax:=params{4};
                              ok:=SERVER_OK;
                        ELSEIF nParams=2 THEN
                              !                      TPWrite "????(??): v_tcp=" + NumToStr(params{1},2) +
                              !                            ", v_ori=" + NumToStr(params{2},2);
                              currentSpeed.v_tcp:=params{1};
                              currentSpeed.v_ori:=params{2};
                              ok:=SERVER_OK;
                        ELSE
                              !                  TPWrite "??: ???????";
                              ok:=SERVER_BAD_MSG;
                        ENDIF

                  CASE 9:
                        !Set zone data
                        IF nParams=4 THEN
                              IF params{1}=1 THEN
                                    currentZone.finep:=TRUE;
                                    currentZone.pzone_tcp:=0.0;
                                    currentZone.pzone_ori:=0.0;
                                    currentZone.zone_ori:=0.0;
                              ELSE
                                    currentZone.finep:=FALSE;
                                    currentZone.pzone_tcp:=params{2};
                                    currentZone.pzone_ori:=params{3};
                                    currentZone.zone_ori:=params{4};
                              ENDIF
                              ok:=SERVER_OK;
                        ELSE
                              ok:=SERVER_BAD_MSG;
                        ENDIF

                        !            CASE 30: !Add Cartesian Coordinates to buffer
                        !                IF nParams = 7 THEN
                        !                    cartesianTarget :=[[params{1},params{2},params{3}],
                        !                                        [params{4},params{5},params{6},params{7}],
                        !                                        [0,0,0,0],
                        !                                        externalAxis];
                        !                    IF BUFFER_POS < MAX_BUFFER THEN
                        !                        BUFFER_POS := BUFFER_POS + 1;
                        !                        bufferTargets{BUFFER_POS} := cartesianTarget;
                        !                        bufferSpeeds{BUFFER_POS} := currentSpeed;
                        !                    ENDIF
                        !                    ok := SERVER_OK;
                        !                ELSE
                        !                    ok:=SERVER_BAD_MSG;
                        !                ENDIF


                  CASE 30:
                        !Add Cartesian Coordinates to buffer
                        TPWrite "????: CASE 30 - ??????";
                        IF nParams=7 THEN

                              !                  TPWrite "????" + NumToStr(BUFFER_POS+1,0) + ": [" +
                              !                            NumToStr(params{1},2) + ", " + NumToStr(params{2},2) + ", " + NumToStr(params{3},2) + "]";
                              !                    TPWrite "????: [" + NumToStr(params{4},5) + ", " + NumToStr(params{5},5) +
                              !                            ", " + NumToStr(params{6},5) + ", " + NumToStr(params{7},5) + "]";

                              cartesianTarget:=[[params{1},params{2},params{3}],
                                        [params{4},params{5},params{6},params{7}],
                                        [0,0,0,0],
                                        externalAxis];
                              IF BUFFER_POS<MAX_BUFFER THEN
                                    BUFFER_POS:=BUFFER_POS+1;
                                    bufferTargets{BUFFER_POS}:=cartesianTarget;
                                    bufferSpeeds{BUFFER_POS}:=currentSpeed;

                                    !new plc buffer add
                                    bufferPLC_motor{BUFFER_POS}:=plc_motor_speed;
                                    bufferPLC_cutter{BUFFER_POS}:=plc_cutter;
                                    bufferPLC_push{BUFFER_POS}:=plc_push;
                                    bufferPLC_temp{BUFFER_POS}:=plc_temp;

                              ENDIF
                              ok:=SERVER_OK;
                        ELSE
                              !                      TPWrite "??: ???????";
                              ok:=SERVER_BAD_MSG;
                        ENDIF


                  CASE 31:
                        !Clear Cartesian Buffer
                        IF nParams=0 THEN
                              BUFFER_POS:=0;
                              ok:=SERVER_OK;
                        ELSE
                              ok:=SERVER_BAD_MSG;
                        ENDIF

                  CASE 32:
                        !Get Buffer Size)
                        IF nParams=0 THEN
                              addString:=NumToStr(BUFFER_POS,2);
                              ok:=SERVER_OK;
                        ELSE
                              ok:=SERVER_BAD_MSG;
                        ENDIF



                  CASE 40:
                        !????????(????)
                        TPWrite "????: CASE 40 - ????????";
                        IF nParams>=7 THEN
                              ! ?1???:????N
                              batchSize_CASE_40:=params{1};
                              TPWrite "????: "+NumToStr(batchSize_CASE_40,0);
                              TPWrite "????: "+NumToStr(nParams,0);

                              idx_CASE_40:=2;
                              ! ?????2??

                              ! ??buffer
                              BUFFER_POS:=0;

                              ! ????N??
                              FOR i FROM 1 TO batchSize_CASE_40 DO
                                    IF idx_CASE_40+6<=nParams THEN
                                          cartesianTarget:=[[params{idx_CASE_40},params{idx_CASE_40+1},params{idx_CASE_40+2}],
                                                [params{idx_CASE_40+3},params{idx_CASE_40+4},params{idx_CASE_40+5},params{idx_CASE_40+6}],
                                                [0,0,0,0],
                                                externalAxis];

                                          IF BUFFER_POS<MAX_BUFFER THEN
                                                BUFFER_POS:=BUFFER_POS+1;
                                                bufferTargets{BUFFER_POS}:=cartesianTarget;
                                                bufferSpeeds{BUFFER_POS}:=currentSpeed;
                                          ENDIF
                                          idx_CASE_40:=idx_CASE_40+7;
                                    ENDIF
                              ENDFOR
                              ok:=SERVER_OK;
                        ELSE
                              TPWrite "??: ??????";
                              ok:=SERVER_BAD_MSG;
                        ENDIF


                        !            CASE 33: !Execute moves in cartesianBuffer as linear moves
                        !                IF nParams = 0 THEN
                        !                    FOR i FROM 1 TO (BUFFER_POS) DO
                        !                        MoveL bufferTargets{i}, bufferSpeeds{i}, currentZone, currentTool \WObj:=currentWobj ;
                        !                    ENDFOR
                        !                    ok := SERVER_OK;
                        !                ELSE
                        !                    ok:=SERVER_BAD_MSG;
                        !                ENDIF


                  CASE 33:
                        !Execute moves in cartesianBuffer as linear moves
                        TPWrite "inter: CASE 33 -------------";
                        TPWrite "?????: "+NumToStr(BUFFER_POS,0);
                        IF nParams=0 THEN
                              !Each time entering CASE 33, reset the first row PLC flag position
                              first_plc_of_path:=TRUE;
                              FOR i FROM 1 TO (BUFFER_POS) DO
                                    reached_idx:=0;

                                    !                        IF (bufferPLC_motor{i} <> 0) OR (bufferPLC_cutter{i} <> 0) OR (bufferPLC_push{i} <> 0) OR (bufferPLC_temp{i} <> 0) THEN
                                    !                            TPWrite "reached:" + NumToStr(i,0) + " line";
                                    !                            TPWrite "bufferTargets: " +
                                    !                                NumToStr(bufferTargets{i}.trans.x,2) + ", " +
                                    !                                NumToStr(bufferTargets{i}.trans.y,2) + ", " +
                                    !                                NumToStr(bufferTargets{i}.trans.z,2) + " | " +
                                    !                                NumToStr(bufferTargets{i}.rot.q1,5) + ", " +
                                    !                                NumToStr(bufferTargets{i}.rot.q2,5) + ", " +
                                    !                                NumToStr(bufferTargets{i}.rot.q3,5) + ", " +
                                    !                                NumToStr(bufferTargets{i}.rot.q4,5);
                                    !                            TPWrite "PLC_buffer: motor=" + NumToStr(bufferPLC_motor{i},2) +
                                    !                                ", cutter=" + NumToStr(bufferPLC_cutter{i},2) +
                                    !                                ", push=" + NumToStr(bufferPLC_push{i},2) +
                                    !                                ", temp=" + NumToStr(bufferPLC_temp{i},2);
                                    !                                SendPLCParamsToPC bufferPLC_motor{i}, bufferPLC_cutter{i}, bufferPLC_push{i}, bufferPLC_temp{i};
                                    !                        ENDIF

                                    ! Only the PLC data here determines whether to send or not.
                                    IF first_plc_of_path OR
                           (bufferPLC_motor{i}<>last_motor) OR
                           (bufferPLC_cutter{i}<>last_cutter) OR
                           (bufferPLC_push{i}<>last_push) OR
                           (bufferPLC_temp{i}<>last_temp) THEN

                                          TPWrite "PLC changed at line "+NumToStr(i,0);
                                          TPWrite "PLC_buffer: motor="+NumToStr(bufferPLC_motor{i},2)+
                                      ", cutter="+NumToStr(bufferPLC_cutter{i},2)+
                                      ", push="+NumToStr(bufferPLC_push{i},2)+
                                      ", temp="+NumToStr(bufferPLC_temp{i},2);

                                          ! ???????????????
                                          SendPLCParamsToPC bufferPLC_motor{i},
                                                bufferPLC_cutter{i},
                                                bufferPLC_push{i},
                                                bufferPLC_temp{i};

                                          ! ??"???"??(? 0,0,0,0)
                                          last_motor:=bufferPLC_motor{i};
                                          last_cutter:=bufferPLC_cutter{i};
                                          last_push:=bufferPLC_push{i};
                                          last_temp:=bufferPLC_temp{i};

                                          ! update_plc,reset_single
                                          first_plc_of_path:=FALSE;
                                    ENDIF
                                    MoveL bufferTargets{i},bufferSpeeds{i},currentZone,currentTool\WObj:=currentWobj;
                                    !                         MoveL bufferTargets{i}, bufferSpeeds{i}, currentZone, xykTool \WObj:=hhn1 ;

                                    !                         ! get current site
                                    !                        curPose := CRobT(\Tool:=currentTool \WObj:=currentWobj);
                                    !                        ! ??????????????
                                    !                        pos_err := Sqrt(
                                    !                            Pow(curPose.trans.x - bufferTargets{i}.trans.x, 2) +
                                    !                            Pow(curPose.trans.y - bufferTargets{i}.trans.y, 2) +
                                    !                            Pow(curPose.trans.z - bufferTargets{i}.trans.z, 2)
                                    !                        );
                                    !                        ori_err := Sqrt(
                                    !                            Pow(curPose.rot.q1 - bufferTargets{i}.rot.q1, 2) +
                                    !                            Pow(curPose.rot.q2 - bufferTargets{i}.rot.q2, 2) +
                                    !                            Pow(curPose.rot.q3 - bufferTargets{i}.rot.q3, 2) +
                                    !                            Pow(curPose.rot.q4 - bufferTargets{i}.rot.q4, 2)
                                    !                        );
                                    !                        ! ?????????
                                    !                        IF (pos_err < 0.1) AND (ori_err < 0.01) THEN
                                    !                            reached_idx := i;
                                    !                            TPWrite "reached: " + NumToStr(reached_idx,0) + " line";
                                    !                        ENDIF

                                    !                        ! print plc data buffer
                                    !                        TPWrite "PLC_buffer: motor=" + NumToStr(bufferPLC_motor{i},2) +
                                    !                          ", cutter=" + NumToStr(bufferPLC_cutter{i},2) +
                                    !                          ", push=" + NumToStr(bufferPLC_push{i},2) +
                                    !                          ", temp=" + NumToStr(bufferPLC_temp{i},2);

                              ENDFOR
                              ok:=SERVER_OK;
                        ELSE
                              ok:=SERVER_BAD_MSG;
                        ENDIF






                  CASE 34:
                        !External Axis move
                        IF nParams=6 THEN
                              externalAxis:=[params{1},params{2},params{3},params{4},params{5},params{6}];
                              jointsTarget:=CJointT();
                              jointsTarget.extax:=externalAxis;
                              ok:=SERVER_OK;
                              moveCompleted:=FALSE;
                              MoveAbsJ jointsTarget,currentSpeed,currentZone,currentTool\WObj:=currentWobj;
                              moveCompleted:=TRUE;
                        ELSE
                              ok:=SERVER_BAD_MSG;
                        ENDIF

                  CASE 35:
                        !Specify circPoint for circular move, and then wait on toPoint
                        IF nParams=7 THEN
                              circPoint:=[[params{1},params{2},params{3}],
                                [params{4},params{5},params{6},params{7}],
                                [0,0,0,0],
                                externalAxis];
                              ok:=SERVER_OK;
                        ELSE
                              ok:=SERVER_BAD_MSG;
                        ENDIF

                  CASE 36:
                        !specify toPoint, and use circPoint specified previously
                        IF nParams=7 THEN
                              cartesianTarget:=[[params{1},params{2},params{3}],
                                        [params{4},params{5},params{6},params{7}],
                                        [0,0,0,0],
                                        externalAxis];
                              MoveC circPoint,cartesianTarget,currentSpeed,currentZone,currentTool\WObj:=currentWobj;
                              ok:=SERVER_OK;
                        ELSE
                              ok:=SERVER_BAD_MSG;
                        ENDIF

                  CASE 98:
                        !returns current robot info: serial number, robotware version, and robot type
                        IF nParams=0 THEN
                              addString:=GetSysInfo(\SerialNo)+"*";
                              addString:=addString+GetSysInfo(\SWVersion)+"*";
                              addString:=addString+GetSysInfo(\RobotType);
                              ok:=SERVER_OK;
                        ELSE
                              ok:=SERVER_BAD_MSG;
                        ENDIF

                  CASE 99:
                        !Close Connection
                        IF nParams=0 THEN
                              TPWrite "SERVER: Client has closed connection.";
                              connected:=FALSE;
                              !//Closing the server
                              SocketClose clientSocket_motion;
                              SocketClose serverSocket_motion;

                              !Reinitiate the server
                              ServerCreateAndConnect_1 ipController,serverPort;
                              connected:=TRUE;
                              reconnected:=TRUE;
                              ok:=SERVER_OK;
                        ELSE
                              ok:=SERVER_BAD_MSG;
                        ENDIF
                  DEFAULT:
                        TPWrite "SERVER: Illegal instruction code";
                        ok:=SERVER_BAD_MSG;
                  ENDTEST


                  !                !//??????
                  !        IF ok = SERVER_OK THEN
                  !            TPWrite "instruct sucessful";
                  !        ELSE
                  !            TPWrite "instruct unsucessful";
                  !        ENDIF
                  !        TPWrite "======================================";


                  !Compose the acknowledge string to send back to the client
                  IF connected=TRUE THEN
                        IF reconnected=FALSE THEN
                              IF SocketGetStatus(clientSocket_motion)=SOCKET_CONNECTED THEN
                                    sendString:=NumToStr(instructionCode,0);
                                    sendString:=sendString+" "+NumToStr(ok,0);
                                    sendString:=sendString+" "+addString;
                                    SocketSend clientSocket_motion\Str:=sendString;
                              ENDIF
                        ENDIF
                  ENDIF
            ENDWHILE

      ERROR (LONG_JMP_ALL_ERR)
!            TPWrite "SERVER: ------";
!            TPWrite "SERVER: Error Handler:"+NumtoStr(ERRNO,0);
            TEST ERRNO
            CASE ERR_SOCK_TIMEOUT:
                  socketTimedOut:=TRUE;
                  TRYNEXT;
            CASE ERR_SOCK_CLOSED:
                  TPWrite "SERVER: Lost connection to the client.";
                  TPWrite "SERVER: Closing socket and restarting.";
                  TPWrite "SERVER: ------";
                  connected:=FALSE;
                  !//Closing the server
                  SocketClose clientSocket_motion;
                  SocketClose serverSocket_motion;
                  !//Reinitiate the server
                  ServerCreateAndConnect_1 ipController,serverPort;
                  reconnected:=FALSE;
                  connected:=TRUE;
                  RETRY;
            DEFAULT:
                  TPWrite "SERVER: Unknown error.";
                  TPWrite "SERVER: Closing socket and restarting.";
                  TPWrite "SERVER: ------";
                  connected:=FALSE;
                  !//Closing the server
                  SocketClose clientSocket_motion;
                  SocketClose serverSocket_motion;
                  !//Reinitiate the server
                  ServerCreateAndConnect_1 ipController,serverPort;
                  reconnected:=FALSE;
                  connected:=TRUE;
                  RETRY;
            ENDTEST
      ENDPROC

ENDMODULE