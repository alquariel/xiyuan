MODULE LOGGER

!////////////////
!GLOBAL VARIABLES
!////////////////
!PC communication
VAR socketdev clientSocket;
VAR socketdev serverSocket;
PERS string ipController := "127.0.0.1";
PERS num loggerPort:= 5001;

!Robot configuration	
! Mirrors currentTool / currentWobj from SERVER.mod so CRobT works after a
! full controller power cycle (PERS vars default to their type zero-value).
PERS tooldata currentTool := [TRUE,[[338.402,18.0101,-541.55],[0.997552,0.00335155,6.229E-6,-0.0698543]],[0.001,[0,0,0.001],[1,0,0,0],0,0,0]];    
PERS wobjdata currentWobj := [FALSE,TRUE,"",[[734.201,20.8803,828.431],[0.713319,0.00336855,-0.00319022,-0.700824]],[[0,0,0],[1,0,0,0]]];
VAR speeddata currentSpeed;
VAR zonedata currentZone;

!//Logger sampling rate
!PERS num loggerWaitTime:= 0.01;  !Recommended for real controller
PERS num loggerWaitTime:= 0.1;    !Recommended for virtual controller

PROC ServerCreateAndConnect(string ip, num port)
	VAR string clientIP;
	
	! ???? / ????? serverSocket ?????? socket,
	! ????????? LISTENING,?? SocketAccept ?? "????????"
	IF SocketGetStatus(serverSocket) <> SOCKET_LISTENING THEN
		SocketCreate serverSocket;
		SocketBind serverSocket, ip, port;
		SocketListen serverSocket;
	ENDIF

	TPWrite "LOGGER: Logger waiting for incomming connections ...";
	WHILE SocketGetStatus(clientSocket) <> SOCKET_CONNECTED DO
		! ?? Accept ???????,????? SocketClose ???????
		IF SocketGetStatus(serverSocket) <> SOCKET_LISTENING THEN
			SocketCreate serverSocket;
			SocketBind serverSocket, ip, port;
			SocketListen serverSocket;
		ENDIF
		SocketAccept serverSocket,clientSocket \ClientAddress:=clientIP \Time:=WAIT_MAX;
		IF SocketGetStatus(clientSocket) <> SOCKET_CONNECTED THEN
			TPWrite "LOGGER: Problem serving an incomming connection.";
			TPWrite "LOGGER: Try reconnecting.";
		ENDIF
		 !Wait 0.5 seconds for the next reconnection
		 WaitTime 0.5;
	ENDWHILE
	TPWrite "LOGGER: Connected to IP " + clientIP;
ENDPROC

PROC main()
	VAR string data;
	VAR robtarget position;
	VAR jointtarget joints;
    VAR string sendString;
	VAR bool connected;

	VAR string date;
	VAR string time;
	VAR clock timer;

	date:= CDate();
	time:= CTime();
    ClkStart timer;
    
	connected:=FALSE;
	ServerCreateAndConnect ipController,loggerPort;	
	connected:=TRUE;
	WHILE TRUE DO
		
		!Cartesian Coordinates
		position := CRobT(\Tool:=currentTool \WObj:=currentWObj);
		data := "# 0 ";
		data := data + date + " " + time + " ";
		data := data + NumToStr(ClkRead(timer),2) + " ";
		data := data + NumToStr(position.trans.x,1) + " ";
		data := data + NumToStr(position.trans.y,1) + " ";
		data := data + NumToStr(position.trans.z,1) + " ";
		data := data + NumToStr(position.rot.q1,3) + " ";
		data := data + NumToStr(position.rot.q2,3) + " ";
		data := data + NumToStr(position.rot.q3,3) + " ";
		data := data + NumToStr(position.rot.q4,3); !End of string	
		IF connected = TRUE THEN
			SocketSend clientSocket \Str:=data;
		ENDIF
		WaitTime loggerWaitTime;
	
		!Joint Coordinates
		joints := CJointT();
		data := "# 1 ";
		data := data + date + " " + time + " ";
		data := data + NumToStr(ClkRead(timer),2) + " ";
		data := data + NumToStr(joints.robax.rax_1,2) + " ";
		data := data + NumToStr(joints.robax.rax_2,2) + " ";
		data := data + NumToStr(joints.robax.rax_3,2) + " ";
		data := data + NumToStr(joints.robax.rax_4,2) + " ";
		data := data + NumToStr(joints.robax.rax_5,2) + " ";
		data := data + NumToStr(joints.robax.rax_6,2); !End of string
		IF connected = TRUE THEN
			SocketSend clientSocket \Str:=data;
		ENDIF
		WaitTime loggerWaitTime;
	ENDWHILE
	ERROR
	IF ERRNO=ERR_SOCK_CLOSED THEN
		TPWrite "LOGGER: Client has closed connection.";
	ELSE
		TPWrite "LOGGER: Connection lost: Unknown problem.";
	ENDIF
	connected:=FALSE;
	!Closing the server
	SocketClose clientSocket;
	SocketClose serverSocket;
	!Reinitiate the server
	ServerCreateAndConnect ipController,loggerPort;
	connected:= TRUE;
	RETRY;
ENDPROC

ENDMODULE