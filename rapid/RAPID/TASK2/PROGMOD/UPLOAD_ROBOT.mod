MODULE UPLOAD_ROBOT

      !////////////////
      !GLOBAL VARIABLES
      !////////////////
      !PC communication
      VAR socketdev clientSocket_upload;
      VAR socketdev serverSocket_upload;
      PERS string ipController;
      PERS num UPLOADPort:=5012;

      !Robot configuration
      PERS tooldata currentTool;
      PERS wobjdata currentWobj;
      VAR speeddata currentSpeed;
      VAR zonedata currentZone;

      VAR string data;
      VAR robtarget position;
      VAR jointtarget joints;
      VAR string sendString;
      PERS bool connected;
      VAR string date;
      VAR string time;
      VAR clock timer;

      ! PLC outputs cache (packed bits)
      VAR num plc_mb143_byte:=0;
      PERS bool plc_laser_state:=TRUE;

      ! MB143 bit mapping (keep within existing 8-bit output, no new signals)
      CONST num PLC_BIT_MOTOR_EXEC:=1;
      CONST num PLC_BIT_LASER_CONFIRM:=2;
      CONST num PLC_BIT_LASER_ON:=4;
      CONST num PLC_BIT_LASER_OFF:=8;
      CONST num PLC_BIT_MOTOR_STATE0:=16;
      CONST num PLC_BIT_MOTOR_STATE1:=32;
      CONST num PLC_BIT_CUTTER:=64;
      CONST num PLC_BIT_PUSH:=128;

      !//UPLOAD sampling rate
      !PERS num UPLOADWaitTime:= 0.01;  !Recommended for real controller
      PERS num UPLOADWaitTime:=0.1;
      !Recommended for virtual controller

      ! Robot monitoring stream for AFP_ABB_CPP Slice K.
      ! The PC monitor client connects to this TASK2 socket and records
      ! framed ROBOT_MONITOR messages into CSV/raw files.
      PERS bool monitor_enabled:=TRUE;

      ! Keep this synchronized with robot.monitor.sample_interval_ms in
      ! config/env_config.json during hardware validation.
      PERS num monitor_wait_time:=0.1;

      ! AFP_ABB_CPP sends this lightweight marker while the monitor socket is
      ! connected. It is not a robot command; it only lets RAPID detect that an
      ! old PC process has closed and a new process should be accepted.
      PERS string monitor_heartbeat_marker:="ROBOT_MONITOR_CLIENT_ALIVE";

      ! If no heartbeat is received within this period, close the old client
      ! socket and wait for AFP_ABB_CPP to reconnect.
      PERS num monitor_heartbeat_timeout:=3.0;

      ! Poll heartbeat less frequently than monitor samples so the receive
      ! timeout cannot add noticeable delay to every robot monitor frame.
      PERS num monitor_heartbeat_poll_interval:=0.5;

      ! Sequence counter lets the PC detect skipped or repeated samples.
      PERS num monitor_seq:=0;

      ! Clock used by CheckMonitorHeartbeat to detect stale accepted clients.
      VAR clock monitor_heartbeat_timer;

      ! Clock used to limit how often RAPID polls for heartbeat bytes.
      VAR clock monitor_heartbeat_poll_timer;


      PERS bool cmdReady:=FALSE;
      PERS num cmdType:=0;
      PERS string cmdPayload:="0";
      ! send finish flag
      PERS bool cmdSent:=TRUE;
      PERS bool ackReceived:=TRUE;
      ! revive server buffer
      PERS string ackMessage:="#FIBER_ANGLE#";
      PERS bool cmdVerified:=TRUE;
      PERS bool cmdWhileFlag:=TRUE;

      PERS bool BeginProgramFlag;

      ! ============================================================
      ! cmdType_eumn
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
      PROC set_dint_bytes_local(VAR signalgo go_b1,VAR signalgo go_b2,VAR signalgo go_b3,VAR signalgo go_b4,num dint_value)
            VAR rawbytes rawbyte1;
            VAR num b1;
            VAR num b2;
            VAR num b3;
            VAR num b4;

            ClearRawBytes rawbyte1;
            PackRawBytes dint_value,rawbyte1,1\IntX:=-4;

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

      PROC plc_laser_power(num power)
            ! Convention kept from original PC code:
            !   power=0 : do nothing
            !   power=1 : close laser
            !   power>1 : set/update power; if laser was off, confirm + turn on
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

      PROC plc_motor_speed(num speed)
            write_plc_motor_speed speed;
            write_plc_mb143_bit PLC_BIT_MOTOR_STATE0,1;
            write_plc_mb143_bit PLC_BIT_MOTOR_STATE1,1;
            pulse_plc_mb143_bit PLC_BIT_MOTOR_EXEC,0.02;
      ENDPROC

      PROC ServerCreateAndConnect_3(string ip,num port)
            VAR string clientIP;

            SocketCreate serverSocket_upload;
            SocketBind serverSocket_upload,ip,port;
            SocketListen serverSocket_upload;
            AcceptUploadClient;
      ENDPROC

      PROC AcceptUploadClient()
            VAR string clientIP;

            TPWrite "UPLOAD: UPLOAD waiting for incomming connections ...";
            WHILE SocketGetStatus(clientSocket_upload)<>SOCKET_CONNECTED DO
                  SocketAccept serverSocket_upload,clientSocket_upload\ClientAddress:=clientIP\Time:=WAIT_MAX;
                  IF SocketGetStatus(clientSocket_upload)<>SOCKET_CONNECTED THEN
                        TPWrite "UPLOAD: Problem serving an incomming connection.";
                        TPWrite "UPLOAD: Try reconnecting.";
                  ENDIF
                  !Wait 0.5 seconds for the next reconnection
                  WaitTime 0.5;
            ENDWHILE
            TPWrite "UPLOAD: Connected to IP "+clientIP;
            ResetMonitorHeartbeat;
      ENDPROC

      PROC ReconnectUploadClient()
            ! Re-enter SocketAccept after the PC client closes and later
            ! restarts. Without this, RAPID may keep the old client socket and
            ! the restarted C++ app can connect without receiving monitor data.
            TPWrite "UPLOAD: Client disconnected. Waiting for reconnection ...";
            connected:=FALSE;
            SocketClose clientSocket_upload;
            AcceptUploadClient;
            connected:=TRUE;
      ERROR
            TPWrite "UPLOAD: Reconnect failed. Recreating server socket ...";
            connected:=FALSE;
            SocketClose clientSocket_upload;
            SocketClose serverSocket_upload;
            ServerCreateAndConnect_3 ipController,UPLOADPort;
            connected:=TRUE;
      ENDPROC

      PROC ResetMonitorHeartbeat()
            ! Start from zero whenever a PC monitor client is newly accepted.
            ClkReset monitor_heartbeat_timer;
            ClkStart monitor_heartbeat_timer;
            ClkReset monitor_heartbeat_poll_timer;
            ClkStart monitor_heartbeat_poll_timer;
      ENDPROC

      PROC CheckMonitorHeartbeat()
            VAR string monitor_rx;

            IF monitor_enabled=FALSE THEN
                  RETURN;
            ENDIF

            IF SocketGetStatus(clientSocket_upload)<>SOCKET_CONNECTED THEN
                  ReconnectUploadClient;
                  RETURN;
            ENDIF

            IF ClkRead(monitor_heartbeat_timer)>monitor_heartbeat_timeout THEN
                  TPWrite "UPLOAD: Monitor heartbeat timeout. Waiting for reconnection ...";
                  ReconnectUploadClient;
                  RETURN;
            ENDIF

            IF ClkRead(monitor_heartbeat_poll_timer)<monitor_heartbeat_poll_interval THEN
                  RETURN;
            ENDIF
            ClkReset monitor_heartbeat_poll_timer;
            ClkStart monitor_heartbeat_poll_timer;

            ! Poll briefly for the PC heartbeat only after the current monitor
            ! frame has already been sent. The 0.001 s receive timeout can add
            ! about 1 ms only on poll cycles, not on every monitor sample.
            SocketReceive clientSocket_upload\Str:=monitor_rx\ReadNoOfBytes:=80\Time:=0.001;
            IF StrFind(monitor_rx,1,monitor_heartbeat_marker)>0 THEN
                  ResetMonitorHeartbeat;
            ENDIF
      ERROR
            TEST ERRNO
            CASE ERR_SOCK_TIMEOUT:
                  IF ClkRead(monitor_heartbeat_timer)>monitor_heartbeat_timeout THEN
                        TPWrite "UPLOAD: Monitor heartbeat timeout. Waiting for reconnection ...";
                        ReconnectUploadClient;
                  ENDIF
                  TRYNEXT;
            CASE ERR_SOCK_CLOSED:
                  ReconnectUploadClient;
                  TRYNEXT;
            DEFAULT:
                  TPWrite "UPLOAD: Monitor heartbeat receive error. Waiting for reconnection ...";
                  ReconnectUploadClient;
                  TRYNEXT;
            ENDTEST
      ENDPROC

      !      PROC BEGIN_PROGRAM()
      !    IF connected = TRUE THEN
      !        SocketSend clientSocket_upload \Str:="#BEGIN_PROGRAM";
      !    ENDIF
      !ENDPROC

      !PROC END_PROGRAM()
      !    IF connected = TRUE THEN
      !        SocketSend clientSocket_upload \Str:="#END_PROGRAM";
      !    ENDIF
      !ENDPROC

      !PROC BEGIN_LAYER()
      !    IF connected = TRUE THEN
      !        SocketSend clientSocket_upload \Str:="#BEGIN_LAYER";
      !    ENDIF
      !ENDPROC

      !PROC END_LAYER()
      !    IF connected = TRUE THEN
      !        SocketSend clientSocket_upload \Str:="#END_LAYER";
      !    ENDIF
      !ENDPROC

      !PROC BEGIN_TRACE()
      !    IF connected = TRUE THEN
      !        SocketSend clientSocket_upload \Str:="#BEGIN_TRACE";
      !    ENDIF
      !ENDPROC

      !PROC END_TRACE()
      !    IF connected = TRUE THEN
      !        SocketSend clientSocket_upload \Str:="#END_TRACE";
      !    ENDIF
      !ENDPROC

      !PROC PROGRAM_ID(string id)
      !    VAR string msg;
      !    msg := "PROGRAM_ID " + id;
      !    IF connected = TRUE THEN
      !        SocketSend clientSocket_upload \Str:="#" + msg;
      !    ENDIF
      !ENDPROC

      !PROC MATERIAL(string id)
      !    VAR string msg;
      !    msg := "MATERIAL " + id;
      !    IF connected = TRUE THEN
      !        SocketSend clientSocket_upload \Str:="#" + msg;
      !    ENDIF
      !ENDPROC

      !PROC NOMINAL_WIDTH(num v)
      !    VAR string msg;
      !    msg := "NOMINAL_WIDTH " + NumToStr(v,3);
      !    IF connected = TRUE THEN
      !        SocketSend clientSocket_upload \Str:="#" + msg;
      !    ENDIF
      !ENDPROC

      !PROC NUMBER_OF_TOWS(num v)
      !    VAR string msg;
      !    msg := "NUMBER_OF_TOWS " + NumToStr(v,0);
      !    IF connected = TRUE THEN
      !        SocketSend clientSocket_upload \Str:="#" + msg;
      !    ENDIF
      !ENDPROC

      !PROC WATER_COOLING_TEMP(num v)
      !    VAR string msg;
      !    msg := "WATER_COOLING_TEMP " + NumToStr(v,0);
      !    IF connected = TRUE THEN
      !        SocketSend clientSocket_upload \Str:="#" + msg;
      !    ENDIF
      !ENDPROC

      !PROC LAYER_Thickness(num v)
      !    VAR string msg;
      !    msg := "LAYER_Thickness " + NumToStr(v,3);
      !    IF connected = TRUE THEN
      !        SocketSend clientSocket_upload \Str:="#" + msg;
      !    ENDIF
      !ENDPROC

      !PROC FIBER_ANGLE(num a1, num a2, num a3, num a4)
      !    VAR string msg;
      !    msg := "FIBER_ANGLE " + NumToStr(a1,0) + " " + NumToStr(a2,0) + " " + NumToStr(a3,0) + " " + NumToStr(a4,0);
      !    IF connected = TRUE THEN
      !        SocketSend clientSocket_upload \Str:="#" + msg;
      !    ENDIF
      !ENDPROC

      !PROC LAYER_NUMBER(num v)
      !    VAR string msg;
      !    msg := "LAYER_NUMBER " + NumToStr(v,0);
      !    IF connected = TRUE THEN
      !        SocketSend clientSocket_upload \Str:="#" + msg;
      !    ENDIF
      !ENDPROC

      !PROC LAYER_ID(num v)
      !    VAR string msg;
      !    msg := "LAYER_ID " + NumToStr(v,0);
      !    IF connected = TRUE THEN
      !        SocketSend clientSocket_upload \Str:="#" + msg;
      !    ENDIF
      !ENDPROC

      !PROC TRACE_ID(num v)
      !    VAR string msg;
      !    msg := "TRACE_ID " + NumToStr(v,0);
      !    IF connected = TRUE THEN
      !        SocketSend clientSocket_upload \Str:="#" + msg;
      !    ENDIF
      !ENDPROC

      !PROC COMPACTION_PRESSURE(num v)
      !    VAR string msg;
      !    msg := "COMPACTION_PRESSURE " + NumToStr(v,0);
      !    IF connected = TRUE THEN
      !        SocketSend clientSocket_upload \Str:="#" + msg;
      !    ENDIF
      !ENDPROC

      !PROC LASER_EMISSION(num v)
      !    VAR string msg;
      !    msg := "LASER_EMISSION " + NumToStr(v,0);
      !    IF connected = TRUE THEN
      !        SocketSend clientSocket_upload \Str:="#" + msg;
      !    ENDIF
      !ENDPROC

      !PROC LASER_POWER(num v)
      !    VAR string msg;
      !    msg := "LASER_POWER " + NumToStr(v,0);
      !    IF connected = TRUE THEN
      !        SocketSend clientSocket_upload \Str:="#" + msg;
      !    ENDIF
      !ENDPROC

      !PROC CUTTER_STATE(num v)
      !    VAR string msg;
      !    msg := "CUTTER_STATE " + NumToStr(v,0);
      !    IF connected = TRUE THEN
      !        SocketSend clientSocket_upload \Str:="#" + msg;
      !    ENDIF
      !ENDPROC

      !PROC COMPACTION_ROLLER_STATE(num v)
      !    VAR string msg;
      !    msg := "COMPACTION_ROLLER_STATE " + NumToStr(v,0);
      !    IF connected = TRUE THEN
      !        SocketSend clientSocket_upload \Str:="#" + msg;
      !    ENDIF
      !ENDPROC

      !PROC PINCH_ROLLER_STATE(num v)
      !    VAR string msg;
      !    msg := "PINCH_ROLLER_STATE " + NumToStr(v,0);
      !    IF connected = TRUE THEN
      !        SocketSend clientSocket_upload \Str:="#" + msg;
      !    ENDIF
      !ENDPROC

      !PROC LAYING_SPEED(num srl, num sro, num sel, num seo)
      !    VAR string msg;
      !    msg := "LAYING_SPEED " + NumToStr(srl,0) + " " + NumToStr(sro,0) + " " + NumToStr(sel,0) + " " + NumToStr(seo,0);
      !    IF connected = TRUE THEN
      !        SocketSend clientSocket_upload \Str:="#" + msg;
      !    ENDIF
      !ENDPROC

      !PROC EXTERNAL_AXIS_DISPLACEMENT(num gra, num ta, num ra)
      !    VAR string msg;
      !    msg := "EXTERNAL_AXIS_DISPLACEMENT " + NumToStr(gra,0) + " " + NumToStr(ta,0) + " " + NumToStr(ra,0);
      !    IF connected = TRUE THEN
      !        SocketSend clientSocket_upload \Str:="#" + msg;
      !    ENDIF
      !ENDPROC

      !PROC TOW_TENSION(string v)
      !    VAR string msg;
      !    msg := "TOW_TENSION " + v;
      !    IF connected = TRUE THEN
      !        SocketSend clientSocket_upload \Str:="#" + msg;
      !    ENDIF
      !ENDPROC

      !PROC HEAD_ANGLE(num v)
      !    VAR string msg;
      !    msg := "HEAD_ANGLE " + NumToStr(v,0);
      !    IF connected = TRUE THEN
      !        SocketSend clientSocket_upload \Str:="#" + msg;
      !    ENDIF
      !ENDPROC

      !PROC PRE_FEED_LENGTH(num v)
      !    VAR string msg;
      !    msg := "PRE_FEED_LENGTH " + NumToStr(v,3);
      !    IF connected = TRUE THEN
      !        SocketSend clientSocket_upload \Str:="#" + msg;
      !    ENDIF
      !ENDPROC

      !PROC PRE_FEED_SPEED(num v)
      !    VAR string msg;
      !    msg := "PRE_FEED_SPEED " + NumToStr(v,3);
      !    IF connected = TRUE THEN
      !        SocketSend clientSocket_upload \Str:="#" + msg;
      !    ENDIF
      !ENDPROC

      !PROC POST_FEED_LENGTH(num v)
      !    VAR string msg;
      !    msg := "POST_FEED_LENGTH " + NumToStr(v,3);
      !    IF connected = TRUE THEN
      !        SocketSend clientSocket_upload \Str:="#" + msg;
      !    ENDIF
      !ENDPROC

      !PROC POST_FEED_SPEED(num v)
      !    VAR string msg;
      !    msg := "POST_FEED_SPEED " + NumToStr(v,3);
      !    IF connected = TRUE THEN
      !        SocketSend clientSocket_upload \Str:="#" + msg;
      !    ENDIF
      !ENDPROC

      !!PROC MOVEL_TO(string rest)
      !!    IF connected = TRUE THEN
      !!        SocketSend clientSocket_upload \Str:="#MOVEL_TO " + rest;
      !!    ENDIF
      !!ENDPROC

      !!PROC MOVEL(string rest)
      !!    IF connected = TRUE THEN
      !!        SocketSend clientSocket_upload \Str:="#MOVEL " + rest;
      !!    ENDIF
      !!ENDPROC


      ! ============ Unified sending function ============
      PROC SocketSend_Direct(string msg)
            IF connected=TRUE THEN
                  SocketSend clientSocket_upload\Str:="#"+msg;
                  TPWrite "Send: "+msg;
            ENDIF
      ENDPROC

      !      PROC SocketReceive_Direct()
      !            VAR string receivedData;
      !            VAR num hashPos;
      !            WHILE TRUE DO
      !                  SocketReceive clientSocket_upload\Str:=receivedData\ReadNoOfBytes:=80;
      !                  ackMessage:=receivedData;
      !                  ackReceived:=TRUE;
      !                  TPWrite "recevice ACK: "+ackMessage;
      !            ENDWHILE
      !      ENDPROC


      !      PROC SocketReceive_Direct()
      !            VAR string receivedData;
      !            VAR string expectedCmd;
      !            VAR num timeout;
      !            VAR clock timer;

      !            TEST cmdType
      !            CASE CMD_BEGIN_PROGRAM:
      !                  expectedCmd:="BEGIN_PROGRAM";
      !            CASE CMD_PROGRAM_ID:
      !                  expectedCmd:="PROGRAM_ID";
      !            CASE CMD_MATERIAL:
      !                  expectedCmd:="MATERIAL";
      !            CASE CMD_NOMINAL_WIDTH:
      !                  expectedCmd:="NOMINAL_WIDTH";
      !            CASE CMD_NUMBER_OF_TOWS:
      !                  expectedCmd:="NUMBER_OF_TOWS";
      !            CASE CMD_WATER_COOLING_TEMP:
      !                  expectedCmd:="WATER_COOLING_TEMP";
      !            CASE CMD_LAYER_THICKNESS:
      !                  expectedCmd:="LAYER_THICKNESS";
      !            CASE CMD_FIBER_ANGLE:
      !                  expectedCmd:="FIBER_ANGLE";
      !            CASE CMD_LAYER_NUMBER:
      !                  expectedCmd:="LAYER_NUMBER";
      !            CASE CMD_BEGIN_LAYER:
      !                  expectedCmd:="BEGIN_LAYER";
      !            CASE CMD_LAYER_ID:
      !                  expectedCmd:="LAYER_ID";
      !            CASE CMD_END_LAYER:
      !                  expectedCmd:="END_LAYER";
      !            CASE CMD_BEGIN_TRACE:
      !                  expectedCmd:="BEGIN_TRACE";
      !            CASE CMD_TRACE_ID:
      !                  expectedCmd:="TRACE_ID";
      !            CASE CMD_COMPACTION_PRESSURE:
      !                  expectedCmd:="COMPACTION_PRESSURE";
      !            CASE CMD_LASER_EMISSION:
      !                  expectedCmd:="LASER_EMISSION";
      !            CASE CMD_LASER_POWER:
      !                  expectedCmd:="LASER_POWER";
      !            CASE CMD_CUTTER_STATE:
      !                  expectedCmd:="CUTTER_STATE";
      !            CASE CMD_COMPACTION_ROLLER_STATE:
      !                  expectedCmd:="COMPACTION_ROLLER_STATE";
      !            CASE CMD_PINCH_ROLLER_STATE:
      !                  expectedCmd:="PINCH_ROLLER_STATE";
      !            CASE CMD_LAYING_SPEED:
      !                  expectedCmd:="LAYING_SPEED";
      !            CASE CMD_EXTERNAL_AXIS:
      !                  expectedCmd:="EXTERNAL_AXIS_DISPLACEMENT";
      !            CASE CMD_TOW_TENSION:
      !                  expectedCmd:="TOW_TENSION";
      !            CASE CMD_HEAD_ANGLE:
      !                  expectedCmd:="HEAD_ANGLE";
      !            CASE CMD_PRE_FEED_LENGTH:
      !                  expectedCmd:="PRE_FEED_LENGTH";
      !            CASE CMD_PRE_FEED_SPEED:
      !                  expectedCmd:="PRE_FEED_SPEED";
      !            CASE CMD_POST_FEED_LENGTH:
      !                  expectedCmd:="POST_FEED_LENGTH";
      !            CASE CMD_POST_FEED_SPEED:
      !                  expectedCmd:="POST_FEED_SPEED";
      !            CASE CMD_MOVEL_TO:
      !                  expectedCmd:="MOVEL_TO";
      !            CASE CMD_MOVEL:
      !                  expectedCmd:="MOVEL";
      !            CASE CMD_END_TRACE:
      !                  expectedCmd:="END_TRACE";
      !            CASE CMD_END_PROGRAM:
      !                  expectedCmd:="END_PROGRAM";
      !            DEFAULT:
      !                  expectedCmd:="UNKNOWN_CMD";
      !            ENDTEST

      !            timeout:=0;
      !            ClkStart timer;
      !            cmdVerified:=FALSE;

      !            WHILE NOT cmdVerified AND timeout<5 DO
      !                  !                  TPWrite "enter recevice while";
      !                  SocketReceive clientSocket_upload\Str:=receivedData\ReadNoOfBytes:=80;
      !                  IF receivedData<>"" THEN
      !                        !                        ackMessage:=receivedData;
      !                        IF StrFind(receivedData,1,expectedCmd)=2 THEN
      !                              TPWrite "Currect receive:"+receivedData+"  sendType:"+expectedCmd;
      !                              cmdVerified:=TRUE;
      !                        ELSE
      !                              TPWrite "False ACK: "+expectedCmd+" recevice: "+receivedData;
      !                              cmdVerified:=FALSE;
      !                        ENDIF
      !                  ENDIF
      !                  timeout:=ClkRead(timer);
      !            ENDWHILE
      !            TPWrite "jump out of receive";
      !            TPWrite "receive assignment cmdVerified:"\Bool:=cmdVerified;
      !            IF timeout>=5 THEN
      !                  TPWrite "TimeOut ACK("+NumToStr(timeout,1)+"s): "+expectedCmd;
      !                  cmdVerified:=FALSE;
      !            ENDIF

      !      ENDPROC

      PROC SocketReceive_Direct(string expectedCmd)
            VAR string receivedData;
            cmdWhileFlag:=False;
