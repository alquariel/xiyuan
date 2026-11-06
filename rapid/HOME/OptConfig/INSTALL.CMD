echo -text "AW configuration installation start ..."
#
# create environment variable
#setenv -Name "FUNCPACK" -value $BOOTPATH
## 
setstr -strvar $TMP_ARC -value $BOOTPATH
setstr -strvar $BOOTPATH -value "$HOME/OptConfig"
# 

# 
# Make required directories
mkdir -path $HOME/ApplData
mkdir -path $HOME/ApplConf
mkdir -path $HOME/ApplSys
#
# Register path for FlexPendant dlls, images etc.
#
# The contents of the following folder will be copied
# to the FlexPendant unit during every FlexPendant restart.
register -type option -description fpdefault -path $BOOTPATH/FP/default
#
mkdir -path $HOME/ApplData/ROB1
mkdir -path $HOME/ApplConf/ROB1
mkdir -path $HOME/ApplSys/ROB1
# 
echo -text "Installing Customer built A based on MU"
# 
config -filename $BOOTPATH/MOC/MOC_MU400MU400MU300_M7M8M9DM1_L2B1N1_AS_TRACKSTN1.cfg -domain MOC -replace
# 

# 
setstr -strvar $BOOTPATH -value "$TMP_ARC"
# 