!            TPWrite "Enter loop and set cmdWhileFlag:"\Bool:=cmdWhileFlag;
            WHILE NOT cmdVerified DO
                  SocketReceive clientSocket_upload\Str:=receivedData\ReadNoOfBytes:=80;
                  !                  IF receivedData<>"" THEN
                  !                        ackMessage:=receivedData;
                  IF StrFind(receivedData,1,expectedCmd)=2 THEN
!                        TPWrite "Currect re:"+receivedData+" sTp:"+expectedCmd;
                        WHILE NOT cmdVerified DO
                              cmdVerified:=TRUE;
                        ENDWHILE
!                        TPWrite "set cmdVerified True:"\Bool:=cmdVerified;
                  ELSE
                        TPWrite "False re:"+receivedData+" sTp:"+expectedCmd;
                        cmdVerified:=FALSE;
                        TPWrite "set cmdVerified False:"\Bool:=cmdVerified;
                  ENDIF
                  !                  ENDIF
            ENDWHILE
            cmdWhileFlag:=TRUE;
!            TPWrite "Exit loop and set cmdWhileFlag:"\Bool:=cmdWhileFlag;
      ENDPROC

      ! ============================================================
      ! ???? - ????????
      ! ============================================================
      PROC ProcessCommand()
            VAR string msgToSend;
            VAR num localCmdType;
            VAR string localPayload;
            VAR num nval;
            VAR bool ok;

            localCmdType:=cmdType;
            localPayload:=cmdPayload;
            cmdReady:=FALSE;

            !            cmdSent:=FALSE;

!            TPWrite "ProcessCommand cmdType:"\Num:=localCmdType;
!            TPWrite "ProcessCommand cmdType:"\Num:=cmdType;
            TEST cmdType
            CASE CMD_BEGIN_PROGRAM:
                  msgToSend:="BEGIN_PROGRAM";
                  SocketSend clientSocket_upload\Str:="#"+msgToSend;
                  !TPWrite "send===============:"+msgToSend;
                  SocketReceive_Direct msgToSend;

            CASE CMD_PROGRAM_ID:
                  msgToSend:="PROGRAM_ID "+localPayload;
                  SocketSend clientSocket_upload\Str:="#"+msgToSend;
                  !TPWrite "send===============:"+msgToSend;
                  SocketReceive_Direct msgToSend;
            CASE CMD_MATERIAL:
                  !                  TPWrite "enter CMD_MATERTAL";
                  msgToSend:="MATERIAL "+localPayload;
                  SocketSend clientSocket_upload\Str:="#"+msgToSend;
                  !TPWrite "send===============:"+msgToSend;
                  SocketReceive_Direct msgToSend;
            CASE CMD_NOMINAL_WIDTH:
                  msgToSend:="NOMINAL_WIDTH "+localPayload;
                  SocketSend clientSocket_upload\Str:="#"+msgToSend;
                  !TPWrite "send===============:"+msgToSend;
                  SocketReceive_Direct msgToSend;
            CASE CMD_NUMBER_OF_TOWS:
                  msgToSend:="NUMBER_OF_TOWS "+localPayload;
                  SocketSend clientSocket_upload\Str:="#"+msgToSend;
                  !TPWrite "send===============:"+msgToSend;
                  SocketReceive_Direct msgToSend;
            CASE CMD_WATER_COOLING_TEMP:
                  msgToSend:="WATER_COOLING_TEMP "+localPayload;
                  SocketSend clientSocket_upload\Str:="#"+msgToSend;
                  !TPWrite "send===============:"+msgToSend;
                  SocketReceive_Direct msgToSend;
            CASE CMD_LAYER_THICKNESS:
                  msgToSend:="LAYER_THICKNESS "+localPayload;
                  SocketSend clientSocket_upload\Str:="#"+msgToSend;
                  !TPWrite "send===============:"+msgToSend;
                  SocketReceive_Direct msgToSend;
            CASE CMD_FIBER_ANGLE:
                  msgToSend:="FIBER_ANGLE "+localPayload;
                  SocketSend clientSocket_upload\Str:="#"+msgToSend;
                  !TPWrite "send===============:"+msgToSend;
                  SocketReceive_Direct msgToSend;
            CASE CMD_LAYER_NUMBER:
                  msgToSend:="LAYER_NUMBER "+localPayload;
                  SocketSend clientSocket_upload\Str:="#"+msgToSend;
                  !TPWrite "send===============:"+msgToSend;
                  SocketReceive_Direct msgToSend;
            CASE CMD_BEGIN_LAYER:
                  msgToSend:="BEGIN_LAYER";
                  SocketSend clientSocket_upload\Str:="#"+msgToSend;
                  !TPWrite "send===============:"+msgToSend;
                  SocketReceive_Direct msgToSend;
            CASE CMD_LAYER_ID:
                  msgToSend:="LAYER_ID "+localPayload;
                  SocketSend clientSocket_upload\Str:="#"+msgToSend;
                  !TPWrite "send===============:"+msgToSend;
                  SocketReceive_Direct msgToSend;
            CASE CMD_END_LAYER:
                  msgToSend:="END_LAYER";
                  SocketSend clientSocket_upload\Str:="#"+msgToSend;
                  !TPWrite "send===============:"+msgToSend;
                  SocketReceive_Direct msgToSend;
            CASE CMD_BEGIN_TRACE:
                  msgToSend:="BEGIN_TRACE";
                  SocketSend clientSocket_upload\Str:="#"+msgToSend;
                  !TPWrite "send===============:"+msgToSend;
                  SocketReceive_Direct msgToSend;
            CASE CMD_TRACE_ID:
                  msgToSend:="TRACE_ID "+localPayload;
                  SocketSend clientSocket_upload\Str:="#"+msgToSend;
                  !TPWrite "send===============:"+msgToSend;
                  SocketReceive_Direct msgToSend;
            CASE CMD_COMPACTION_PRESSURE:
                  msgToSend:="COMPACTION_PRESSURE "+localPayload;
                  SocketSend clientSocket_upload\Str:="#"+msgToSend;
                  !TPWrite "send===============:"+msgToSend;
                  SocketReceive_Direct msgToSend;
            CASE CMD_LASER_EMISSION:
                  msgToSend:="LASER_EMISSION "+localPayload;
                  SocketSend clientSocket_upload\Str:="#"+msgToSend;
                  !TPWrite "send===============:"+msgToSend;
                  SocketReceive_Direct msgToSend;
            CASE CMD_LASER_POWER:
                  ok := StrToVal(localPayload,nval);
                  IF NOT ok THEN
                        nval := 0;
                  ENDIF
                  plc_laser_power nval;
            CASE CMD_CUTTER_STATE:
                  ok := StrToVal(localPayload,nval);
                  IF NOT ok THEN
                        nval := 0;
                  ENDIF
                  write_plc_mb143_bit PLC_BIT_CUTTER,nval;
            CASE CMD_COMPACTION_ROLLER_STATE:
                  msgToSend:="COMPACTION_ROLLER_STATE "+localPayload;
                  SocketSend clientSocket_upload\Str:="#"+msgToSend;
                  !TPWrite "send===============:"+msgToSend;
                  SocketReceive_Direct msgToSend;
            CASE CMD_PINCH_ROLLER_STATE:
                  ok := StrToVal(localPayload,nval);
                  IF NOT ok THEN
                        nval := 0;
                  ENDIF
                  write_plc_mb143_bit PLC_BIT_PUSH,nval;
            CASE CMD_LAYING_SPEED:
                  msgToSend:="LAYING_SPEED "+localPayload;
                  SocketSend clientSocket_upload\Str:="#"+msgToSend;
                  !TPWrite "send===============:"+msgToSend;
                  SocketReceive_Direct msgToSend;
            CASE CMD_EXTERNAL_AXIS:
                  msgToSend:="EXTERNAL_AXIS_DISPLACEMENT "+localPayload;
                  SocketSend clientSocket_upload\Str:="#"+msgToSend;
                  !TPWrite "send===============:"+msgToSend;
                  SocketReceive_Direct msgToSend;
            CASE CMD_TOW_TENSION:
                  msgToSend:="TOW_TENSION "+localPayload;
                  SocketSend clientSocket_upload\Str:="#"+msgToSend;
                  !TPWrite "send===============:"+msgToSend;
                  SocketReceive_Direct msgToSend;
            CASE CMD_HEAD_ANGLE:
                  msgToSend:="HEAD_ANGLE "+localPayload;
                  SocketSend clientSocket_upload\Str:="#"+msgToSend;
                  !TPWrite "send===============:"+msgToSend;
                  SocketReceive_Direct msgToSend;
            CASE CMD_FEED_LENGTH:
                  msgToSend:="FEED_LENGTH "+localPayload;
                  SocketSend clientSocket_upload\Str:="#"+msgToSend;
                  !TPWrite "send===============:"+msgToSend;
                  SocketReceive_Direct msgToSend;

            CASE CMD_FEED_SPEED:
                  msgToSend:="FEED_SPEED "+localPayload;
                  SocketSend clientSocket_upload\Str:="#"+msgToSend;
                  !TPWrite "send===============:"+msgToSend;
                  SocketReceive_Direct msgToSend;
            CASE CMD_MOVEL:

            CASE CMD_PLC_FEED_SPEED:
                  ok := StrToVal(localPayload,nval);
                  IF NOT ok THEN
                        nval := 0;
                  ENDIF
                  plc_motor_speed nval;
            CASE CMD_END_TRACE:
                  msgToSend:="END_TRACE";
                  SocketSend clientSocket_upload\Str:="#"+msgToSend;
                  !TPWrite "send===============:"+msgToSend;
                  SocketReceive_Direct msgToSend;
            DEFAULT:
                  msgToSend:="UNKNOWN_CMD";
                  SocketSend clientSocket_upload\Str:="#"+msgToSend;
                  !TPWrite "send===============:"+msgToSend;
                  SocketReceive_Direct msgToSend;
            ENDTEST
            !            TPWrite "already socketsend:"+msgToSend;

            cmdSent:=TRUE;
            !            cmdType:=0;
            !            cmdPayload:="";
            !            cmdReady:=FALSE;
      ENDPROC

      PROC SendRobotMonitorMessage()
            VAR robtarget monitor_pose;
            VAR string monitor_msg;

            IF monitor_enabled=FALSE THEN
                  RETURN;
            ENDIF

            IF SocketGetStatus(clientSocket_upload)<>SOCKET_CONNECTED THEN
                  ReconnectUploadClient;
            ENDIF

            ! Read the current TCP position in the same tool/workobject frame
            ! used by the motion task. This first monitor frame is deliberately
            ! compact so it stays practical for RAPID socket strings.
            monitor_pose:=CRobT(\Tool:=currentTool\WObj:=currentWobj);
            monitor_seq:=monitor_seq+1;

            monitor_msg:="ROBOT_MONITOR s="+NumToStr(monitor_seq,0);
            monitor_msg:=monitor_msg+" x="+NumToStr(monitor_pose.trans.x,1);
            monitor_msg:=monitor_msg+" y="+NumToStr(monitor_pose.trans.y,1);
            monitor_msg:=monitor_msg+" z="+NumToStr(monitor_pose.trans.z,1);
            monitor_msg:=monitor_msg+" v="+NumToStr(currentSpeed.v_tcp,1);

            ! The surrounding # characters are frame delimiters used by the C++
            ! RobotMonitorClient to separate messages in the TCP byte stream.
            SocketSend clientSocket_upload\Str:="#"+monitor_msg+"#";
      ERROR
            TEST ERRNO
            CASE ERR_SOCK_CLOSED:
                  TPWrite "UPLOAD: Monitor socket closed during send. Waiting for reconnection ...";
                  ReconnectUploadClient;
                  ! The failed monitor sample can be skipped. The next main
                  ! loop cycle will build and send a fresh sample.
                  TRYNEXT;
            DEFAULT:
                  TPWrite "UPLOAD: Monitor send failed. Waiting for reconnection ...";
                  ReconnectUploadClient;
                  ! Treat other recoverable send errors the same way: rebuild
                  ! the connection and skip only the failed sample.
                  TRYNEXT;
            ENDTEST
      ENDPROC

      PROC main()
            !      TPWrite "test rob 3";
            VAR string data;
            VAR robtarget position;
            VAR jointtarget joints;
            VAR string sendString;

            VAR string date;
            VAR string time;
            VAR clock timer;

            date:=CDate();
            time:=CTime();
            ClkStart timer;

            connected:=FALSE;
            ServerCreateAndConnect_3 ipController,UPLOADPort;
            connected:=TRUE;

            !            main_load;


            ! ????????
            TPWrite "Server task started - monitoring commands...";

            WHILE TRUE DO
                  !                  TPWrite "enter robot3 while first layer,cmdReady:"\Bool:=cmdReady;
                  IF cmdReady=TRUE THEN
                        !                        TPWrite "enter robot3 while second layer";
                        ProcessCommand;
                  ENDIF
                  SendRobotMonitorMessage;
                  CheckMonitorHeartbeat;
                  WaitTime monitor_wait_time;
            ENDWHILE






            !	ERROR
            !	IF ERRNO=ERR_SOCK_CLOSED THEN
            !		TPWrite "upload: Client has closed connection.";
            !	ELSE
            !		TPWrite "upload: Connection lost: Unknown problem.";
            !	ENDIF
            !	connected:=FALSE;
            !	!Closing the server
            !	SocketClose clientSocket_upload;
            !	SocketClose serverSocket_upload;
            !	!Reinitiate the server
            !	ServerCreateAndConnect_3 ipController,monitorPort;
            !	connected:= TRUE;
            !	RETRY;
      ENDPROC

ENDMODULE
