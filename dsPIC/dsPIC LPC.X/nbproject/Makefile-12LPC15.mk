#
# Generated Makefile - do not edit!
#
# Edit the Makefile in the project folder instead (../Makefile). Each target
# has a -pre and a -post target defined where you can add customized code.
#
# This makefile implements configuration specific macros and targets.


# Include project Makefile
ifeq "${IGNORE_LOCAL}" "TRUE"
# do not include local makefile. User is passing all local related variables already
else
include Makefile
# Include makefile containing local settings
ifeq "$(wildcard nbproject/Makefile-local-12LPC15.mk)" "nbproject/Makefile-local-12LPC15.mk"
include nbproject/Makefile-local-12LPC15.mk
endif
endif

# Environment
MKDIR=gnumkdir -p
RM=rm -f 
MV=mv 
CP=cp 

# Macros
CND_CONF=12LPC15
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
IMAGE_TYPE=debug
OUTPUT_SUFFIX=elf
DEBUGGABLE_SUFFIX=elf
FINAL_IMAGE=dist/${CND_CONF}/${IMAGE_TYPE}/dsPIC_LPC.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}
else
IMAGE_TYPE=production
OUTPUT_SUFFIX=hex
DEBUGGABLE_SUFFIX=elf
FINAL_IMAGE=dist/${CND_CONF}/${IMAGE_TYPE}/dsPIC_LPC.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}
endif

ifeq ($(COMPARE_BUILD), true)
COMPARISON_BUILD=-mafrlcsj
else
COMPARISON_BUILD=
endif

ifdef SUB_IMAGE_ADDRESS
SUB_IMAGE_ADDRESS_COMMAND=--image-address $(SUB_IMAGE_ADDRESS)
else
SUB_IMAGE_ADDRESS_COMMAND=
endif

# Object Directory
OBJECTDIR=build/${CND_CONF}/${IMAGE_TYPE}

# Distribution Directory
DISTDIR=dist/${CND_CONF}/${IMAGE_TYPE}

# Source Files Quoted if spaced
SOURCEFILES_QUOTED_IF_SPACED=../src/inverter.c ../src/main.c ../src/ui.c ../src/task_dev.c ../src/task_temp.c ../src/task_main.c ../src/common/analog.c ../src/common/analog_dsPIC33F.c ../src/common/batt_temp.c ../src/common/CAN/dsPIC33_CAN.c ../src/common/CAN/J1939.c ../src/common/CAN/rv_can.c ../src/common/CAN/sensata_can.c ../src/common/charger.c ../src/common/charger_3step.c ../src/common/charger_cmds.c ../src/common/charger_isr.c ../src/common/charger_liion.c ../src/common/config.c ../src/common/converter_cmds.c ../src/common/dac.c ../src/common/dsPIC_serial.c ../src/common/hs_temp.c ../src/common/inverter_cmds.c ../src/common/inv_check_supply.c ../src/common/itoa.c ../src/common/log.c ../src/common/nvm.c ../src/common/options.c ../src/common/pwm.c ../src/common/rom.c ../src/common/signal_capture.c ../src/common/sine_table.c ../src/common/spi.c ../src/common/sqrt.c ../src/common/ssr.c ../src/common/tasker.c ../src/common/timer1.c ../src/common/timer3.c ../src/common/traps.c ../src/common/fan_ctrl_lpc.c ../src/common/getErrLoc.s

# Object Files Quoted if spaced
OBJECTFILES_QUOTED_IF_SPACED=${OBJECTDIR}/_ext/1360937237/inverter.o ${OBJECTDIR}/_ext/1360937237/main.o ${OBJECTDIR}/_ext/1360937237/ui.o ${OBJECTDIR}/_ext/1360937237/task_dev.o ${OBJECTDIR}/_ext/1360937237/task_temp.o ${OBJECTDIR}/_ext/1360937237/task_main.o ${OBJECTDIR}/_ext/394045403/analog.o ${OBJECTDIR}/_ext/394045403/analog_dsPIC33F.o ${OBJECTDIR}/_ext/394045403/batt_temp.o ${OBJECTDIR}/_ext/919134522/dsPIC33_CAN.o ${OBJECTDIR}/_ext/919134522/J1939.o ${OBJECTDIR}/_ext/919134522/rv_can.o ${OBJECTDIR}/_ext/919134522/sensata_can.o ${OBJECTDIR}/_ext/394045403/charger.o ${OBJECTDIR}/_ext/394045403/charger_3step.o ${OBJECTDIR}/_ext/394045403/charger_cmds.o ${OBJECTDIR}/_ext/394045403/charger_isr.o ${OBJECTDIR}/_ext/394045403/charger_liion.o ${OBJECTDIR}/_ext/394045403/config.o ${OBJECTDIR}/_ext/394045403/converter_cmds.o ${OBJECTDIR}/_ext/394045403/dac.o ${OBJECTDIR}/_ext/394045403/dsPIC_serial.o ${OBJECTDIR}/_ext/394045403/hs_temp.o ${OBJECTDIR}/_ext/394045403/inverter_cmds.o ${OBJECTDIR}/_ext/394045403/inv_check_supply.o ${OBJECTDIR}/_ext/394045403/itoa.o ${OBJECTDIR}/_ext/394045403/log.o ${OBJECTDIR}/_ext/394045403/nvm.o ${OBJECTDIR}/_ext/394045403/options.o ${OBJECTDIR}/_ext/394045403/pwm.o ${OBJECTDIR}/_ext/394045403/rom.o ${OBJECTDIR}/_ext/394045403/signal_capture.o ${OBJECTDIR}/_ext/394045403/sine_table.o ${OBJECTDIR}/_ext/394045403/spi.o ${OBJECTDIR}/_ext/394045403/sqrt.o ${OBJECTDIR}/_ext/394045403/ssr.o ${OBJECTDIR}/_ext/394045403/tasker.o ${OBJECTDIR}/_ext/394045403/timer1.o ${OBJECTDIR}/_ext/394045403/timer3.o ${OBJECTDIR}/_ext/394045403/traps.o ${OBJECTDIR}/_ext/394045403/fan_ctrl_lpc.o ${OBJECTDIR}/_ext/394045403/getErrLoc.o
POSSIBLE_DEPFILES=${OBJECTDIR}/_ext/1360937237/inverter.o.d ${OBJECTDIR}/_ext/1360937237/main.o.d ${OBJECTDIR}/_ext/1360937237/ui.o.d ${OBJECTDIR}/_ext/1360937237/task_dev.o.d ${OBJECTDIR}/_ext/1360937237/task_temp.o.d ${OBJECTDIR}/_ext/1360937237/task_main.o.d ${OBJECTDIR}/_ext/394045403/analog.o.d ${OBJECTDIR}/_ext/394045403/analog_dsPIC33F.o.d ${OBJECTDIR}/_ext/394045403/batt_temp.o.d ${OBJECTDIR}/_ext/919134522/dsPIC33_CAN.o.d ${OBJECTDIR}/_ext/919134522/J1939.o.d ${OBJECTDIR}/_ext/919134522/rv_can.o.d ${OBJECTDIR}/_ext/919134522/sensata_can.o.d ${OBJECTDIR}/_ext/394045403/charger.o.d ${OBJECTDIR}/_ext/394045403/charger_3step.o.d ${OBJECTDIR}/_ext/394045403/charger_cmds.o.d ${OBJECTDIR}/_ext/394045403/charger_isr.o.d ${OBJECTDIR}/_ext/394045403/charger_liion.o.d ${OBJECTDIR}/_ext/394045403/config.o.d ${OBJECTDIR}/_ext/394045403/converter_cmds.o.d ${OBJECTDIR}/_ext/394045403/dac.o.d ${OBJECTDIR}/_ext/394045403/dsPIC_serial.o.d ${OBJECTDIR}/_ext/394045403/hs_temp.o.d ${OBJECTDIR}/_ext/394045403/inverter_cmds.o.d ${OBJECTDIR}/_ext/394045403/inv_check_supply.o.d ${OBJECTDIR}/_ext/394045403/itoa.o.d ${OBJECTDIR}/_ext/394045403/log.o.d ${OBJECTDIR}/_ext/394045403/nvm.o.d ${OBJECTDIR}/_ext/394045403/options.o.d ${OBJECTDIR}/_ext/394045403/pwm.o.d ${OBJECTDIR}/_ext/394045403/rom.o.d ${OBJECTDIR}/_ext/394045403/signal_capture.o.d ${OBJECTDIR}/_ext/394045403/sine_table.o.d ${OBJECTDIR}/_ext/394045403/spi.o.d ${OBJECTDIR}/_ext/394045403/sqrt.o.d ${OBJECTDIR}/_ext/394045403/ssr.o.d ${OBJECTDIR}/_ext/394045403/tasker.o.d ${OBJECTDIR}/_ext/394045403/timer1.o.d ${OBJECTDIR}/_ext/394045403/timer3.o.d ${OBJECTDIR}/_ext/394045403/traps.o.d ${OBJECTDIR}/_ext/394045403/fan_ctrl_lpc.o.d ${OBJECTDIR}/_ext/394045403/getErrLoc.o.d

# Object Files
OBJECTFILES=${OBJECTDIR}/_ext/1360937237/inverter.o ${OBJECTDIR}/_ext/1360937237/main.o ${OBJECTDIR}/_ext/1360937237/ui.o ${OBJECTDIR}/_ext/1360937237/task_dev.o ${OBJECTDIR}/_ext/1360937237/task_temp.o ${OBJECTDIR}/_ext/1360937237/task_main.o ${OBJECTDIR}/_ext/394045403/analog.o ${OBJECTDIR}/_ext/394045403/analog_dsPIC33F.o ${OBJECTDIR}/_ext/394045403/batt_temp.o ${OBJECTDIR}/_ext/919134522/dsPIC33_CAN.o ${OBJECTDIR}/_ext/919134522/J1939.o ${OBJECTDIR}/_ext/919134522/rv_can.o ${OBJECTDIR}/_ext/919134522/sensata_can.o ${OBJECTDIR}/_ext/394045403/charger.o ${OBJECTDIR}/_ext/394045403/charger_3step.o ${OBJECTDIR}/_ext/394045403/charger_cmds.o ${OBJECTDIR}/_ext/394045403/charger_isr.o ${OBJECTDIR}/_ext/394045403/charger_liion.o ${OBJECTDIR}/_ext/394045403/config.o ${OBJECTDIR}/_ext/394045403/converter_cmds.o ${OBJECTDIR}/_ext/394045403/dac.o ${OBJECTDIR}/_ext/394045403/dsPIC_serial.o ${OBJECTDIR}/_ext/394045403/hs_temp.o ${OBJECTDIR}/_ext/394045403/inverter_cmds.o ${OBJECTDIR}/_ext/394045403/inv_check_supply.o ${OBJECTDIR}/_ext/394045403/itoa.o ${OBJECTDIR}/_ext/394045403/log.o ${OBJECTDIR}/_ext/394045403/nvm.o ${OBJECTDIR}/_ext/394045403/options.o ${OBJECTDIR}/_ext/394045403/pwm.o ${OBJECTDIR}/_ext/394045403/rom.o ${OBJECTDIR}/_ext/394045403/signal_capture.o ${OBJECTDIR}/_ext/394045403/sine_table.o ${OBJECTDIR}/_ext/394045403/spi.o ${OBJECTDIR}/_ext/394045403/sqrt.o ${OBJECTDIR}/_ext/394045403/ssr.o ${OBJECTDIR}/_ext/394045403/tasker.o ${OBJECTDIR}/_ext/394045403/timer1.o ${OBJECTDIR}/_ext/394045403/timer3.o ${OBJECTDIR}/_ext/394045403/traps.o ${OBJECTDIR}/_ext/394045403/fan_ctrl_lpc.o ${OBJECTDIR}/_ext/394045403/getErrLoc.o

# Source Files
SOURCEFILES=../src/inverter.c ../src/main.c ../src/ui.c ../src/task_dev.c ../src/task_temp.c ../src/task_main.c ../src/common/analog.c ../src/common/analog_dsPIC33F.c ../src/common/batt_temp.c ../src/common/CAN/dsPIC33_CAN.c ../src/common/CAN/J1939.c ../src/common/CAN/rv_can.c ../src/common/CAN/sensata_can.c ../src/common/charger.c ../src/common/charger_3step.c ../src/common/charger_cmds.c ../src/common/charger_isr.c ../src/common/charger_liion.c ../src/common/config.c ../src/common/converter_cmds.c ../src/common/dac.c ../src/common/dsPIC_serial.c ../src/common/hs_temp.c ../src/common/inverter_cmds.c ../src/common/inv_check_supply.c ../src/common/itoa.c ../src/common/log.c ../src/common/nvm.c ../src/common/options.c ../src/common/pwm.c ../src/common/rom.c ../src/common/signal_capture.c ../src/common/sine_table.c ../src/common/spi.c ../src/common/sqrt.c ../src/common/ssr.c ../src/common/tasker.c ../src/common/timer1.c ../src/common/timer3.c ../src/common/traps.c ../src/common/fan_ctrl_lpc.c ../src/common/getErrLoc.s


CFLAGS=
ASFLAGS=
LDLIBSOPTIONS=

############# Tool locations ##########################################
# If you copy a project from one host to another, the path where the  #
# compiler is installed may be different.                             #
# If you open this project with MPLAB X in the new host, this         #
# makefile will be regenerated and the paths will be corrected.       #
#######################################################################
# fixDeps replaces a bunch of sed/cat/printf statements that slow down the build
FIXDEPS=fixDeps

# The following macros may be used in the pre and post step lines
Device=dsPIC33FJ128MC706A
ProjectDir="C:\Users\A1021170\Projects\sandbox\LPC\dsPIC\dsPIC LPC.X"
ConfName=12LPC15
ImagePath="dist\12LPC15\${IMAGE_TYPE}\dsPIC_LPC.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}"
ImageDir="dist\12LPC15\${IMAGE_TYPE}"
ImageName="dsPIC_LPC.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}"
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
IsDebug="true"
else
IsDebug="false"
endif

.build-conf:  ${BUILD_SUBPROJECTS}
ifneq ($(INFORMATION_MESSAGE), )
	@echo $(INFORMATION_MESSAGE)
endif
	${MAKE}  -f nbproject/Makefile-12LPC15.mk dist/${CND_CONF}/${IMAGE_TYPE}/dsPIC_LPC.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}
	@echo "--------------------------------------"
	@echo "User defined post-build step: [copy ${ImagePath} "..\..\..\${ConfName}.${OUTPUT_SUFFIX}"]"
	@copy ${ImagePath} "..\..\..\${ConfName}.${OUTPUT_SUFFIX}"
	@echo "--------------------------------------"

MP_PROCESSOR_OPTION=33FJ128MC706A
MP_LINKER_FILE_OPTION=,--script=p33FJ128MC706A.gld
# ------------------------------------------------------------------------------------
# Rules for buildStep: compile
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
${OBJECTDIR}/_ext/1360937237/inverter.o: ../src/inverter.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1360937237" 
	@${RM} ${OBJECTDIR}/_ext/1360937237/inverter.o.d 
	@${RM} ${OBJECTDIR}/_ext/1360937237/inverter.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../src/inverter.c  -o ${OBJECTDIR}/_ext/1360937237/inverter.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/1360937237/inverter.o.d"      -g -D__DEBUG -D__MPLAB_DEBUGGER_PK3=1    -omf=elf -DMODEL_12LPC15_FW0058 -DXPRJ_12LPC15=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O0 -I"../src" -I"../src/common" -I"../src/common/Cals" -I"../src/common/CAN" -I"../src/common/Models" -DMODEL_12LPC15 -msmart-io=1 -Wall -msfr-warn=off -finline 
	@${FIXDEPS} "${OBJECTDIR}/_ext/1360937237/inverter.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/1360937237/main.o: ../src/main.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1360937237" 
	@${RM} ${OBJECTDIR}/_ext/1360937237/main.o.d 
	@${RM} ${OBJECTDIR}/_ext/1360937237/main.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../src/main.c  -o ${OBJECTDIR}/_ext/1360937237/main.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/1360937237/main.o.d"      -g -D__DEBUG -D__MPLAB_DEBUGGER_PK3=1    -omf=elf -DMODEL_12LPC15_FW0058 -DXPRJ_12LPC15=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O0 -I"../src" -I"../src/common" -I"../src/common/Cals" -I"../src/common/CAN" -I"../src/common/Models" -DMODEL_12LPC15 -msmart-io=1 -Wall -msfr-warn=off -finline 
	@${FIXDEPS} "${OBJECTDIR}/_ext/1360937237/main.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/1360937237/ui.o: ../src/ui.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1360937237" 
	@${RM} ${OBJECTDIR}/_ext/1360937237/ui.o.d 
	@${RM} ${OBJECTDIR}/_ext/1360937237/ui.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../src/ui.c  -o ${OBJECTDIR}/_ext/1360937237/ui.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/1360937237/ui.o.d"      -g -D__DEBUG -D__MPLAB_DEBUGGER_PK3=1    -omf=elf -DMODEL_12LPC15_FW0058 -DXPRJ_12LPC15=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O0 -I"../src" -I"../src/common" -I"../src/common/Cals" -I"../src/common/CAN" -I"../src/common/Models" -DMODEL_12LPC15 -msmart-io=1 -Wall -msfr-warn=off -finline 
	@${FIXDEPS} "${OBJECTDIR}/_ext/1360937237/ui.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/1360937237/task_dev.o: ../src/task_dev.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1360937237" 
	@${RM} ${OBJECTDIR}/_ext/1360937237/task_dev.o.d 
	@${RM} ${OBJECTDIR}/_ext/1360937237/task_dev.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../src/task_dev.c  -o ${OBJECTDIR}/_ext/1360937237/task_dev.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/1360937237/task_dev.o.d"      -g -D__DEBUG -D__MPLAB_DEBUGGER_PK3=1    -omf=elf -DMODEL_12LPC15_FW0058 -DXPRJ_12LPC15=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O0 -I"../src" -I"../src/common" -I"../src/common/Cals" -I"../src/common/CAN" -I"../src/common/Models" -DMODEL_12LPC15 -msmart-io=1 -Wall -msfr-warn=off -finline 
	@${FIXDEPS} "${OBJECTDIR}/_ext/1360937237/task_dev.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/1360937237/task_temp.o: ../src/task_temp.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1360937237" 
	@${RM} ${OBJECTDIR}/_ext/1360937237/task_temp.o.d 
	@${RM} ${OBJECTDIR}/_ext/1360937237/task_temp.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../src/task_temp.c  -o ${OBJECTDIR}/_ext/1360937237/task_temp.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/1360937237/task_temp.o.d"      -g -D__DEBUG -D__MPLAB_DEBUGGER_PK3=1    -omf=elf -DMODEL_12LPC15_FW0058 -DXPRJ_12LPC15=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O0 -I"../src" -I"../src/common" -I"../src/common/Cals" -I"../src/common/CAN" -I"../src/common/Models" -DMODEL_12LPC15 -msmart-io=1 -Wall -msfr-warn=off -finline 
	@${FIXDEPS} "${OBJECTDIR}/_ext/1360937237/task_temp.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/1360937237/task_main.o: ../src/task_main.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1360937237" 
	@${RM} ${OBJECTDIR}/_ext/1360937237/task_main.o.d 
	@${RM} ${OBJECTDIR}/_ext/1360937237/task_main.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../src/task_main.c  -o ${OBJECTDIR}/_ext/1360937237/task_main.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/1360937237/task_main.o.d"      -g -D__DEBUG -D__MPLAB_DEBUGGER_PK3=1    -omf=elf -DMODEL_12LPC15_FW0058 -DXPRJ_12LPC15=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O0 -I"../src" -I"../src/common" -I"../src/common/Cals" -I"../src/common/CAN" -I"../src/common/Models" -DMODEL_12LPC15 -msmart-io=1 -Wall -msfr-warn=off -finline 
	@${FIXDEPS} "${OBJECTDIR}/_ext/1360937237/task_main.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/394045403/analog.o: ../src/common/analog.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/394045403" 
	@${RM} ${OBJECTDIR}/_ext/394045403/analog.o.d 
	@${RM} ${OBJECTDIR}/_ext/394045403/analog.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../src/common/analog.c  -o ${OBJECTDIR}/_ext/394045403/analog.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/394045403/analog.o.d"      -g -D__DEBUG -D__MPLAB_DEBUGGER_PK3=1    -omf=elf -DMODEL_12LPC15_FW0058 -DXPRJ_12LPC15=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O0 -I"../src" -I"../src/common" -I"../src/common/Cals" -I"../src/common/CAN" -I"../src/common/Models" -DMODEL_12LPC15 -msmart-io=1 -Wall -msfr-warn=off -finline 
	@${FIXDEPS} "${OBJECTDIR}/_ext/394045403/analog.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/394045403/analog_dsPIC33F.o: ../src/common/analog_dsPIC33F.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/394045403" 
	@${RM} ${OBJECTDIR}/_ext/394045403/analog_dsPIC33F.o.d 
	@${RM} ${OBJECTDIR}/_ext/394045403/analog_dsPIC33F.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../src/common/analog_dsPIC33F.c  -o ${OBJECTDIR}/_ext/394045403/analog_dsPIC33F.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/394045403/analog_dsPIC33F.o.d"      -g -D__DEBUG -D__MPLAB_DEBUGGER_PK3=1    -omf=elf -DMODEL_12LPC15_FW0058 -DXPRJ_12LPC15=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O0 -I"../src" -I"../src/common" -I"../src/common/Cals" -I"../src/common/CAN" -I"../src/common/Models" -DMODEL_12LPC15 -msmart-io=1 -Wall -msfr-warn=off -finline 
	@${FIXDEPS} "${OBJECTDIR}/_ext/394045403/analog_dsPIC33F.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/394045403/batt_temp.o: ../src/common/batt_temp.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/394045403" 
	@${RM} ${OBJECTDIR}/_ext/394045403/batt_temp.o.d 
	@${RM} ${OBJECTDIR}/_ext/394045403/batt_temp.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../src/common/batt_temp.c  -o ${OBJECTDIR}/_ext/394045403/batt_temp.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/394045403/batt_temp.o.d"      -g -D__DEBUG -D__MPLAB_DEBUGGER_PK3=1    -omf=elf -DMODEL_12LPC15_FW0058 -DXPRJ_12LPC15=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O0 -I"../src" -I"../src/common" -I"../src/common/Cals" -I"../src/common/CAN" -I"../src/common/Models" -DMODEL_12LPC15 -msmart-io=1 -Wall -msfr-warn=off -finline 
	@${FIXDEPS} "${OBJECTDIR}/_ext/394045403/batt_temp.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/919134522/dsPIC33_CAN.o: ../src/common/CAN/dsPIC33_CAN.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/919134522" 
	@${RM} ${OBJECTDIR}/_ext/919134522/dsPIC33_CAN.o.d 
	@${RM} ${OBJECTDIR}/_ext/919134522/dsPIC33_CAN.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../src/common/CAN/dsPIC33_CAN.c  -o ${OBJECTDIR}/_ext/919134522/dsPIC33_CAN.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/919134522/dsPIC33_CAN.o.d"      -g -D__DEBUG -D__MPLAB_DEBUGGER_PK3=1    -omf=elf -DMODEL_12LPC15_FW0058 -DXPRJ_12LPC15=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O0 -I"../src" -I"../src/common" -I"../src/common/Cals" -I"../src/common/CAN" -I"../src/common/Models" -DMODEL_12LPC15 -msmart-io=1 -Wall -msfr-warn=off -finline 
	@${FIXDEPS} "${OBJECTDIR}/_ext/919134522/dsPIC33_CAN.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/919134522/J1939.o: ../src/common/CAN/J1939.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/919134522" 
	@${RM} ${OBJECTDIR}/_ext/919134522/J1939.o.d 
	@${RM} ${OBJECTDIR}/_ext/919134522/J1939.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../src/common/CAN/J1939.c  -o ${OBJECTDIR}/_ext/919134522/J1939.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/919134522/J1939.o.d"      -g -D__DEBUG -D__MPLAB_DEBUGGER_PK3=1    -omf=elf -DMODEL_12LPC15_FW0058 -DXPRJ_12LPC15=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O0 -I"../src" -I"../src/common" -I"../src/common/Cals" -I"../src/common/CAN" -I"../src/common/Models" -DMODEL_12LPC15 -msmart-io=1 -Wall -msfr-warn=off -finline 
	@${FIXDEPS} "${OBJECTDIR}/_ext/919134522/J1939.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/919134522/rv_can.o: ../src/common/CAN/rv_can.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/919134522" 
	@${RM} ${OBJECTDIR}/_ext/919134522/rv_can.o.d 
	@${RM} ${OBJECTDIR}/_ext/919134522/rv_can.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../src/common/CAN/rv_can.c  -o ${OBJECTDIR}/_ext/919134522/rv_can.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/919134522/rv_can.o.d"      -g -D__DEBUG -D__MPLAB_DEBUGGER_PK3=1    -omf=elf -DMODEL_12LPC15_FW0058 -DXPRJ_12LPC15=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O0 -I"../src" -I"../src/common" -I"../src/common/Cals" -I"../src/common/CAN" -I"../src/common/Models" -DMODEL_12LPC15 -msmart-io=1 -Wall -msfr-warn=off -finline 
	@${FIXDEPS} "${OBJECTDIR}/_ext/919134522/rv_can.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/919134522/sensata_can.o: ../src/common/CAN/sensata_can.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/919134522" 
	@${RM} ${OBJECTDIR}/_ext/919134522/sensata_can.o.d 
	@${RM} ${OBJECTDIR}/_ext/919134522/sensata_can.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../src/common/CAN/sensata_can.c  -o ${OBJECTDIR}/_ext/919134522/sensata_can.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/919134522/sensata_can.o.d"      -g -D__DEBUG -D__MPLAB_DEBUGGER_PK3=1    -omf=elf -DMODEL_12LPC15_FW0058 -DXPRJ_12LPC15=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O0 -I"../src" -I"../src/common" -I"../src/common/Cals" -I"../src/common/CAN" -I"../src/common/Models" -DMODEL_12LPC15 -msmart-io=1 -Wall -msfr-warn=off -finline 
	@${FIXDEPS} "${OBJECTDIR}/_ext/919134522/sensata_can.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/394045403/charger.o: ../src/common/charger.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/394045403" 
	@${RM} ${OBJECTDIR}/_ext/394045403/charger.o.d 
	@${RM} ${OBJECTDIR}/_ext/394045403/charger.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../src/common/charger.c  -o ${OBJECTDIR}/_ext/394045403/charger.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/394045403/charger.o.d"      -g -D__DEBUG -D__MPLAB_DEBUGGER_PK3=1    -omf=elf -DMODEL_12LPC15_FW0058 -DXPRJ_12LPC15=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O0 -I"../src" -I"../src/common" -I"../src/common/Cals" -I"../src/common/CAN" -I"../src/common/Models" -DMODEL_12LPC15 -msmart-io=1 -Wall -msfr-warn=off -finline 
	@${FIXDEPS} "${OBJECTDIR}/_ext/394045403/charger.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/394045403/charger_3step.o: ../src/common/charger_3step.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/394045403" 
	@${RM} ${OBJECTDIR}/_ext/394045403/charger_3step.o.d 
	@${RM} ${OBJECTDIR}/_ext/394045403/charger_3step.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../src/common/charger_3step.c  -o ${OBJECTDIR}/_ext/394045403/charger_3step.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/394045403/charger_3step.o.d"      -g -D__DEBUG -D__MPLAB_DEBUGGER_PK3=1    -omf=elf -DMODEL_12LPC15_FW0058 -DXPRJ_12LPC15=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O0 -I"../src" -I"../src/common" -I"../src/common/Cals" -I"../src/common/CAN" -I"../src/common/Models" -DMODEL_12LPC15 -msmart-io=1 -Wall -msfr-warn=off -finline 
	@${FIXDEPS} "${OBJECTDIR}/_ext/394045403/charger_3step.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/394045403/charger_cmds.o: ../src/common/charger_cmds.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/394045403" 
	@${RM} ${OBJECTDIR}/_ext/394045403/charger_cmds.o.d 
	@${RM} ${OBJECTDIR}/_ext/394045403/charger_cmds.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../src/common/charger_cmds.c  -o ${OBJECTDIR}/_ext/394045403/charger_cmds.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/394045403/charger_cmds.o.d"      -g -D__DEBUG -D__MPLAB_DEBUGGER_PK3=1    -omf=elf -DMODEL_12LPC15_FW0058 -DXPRJ_12LPC15=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O0 -I"../src" -I"../src/common" -I"../src/common/Cals" -I"../src/common/CAN" -I"../src/common/Models" -DMODEL_12LPC15 -msmart-io=1 -Wall -msfr-warn=off -finline 
	@${FIXDEPS} "${OBJECTDIR}/_ext/394045403/charger_cmds.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/394045403/charger_isr.o: ../src/common/charger_isr.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/394045403" 
	@${RM} ${OBJECTDIR}/_ext/394045403/charger_isr.o.d 
	@${RM} ${OBJECTDIR}/_ext/394045403/charger_isr.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../src/common/charger_isr.c  -o ${OBJECTDIR}/_ext/394045403/charger_isr.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/394045403/charger_isr.o.d"      -g -D__DEBUG -D__MPLAB_DEBUGGER_PK3=1    -omf=elf -DMODEL_12LPC15_FW0058 -DXPRJ_12LPC15=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O0 -I"../src" -I"../src/common" -I"../src/common/Cals" -I"../src/common/CAN" -I"../src/common/Models" -DMODEL_12LPC15 -msmart-io=1 -Wall -msfr-warn=off -finline 
	@${FIXDEPS} "${OBJECTDIR}/_ext/394045403/charger_isr.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/394045403/charger_liion.o: ../src/common/charger_liion.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/394045403" 
	@${RM} ${OBJECTDIR}/_ext/394045403/charger_liion.o.d 
	@${RM} ${OBJECTDIR}/_ext/394045403/charger_liion.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../src/common/charger_liion.c  -o ${OBJECTDIR}/_ext/394045403/charger_liion.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/394045403/charger_liion.o.d"      -g -D__DEBUG -D__MPLAB_DEBUGGER_PK3=1    -omf=elf -DMODEL_12LPC15_FW0058 -DXPRJ_12LPC15=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O0 -I"../src" -I"../src/common" -I"../src/common/Cals" -I"../src/common/CAN" -I"../src/common/Models" -DMODEL_12LPC15 -msmart-io=1 -Wall -msfr-warn=off -finline 
	@${FIXDEPS} "${OBJECTDIR}/_ext/394045403/charger_liion.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/394045403/config.o: ../src/common/config.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/394045403" 
	@${RM} ${OBJECTDIR}/_ext/394045403/config.o.d 
	@${RM} ${OBJECTDIR}/_ext/394045403/config.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../src/common/config.c  -o ${OBJECTDIR}/_ext/394045403/config.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/394045403/config.o.d"      -g -D__DEBUG -D__MPLAB_DEBUGGER_PK3=1    -omf=elf -DMODEL_12LPC15_FW0058 -DXPRJ_12LPC15=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O0 -I"../src" -I"../src/common" -I"../src/common/Cals" -I"../src/common/CAN" -I"../src/common/Models" -DMODEL_12LPC15 -msmart-io=1 -Wall -msfr-warn=off -finline 
	@${FIXDEPS} "${OBJECTDIR}/_ext/394045403/config.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/394045403/converter_cmds.o: ../src/common/converter_cmds.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/394045403" 
	@${RM} ${OBJECTDIR}/_ext/394045403/converter_cmds.o.d 
	@${RM} ${OBJECTDIR}/_ext/394045403/converter_cmds.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../src/common/converter_cmds.c  -o ${OBJECTDIR}/_ext/394045403/converter_cmds.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/394045403/converter_cmds.o.d"      -g -D__DEBUG -D__MPLAB_DEBUGGER_PK3=1    -omf=elf -DMODEL_12LPC15_FW0058 -DXPRJ_12LPC15=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O0 -I"../src" -I"../src/common" -I"../src/common/Cals" -I"../src/common/CAN" -I"../src/common/Models" -DMODEL_12LPC15 -msmart-io=1 -Wall -msfr-warn=off -finline 
	@${FIXDEPS} "${OBJECTDIR}/_ext/394045403/converter_cmds.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/394045403/dac.o: ../src/common/dac.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/394045403" 
	@${RM} ${OBJECTDIR}/_ext/394045403/dac.o.d 
	@${RM} ${OBJECTDIR}/_ext/394045403/dac.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../src/common/dac.c  -o ${OBJECTDIR}/_ext/394045403/dac.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/394045403/dac.o.d"      -g -D__DEBUG -D__MPLAB_DEBUGGER_PK3=1    -omf=elf -DMODEL_12LPC15_FW0058 -DXPRJ_12LPC15=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O0 -I"../src" -I"../src/common" -I"../src/common/Cals" -I"../src/common/CAN" -I"../src/common/Models" -DMODEL_12LPC15 -msmart-io=1 -Wall -msfr-warn=off -finline 
	@${FIXDEPS} "${OBJECTDIR}/_ext/394045403/dac.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/394045403/dsPIC_serial.o: ../src/common/dsPIC_serial.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/394045403" 
	@${RM} ${OBJECTDIR}/_ext/394045403/dsPIC_serial.o.d 
	@${RM} ${OBJECTDIR}/_ext/394045403/dsPIC_serial.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../src/common/dsPIC_serial.c  -o ${OBJECTDIR}/_ext/394045403/dsPIC_serial.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/394045403/dsPIC_serial.o.d"      -g -D__DEBUG -D__MPLAB_DEBUGGER_PK3=1    -omf=elf -DMODEL_12LPC15_FW0058 -DXPRJ_12LPC15=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O0 -I"../src" -I"../src/common" -I"../src/common/Cals" -I"../src/common/CAN" -I"../src/common/Models" -DMODEL_12LPC15 -msmart-io=1 -Wall -msfr-warn=off -finline 
	@${FIXDEPS} "${OBJECTDIR}/_ext/394045403/dsPIC_serial.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/394045403/hs_temp.o: ../src/common/hs_temp.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/394045403" 
	@${RM} ${OBJECTDIR}/_ext/394045403/hs_temp.o.d 
	@${RM} ${OBJECTDIR}/_ext/394045403/hs_temp.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../src/common/hs_temp.c  -o ${OBJECTDIR}/_ext/394045403/hs_temp.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/394045403/hs_temp.o.d"      -g -D__DEBUG -D__MPLAB_DEBUGGER_PK3=1    -omf=elf -DMODEL_12LPC15_FW0058 -DXPRJ_12LPC15=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O0 -I"../src" -I"../src/common" -I"../src/common/Cals" -I"../src/common/CAN" -I"../src/common/Models" -DMODEL_12LPC15 -msmart-io=1 -Wall -msfr-warn=off -finline 
	@${FIXDEPS} "${OBJECTDIR}/_ext/394045403/hs_temp.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/394045403/inverter_cmds.o: ../src/common/inverter_cmds.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/394045403" 
	@${RM} ${OBJECTDIR}/_ext/394045403/inverter_cmds.o.d 
	@${RM} ${OBJECTDIR}/_ext/394045403/inverter_cmds.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../src/common/inverter_cmds.c  -o ${OBJECTDIR}/_ext/394045403/inverter_cmds.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/394045403/inverter_cmds.o.d"      -g -D__DEBUG -D__MPLAB_DEBUGGER_PK3=1    -omf=elf -DMODEL_12LPC15_FW0058 -DXPRJ_12LPC15=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O0 -I"../src" -I"../src/common" -I"../src/common/Cals" -I"../src/common/CAN" -I"../src/common/Models" -DMODEL_12LPC15 -msmart-io=1 -Wall -msfr-warn=off -finline 
	@${FIXDEPS} "${OBJECTDIR}/_ext/394045403/inverter_cmds.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/394045403/inv_check_supply.o: ../src/common/inv_check_supply.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/394045403" 
	@${RM} ${OBJECTDIR}/_ext/394045403/inv_check_supply.o.d 
	@${RM} ${OBJECTDIR}/_ext/394045403/inv_check_supply.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../src/common/inv_check_supply.c  -o ${OBJECTDIR}/_ext/394045403/inv_check_supply.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/394045403/inv_check_supply.o.d"      -g -D__DEBUG -D__MPLAB_DEBUGGER_PK3=1    -omf=elf -DMODEL_12LPC15_FW0058 -DXPRJ_12LPC15=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O0 -I"../src" -I"../src/common" -I"../src/common/Cals" -I"../src/common/CAN" -I"../src/common/Models" -DMODEL_12LPC15 -msmart-io=1 -Wall -msfr-warn=off -finline 
	@${FIXDEPS} "${OBJECTDIR}/_ext/394045403/inv_check_supply.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/394045403/itoa.o: ../src/common/itoa.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/394045403" 
	@${RM} ${OBJECTDIR}/_ext/394045403/itoa.o.d 
	@${RM} ${OBJECTDIR}/_ext/394045403/itoa.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../src/common/itoa.c  -o ${OBJECTDIR}/_ext/394045403/itoa.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/394045403/itoa.o.d"      -g -D__DEBUG -D__MPLAB_DEBUGGER_PK3=1    -omf=elf -DMODEL_12LPC15_FW0058 -DXPRJ_12LPC15=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O0 -I"../src" -I"../src/common" -I"../src/common/Cals" -I"../src/common/CAN" -I"../src/common/Models" -DMODEL_12LPC15 -msmart-io=1 -Wall -msfr-warn=off -finline 
	@${FIXDEPS} "${OBJECTDIR}/_ext/394045403/itoa.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/394045403/log.o: ../src/common/log.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/394045403" 
	@${RM} ${OBJECTDIR}/_ext/394045403/log.o.d 
	@${RM} ${OBJECTDIR}/_ext/394045403/log.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../src/common/log.c  -o ${OBJECTDIR}/_ext/394045403/log.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/394045403/log.o.d"      -g -D__DEBUG -D__MPLAB_DEBUGGER_PK3=1    -omf=elf -DMODEL_12LPC15_FW0058 -DXPRJ_12LPC15=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O0 -I"../src" -I"../src/common" -I"../src/common/Cals" -I"../src/common/CAN" -I"../src/common/Models" -DMODEL_12LPC15 -msmart-io=1 -Wall -msfr-warn=off -finline 
	@${FIXDEPS} "${OBJECTDIR}/_ext/394045403/log.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/394045403/nvm.o: ../src/common/nvm.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/394045403" 
	@${RM} ${OBJECTDIR}/_ext/394045403/nvm.o.d 
	@${RM} ${OBJECTDIR}/_ext/394045403/nvm.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../src/common/nvm.c  -o ${OBJECTDIR}/_ext/394045403/nvm.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/394045403/nvm.o.d"      -g -D__DEBUG -D__MPLAB_DEBUGGER_PK3=1    -omf=elf -DMODEL_12LPC15_FW0058 -DXPRJ_12LPC15=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O0 -I"../src" -I"../src/common" -I"../src/common/Cals" -I"../src/common/CAN" -I"../src/common/Models" -DMODEL_12LPC15 -msmart-io=1 -Wall -msfr-warn=off -finline 
	@${FIXDEPS} "${OBJECTDIR}/_ext/394045403/nvm.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/394045403/options.o: ../src/common/options.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/394045403" 
	@${RM} ${OBJECTDIR}/_ext/394045403/options.o.d 
	@${RM} ${OBJECTDIR}/_ext/394045403/options.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../src/common/options.c  -o ${OBJECTDIR}/_ext/394045403/options.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/394045403/options.o.d"      -g -D__DEBUG -D__MPLAB_DEBUGGER_PK3=1    -omf=elf -DMODEL_12LPC15_FW0058 -DXPRJ_12LPC15=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O0 -I"../src" -I"../src/common" -I"../src/common/Cals" -I"../src/common/CAN" -I"../src/common/Models" -DMODEL_12LPC15 -msmart-io=1 -Wall -msfr-warn=off -finline 
	@${FIXDEPS} "${OBJECTDIR}/_ext/394045403/options.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/394045403/pwm.o: ../src/common/pwm.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/394045403" 
	@${RM} ${OBJECTDIR}/_ext/394045403/pwm.o.d 
	@${RM} ${OBJECTDIR}/_ext/394045403/pwm.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../src/common/pwm.c  -o ${OBJECTDIR}/_ext/394045403/pwm.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/394045403/pwm.o.d"      -g -D__DEBUG -D__MPLAB_DEBUGGER_PK3=1    -omf=elf -DMODEL_12LPC15_FW0058 -DXPRJ_12LPC15=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O0 -I"../src" -I"../src/common" -I"../src/common/Cals" -I"../src/common/CAN" -I"../src/common/Models" -DMODEL_12LPC15 -msmart-io=1 -Wall -msfr-warn=off -finline 
	@${FIXDEPS} "${OBJECTDIR}/_ext/394045403/pwm.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/394045403/rom.o: ../src/common/rom.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/394045403" 
	@${RM} ${OBJECTDIR}/_ext/394045403/rom.o.d 
	@${RM} ${OBJECTDIR}/_ext/394045403/rom.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../src/common/rom.c  -o ${OBJECTDIR}/_ext/394045403/rom.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/394045403/rom.o.d"      -g -D__DEBUG -D__MPLAB_DEBUGGER_PK3=1    -omf=elf -DMODEL_12LPC15_FW0058 -DXPRJ_12LPC15=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O0 -I"../src" -I"../src/common" -I"../src/common/Cals" -I"../src/common/CAN" -I"../src/common/Models" -DMODEL_12LPC15 -msmart-io=1 -Wall -msfr-warn=off -finline 
	@${FIXDEPS} "${OBJECTDIR}/_ext/394045403/rom.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/394045403/signal_capture.o: ../src/common/signal_capture.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/394045403" 
	@${RM} ${OBJECTDIR}/_ext/394045403/signal_capture.o.d 
	@${RM} ${OBJECTDIR}/_ext/394045403/signal_capture.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../src/common/signal_capture.c  -o ${OBJECTDIR}/_ext/394045403/signal_capture.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/394045403/signal_capture.o.d"      -g -D__DEBUG -D__MPLAB_DEBUGGER_PK3=1    -omf=elf -DMODEL_12LPC15_FW0058 -DXPRJ_12LPC15=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O0 -I"../src" -I"../src/common" -I"../src/common/Cals" -I"../src/common/CAN" -I"../src/common/Models" -DMODEL_12LPC15 -msmart-io=1 -Wall -msfr-warn=off -finline 
	@${FIXDEPS} "${OBJECTDIR}/_ext/394045403/signal_capture.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/394045403/sine_table.o: ../src/common/sine_table.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/394045403" 
	@${RM} ${OBJECTDIR}/_ext/394045403/sine_table.o.d 
	@${RM} ${OBJECTDIR}/_ext/394045403/sine_table.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../src/common/sine_table.c  -o ${OBJECTDIR}/_ext/394045403/sine_table.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/394045403/sine_table.o.d"      -g -D__DEBUG -D__MPLAB_DEBUGGER_PK3=1    -omf=elf -DMODEL_12LPC15_FW0058 -DXPRJ_12LPC15=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O0 -I"../src" -I"../src/common" -I"../src/common/Cals" -I"../src/common/CAN" -I"../src/common/Models" -DMODEL_12LPC15 -msmart-io=1 -Wall -msfr-warn=off -finline 
	@${FIXDEPS} "${OBJECTDIR}/_ext/394045403/sine_table.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/394045403/spi.o: ../src/common/spi.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/394045403" 
	@${RM} ${OBJECTDIR}/_ext/394045403/spi.o.d 
	@${RM} ${OBJECTDIR}/_ext/394045403/spi.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../src/common/spi.c  -o ${OBJECTDIR}/_ext/394045403/spi.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/394045403/spi.o.d"      -g -D__DEBUG -D__MPLAB_DEBUGGER_PK3=1    -omf=elf -DMODEL_12LPC15_FW0058 -DXPRJ_12LPC15=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O0 -I"../src" -I"../src/common" -I"../src/common/Cals" -I"../src/common/CAN" -I"../src/common/Models" -DMODEL_12LPC15 -msmart-io=1 -Wall -msfr-warn=off -finline 
	@${FIXDEPS} "${OBJECTDIR}/_ext/394045403/spi.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/394045403/sqrt.o: ../src/common/sqrt.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/394045403" 
	@${RM} ${OBJECTDIR}/_ext/394045403/sqrt.o.d 
	@${RM} ${OBJECTDIR}/_ext/394045403/sqrt.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../src/common/sqrt.c  -o ${OBJECTDIR}/_ext/394045403/sqrt.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/394045403/sqrt.o.d"      -g -D__DEBUG -D__MPLAB_DEBUGGER_PK3=1    -omf=elf -DMODEL_12LPC15_FW0058 -DXPRJ_12LPC15=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O0 -I"../src" -I"../src/common" -I"../src/common/Cals" -I"../src/common/CAN" -I"../src/common/Models" -DMODEL_12LPC15 -msmart-io=1 -Wall -msfr-warn=off -finline 
	@${FIXDEPS} "${OBJECTDIR}/_ext/394045403/sqrt.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/394045403/ssr.o: ../src/common/ssr.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/394045403" 
	@${RM} ${OBJECTDIR}/_ext/394045403/ssr.o.d 
	@${RM} ${OBJECTDIR}/_ext/394045403/ssr.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../src/common/ssr.c  -o ${OBJECTDIR}/_ext/394045403/ssr.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/394045403/ssr.o.d"      -g -D__DEBUG -D__MPLAB_DEBUGGER_PK3=1    -omf=elf -DMODEL_12LPC15_FW0058 -DXPRJ_12LPC15=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O0 -I"../src" -I"../src/common" -I"../src/common/Cals" -I"../src/common/CAN" -I"../src/common/Models" -DMODEL_12LPC15 -msmart-io=1 -Wall -msfr-warn=off -finline 
	@${FIXDEPS} "${OBJECTDIR}/_ext/394045403/ssr.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/394045403/tasker.o: ../src/common/tasker.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/394045403" 
	@${RM} ${OBJECTDIR}/_ext/394045403/tasker.o.d 
	@${RM} ${OBJECTDIR}/_ext/394045403/tasker.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../src/common/tasker.c  -o ${OBJECTDIR}/_ext/394045403/tasker.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/394045403/tasker.o.d"      -g -D__DEBUG -D__MPLAB_DEBUGGER_PK3=1    -omf=elf -DMODEL_12LPC15_FW0058 -DXPRJ_12LPC15=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O0 -I"../src" -I"../src/common" -I"../src/common/Cals" -I"../src/common/CAN" -I"../src/common/Models" -DMODEL_12LPC15 -msmart-io=1 -Wall -msfr-warn=off -finline 
	@${FIXDEPS} "${OBJECTDIR}/_ext/394045403/tasker.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/394045403/timer1.o: ../src/common/timer1.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/394045403" 
	@${RM} ${OBJECTDIR}/_ext/394045403/timer1.o.d 
	@${RM} ${OBJECTDIR}/_ext/394045403/timer1.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../src/common/timer1.c  -o ${OBJECTDIR}/_ext/394045403/timer1.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/394045403/timer1.o.d"      -g -D__DEBUG -D__MPLAB_DEBUGGER_PK3=1    -omf=elf -DMODEL_12LPC15_FW0058 -DXPRJ_12LPC15=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O0 -I"../src" -I"../src/common" -I"../src/common/Cals" -I"../src/common/CAN" -I"../src/common/Models" -DMODEL_12LPC15 -msmart-io=1 -Wall -msfr-warn=off -finline 
	@${FIXDEPS} "${OBJECTDIR}/_ext/394045403/timer1.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/394045403/timer3.o: ../src/common/timer3.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/394045403" 
	@${RM} ${OBJECTDIR}/_ext/394045403/timer3.o.d 
	@${RM} ${OBJECTDIR}/_ext/394045403/timer3.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../src/common/timer3.c  -o ${OBJECTDIR}/_ext/394045403/timer3.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/394045403/timer3.o.d"      -g -D__DEBUG -D__MPLAB_DEBUGGER_PK3=1    -omf=elf -DMODEL_12LPC15_FW0058 -DXPRJ_12LPC15=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O0 -I"../src" -I"../src/common" -I"../src/common/Cals" -I"../src/common/CAN" -I"../src/common/Models" -DMODEL_12LPC15 -msmart-io=1 -Wall -msfr-warn=off -finline 
	@${FIXDEPS} "${OBJECTDIR}/_ext/394045403/timer3.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/394045403/traps.o: ../src/common/traps.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/394045403" 
	@${RM} ${OBJECTDIR}/_ext/394045403/traps.o.d 
	@${RM} ${OBJECTDIR}/_ext/394045403/traps.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../src/common/traps.c  -o ${OBJECTDIR}/_ext/394045403/traps.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/394045403/traps.o.d"      -g -D__DEBUG -D__MPLAB_DEBUGGER_PK3=1    -omf=elf -DMODEL_12LPC15_FW0058 -DXPRJ_12LPC15=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O0 -I"../src" -I"../src/common" -I"../src/common/Cals" -I"../src/common/CAN" -I"../src/common/Models" -DMODEL_12LPC15 -msmart-io=1 -Wall -msfr-warn=off -finline 
	@${FIXDEPS} "${OBJECTDIR}/_ext/394045403/traps.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/394045403/fan_ctrl_lpc.o: ../src/common/fan_ctrl_lpc.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/394045403" 
	@${RM} ${OBJECTDIR}/_ext/394045403/fan_ctrl_lpc.o.d 
	@${RM} ${OBJECTDIR}/_ext/394045403/fan_ctrl_lpc.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../src/common/fan_ctrl_lpc.c  -o ${OBJECTDIR}/_ext/394045403/fan_ctrl_lpc.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/394045403/fan_ctrl_lpc.o.d"      -g -D__DEBUG -D__MPLAB_DEBUGGER_PK3=1    -omf=elf -DMODEL_12LPC15_FW0058 -DXPRJ_12LPC15=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O0 -I"../src" -I"../src/common" -I"../src/common/Cals" -I"../src/common/CAN" -I"../src/common/Models" -DMODEL_12LPC15 -msmart-io=1 -Wall -msfr-warn=off -finline 
	@${FIXDEPS} "${OBJECTDIR}/_ext/394045403/fan_ctrl_lpc.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
else
${OBJECTDIR}/_ext/1360937237/inverter.o: ../src/inverter.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1360937237" 
	@${RM} ${OBJECTDIR}/_ext/1360937237/inverter.o.d 
	@${RM} ${OBJECTDIR}/_ext/1360937237/inverter.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../src/inverter.c  -o ${OBJECTDIR}/_ext/1360937237/inverter.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/1360937237/inverter.o.d"        -g -omf=elf -DMODEL_12LPC15_FW0058 -DXPRJ_12LPC15=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O0 -I"../src" -I"../src/common" -I"../src/common/Cals" -I"../src/common/CAN" -I"../src/common/Models" -DMODEL_12LPC15 -msmart-io=1 -Wall -msfr-warn=off -finline 
	@${FIXDEPS} "${OBJECTDIR}/_ext/1360937237/inverter.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/1360937237/main.o: ../src/main.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1360937237" 
	@${RM} ${OBJECTDIR}/_ext/1360937237/main.o.d 
	@${RM} ${OBJECTDIR}/_ext/1360937237/main.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../src/main.c  -o ${OBJECTDIR}/_ext/1360937237/main.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/1360937237/main.o.d"        -g -omf=elf -DMODEL_12LPC15_FW0058 -DXPRJ_12LPC15=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O0 -I"../src" -I"../src/common" -I"../src/common/Cals" -I"../src/common/CAN" -I"../src/common/Models" -DMODEL_12LPC15 -msmart-io=1 -Wall -msfr-warn=off -finline 
	@${FIXDEPS} "${OBJECTDIR}/_ext/1360937237/main.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/1360937237/ui.o: ../src/ui.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1360937237" 
	@${RM} ${OBJECTDIR}/_ext/1360937237/ui.o.d 
	@${RM} ${OBJECTDIR}/_ext/1360937237/ui.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../src/ui.c  -o ${OBJECTDIR}/_ext/1360937237/ui.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/1360937237/ui.o.d"        -g -omf=elf -DMODEL_12LPC15_FW0058 -DXPRJ_12LPC15=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O0 -I"../src" -I"../src/common" -I"../src/common/Cals" -I"../src/common/CAN" -I"../src/common/Models" -DMODEL_12LPC15 -msmart-io=1 -Wall -msfr-warn=off -finline 
	@${FIXDEPS} "${OBJECTDIR}/_ext/1360937237/ui.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/1360937237/task_dev.o: ../src/task_dev.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1360937237" 
	@${RM} ${OBJECTDIR}/_ext/1360937237/task_dev.o.d 
	@${RM} ${OBJECTDIR}/_ext/1360937237/task_dev.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../src/task_dev.c  -o ${OBJECTDIR}/_ext/1360937237/task_dev.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/1360937237/task_dev.o.d"        -g -omf=elf -DMODEL_12LPC15_FW0058 -DXPRJ_12LPC15=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O0 -I"../src" -I"../src/common" -I"../src/common/Cals" -I"../src/common/CAN" -I"../src/common/Models" -DMODEL_12LPC15 -msmart-io=1 -Wall -msfr-warn=off -finline 
	@${FIXDEPS} "${OBJECTDIR}/_ext/1360937237/task_dev.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/1360937237/task_temp.o: ../src/task_temp.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1360937237" 
	@${RM} ${OBJECTDIR}/_ext/1360937237/task_temp.o.d 
	@${RM} ${OBJECTDIR}/_ext/1360937237/task_temp.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../src/task_temp.c  -o ${OBJECTDIR}/_ext/1360937237/task_temp.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/1360937237/task_temp.o.d"        -g -omf=elf -DMODEL_12LPC15_FW0058 -DXPRJ_12LPC15=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O0 -I"../src" -I"../src/common" -I"../src/common/Cals" -I"../src/common/CAN" -I"../src/common/Models" -DMODEL_12LPC15 -msmart-io=1 -Wall -msfr-warn=off -finline 
	@${FIXDEPS} "${OBJECTDIR}/_ext/1360937237/task_temp.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/1360937237/task_main.o: ../src/task_main.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1360937237" 
	@${RM} ${OBJECTDIR}/_ext/1360937237/task_main.o.d 
	@${RM} ${OBJECTDIR}/_ext/1360937237/task_main.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../src/task_main.c  -o ${OBJECTDIR}/_ext/1360937237/task_main.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/1360937237/task_main.o.d"        -g -omf=elf -DMODEL_12LPC15_FW0058 -DXPRJ_12LPC15=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O0 -I"../src" -I"../src/common" -I"../src/common/Cals" -I"../src/common/CAN" -I"../src/common/Models" -DMODEL_12LPC15 -msmart-io=1 -Wall -msfr-warn=off -finline 
	@${FIXDEPS} "${OBJECTDIR}/_ext/1360937237/task_main.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/394045403/analog.o: ../src/common/analog.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/394045403" 
	@${RM} ${OBJECTDIR}/_ext/394045403/analog.o.d 
	@${RM} ${OBJECTDIR}/_ext/394045403/analog.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../src/common/analog.c  -o ${OBJECTDIR}/_ext/394045403/analog.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/394045403/analog.o.d"        -g -omf=elf -DMODEL_12LPC15_FW0058 -DXPRJ_12LPC15=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O0 -I"../src" -I"../src/common" -I"../src/common/Cals" -I"../src/common/CAN" -I"../src/common/Models" -DMODEL_12LPC15 -msmart-io=1 -Wall -msfr-warn=off -finline 
	@${FIXDEPS} "${OBJECTDIR}/_ext/394045403/analog.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/394045403/analog_dsPIC33F.o: ../src/common/analog_dsPIC33F.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/394045403" 
	@${RM} ${OBJECTDIR}/_ext/394045403/analog_dsPIC33F.o.d 
	@${RM} ${OBJECTDIR}/_ext/394045403/analog_dsPIC33F.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../src/common/analog_dsPIC33F.c  -o ${OBJECTDIR}/_ext/394045403/analog_dsPIC33F.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/394045403/analog_dsPIC33F.o.d"        -g -omf=elf -DMODEL_12LPC15_FW0058 -DXPRJ_12LPC15=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O0 -I"../src" -I"../src/common" -I"../src/common/Cals" -I"../src/common/CAN" -I"../src/common/Models" -DMODEL_12LPC15 -msmart-io=1 -Wall -msfr-warn=off -finline 
	@${FIXDEPS} "${OBJECTDIR}/_ext/394045403/analog_dsPIC33F.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/394045403/batt_temp.o: ../src/common/batt_temp.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/394045403" 
	@${RM} ${OBJECTDIR}/_ext/394045403/batt_temp.o.d 
	@${RM} ${OBJECTDIR}/_ext/394045403/batt_temp.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../src/common/batt_temp.c  -o ${OBJECTDIR}/_ext/394045403/batt_temp.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/394045403/batt_temp.o.d"        -g -omf=elf -DMODEL_12LPC15_FW0058 -DXPRJ_12LPC15=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O0 -I"../src" -I"../src/common" -I"../src/common/Cals" -I"../src/common/CAN" -I"../src/common/Models" -DMODEL_12LPC15 -msmart-io=1 -Wall -msfr-warn=off -finline 
	@${FIXDEPS} "${OBJECTDIR}/_ext/394045403/batt_temp.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/919134522/dsPIC33_CAN.o: ../src/common/CAN/dsPIC33_CAN.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/919134522" 
	@${RM} ${OBJECTDIR}/_ext/919134522/dsPIC33_CAN.o.d 
	@${RM} ${OBJECTDIR}/_ext/919134522/dsPIC33_CAN.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../src/common/CAN/dsPIC33_CAN.c  -o ${OBJECTDIR}/_ext/919134522/dsPIC33_CAN.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/919134522/dsPIC33_CAN.o.d"        -g -omf=elf -DMODEL_12LPC15_FW0058 -DXPRJ_12LPC15=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O0 -I"../src" -I"../src/common" -I"../src/common/Cals" -I"../src/common/CAN" -I"../src/common/Models" -DMODEL_12LPC15 -msmart-io=1 -Wall -msfr-warn=off -finline 
	@${FIXDEPS} "${OBJECTDIR}/_ext/919134522/dsPIC33_CAN.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/919134522/J1939.o: ../src/common/CAN/J1939.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/919134522" 
	@${RM} ${OBJECTDIR}/_ext/919134522/J1939.o.d 
	@${RM} ${OBJECTDIR}/_ext/919134522/J1939.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../src/common/CAN/J1939.c  -o ${OBJECTDIR}/_ext/919134522/J1939.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/919134522/J1939.o.d"        -g -omf=elf -DMODEL_12LPC15_FW0058 -DXPRJ_12LPC15=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O0 -I"../src" -I"../src/common" -I"../src/common/Cals" -I"../src/common/CAN" -I"../src/common/Models" -DMODEL_12LPC15 -msmart-io=1 -Wall -msfr-warn=off -finline 
	@${FIXDEPS} "${OBJECTDIR}/_ext/919134522/J1939.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/919134522/rv_can.o: ../src/common/CAN/rv_can.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/919134522" 
	@${RM} ${OBJECTDIR}/_ext/919134522/rv_can.o.d 
	@${RM} ${OBJECTDIR}/_ext/919134522/rv_can.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../src/common/CAN/rv_can.c  -o ${OBJECTDIR}/_ext/919134522/rv_can.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/919134522/rv_can.o.d"        -g -omf=elf -DMODEL_12LPC15_FW0058 -DXPRJ_12LPC15=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O0 -I"../src" -I"../src/common" -I"../src/common/Cals" -I"../src/common/CAN" -I"../src/common/Models" -DMODEL_12LPC15 -msmart-io=1 -Wall -msfr-warn=off -finline 
	@${FIXDEPS} "${OBJECTDIR}/_ext/919134522/rv_can.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/919134522/sensata_can.o: ../src/common/CAN/sensata_can.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/919134522" 
	@${RM} ${OBJECTDIR}/_ext/919134522/sensata_can.o.d 
	@${RM} ${OBJECTDIR}/_ext/919134522/sensata_can.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../src/common/CAN/sensata_can.c  -o ${OBJECTDIR}/_ext/919134522/sensata_can.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/919134522/sensata_can.o.d"        -g -omf=elf -DMODEL_12LPC15_FW0058 -DXPRJ_12LPC15=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O0 -I"../src" -I"../src/common" -I"../src/common/Cals" -I"../src/common/CAN" -I"../src/common/Models" -DMODEL_12LPC15 -msmart-io=1 -Wall -msfr-warn=off -finline 
	@${FIXDEPS} "${OBJECTDIR}/_ext/919134522/sensata_can.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/394045403/charger.o: ../src/common/charger.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/394045403" 
	@${RM} ${OBJECTDIR}/_ext/394045403/charger.o.d 
	@${RM} ${OBJECTDIR}/_ext/394045403/charger.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../src/common/charger.c  -o ${OBJECTDIR}/_ext/394045403/charger.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/394045403/charger.o.d"        -g -omf=elf -DMODEL_12LPC15_FW0058 -DXPRJ_12LPC15=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O0 -I"../src" -I"../src/common" -I"../src/common/Cals" -I"../src/common/CAN" -I"../src/common/Models" -DMODEL_12LPC15 -msmart-io=1 -Wall -msfr-warn=off -finline 
	@${FIXDEPS} "${OBJECTDIR}/_ext/394045403/charger.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/394045403/charger_3step.o: ../src/common/charger_3step.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/394045403" 
	@${RM} ${OBJECTDIR}/_ext/394045403/charger_3step.o.d 
	@${RM} ${OBJECTDIR}/_ext/394045403/charger_3step.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../src/common/charger_3step.c  -o ${OBJECTDIR}/_ext/394045403/charger_3step.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/394045403/charger_3step.o.d"        -g -omf=elf -DMODEL_12LPC15_FW0058 -DXPRJ_12LPC15=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O0 -I"../src" -I"../src/common" -I"../src/common/Cals" -I"../src/common/CAN" -I"../src/common/Models" -DMODEL_12LPC15 -msmart-io=1 -Wall -msfr-warn=off -finline 
	@${FIXDEPS} "${OBJECTDIR}/_ext/394045403/charger_3step.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/394045403/charger_cmds.o: ../src/common/charger_cmds.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/394045403" 
	@${RM} ${OBJECTDIR}/_ext/394045403/charger_cmds.o.d 
	@${RM} ${OBJECTDIR}/_ext/394045403/charger_cmds.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../src/common/charger_cmds.c  -o ${OBJECTDIR}/_ext/394045403/charger_cmds.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/394045403/charger_cmds.o.d"        -g -omf=elf -DMODEL_12LPC15_FW0058 -DXPRJ_12LPC15=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O0 -I"../src" -I"../src/common" -I"../src/common/Cals" -I"../src/common/CAN" -I"../src/common/Models" -DMODEL_12LPC15 -msmart-io=1 -Wall -msfr-warn=off -finline 
	@${FIXDEPS} "${OBJECTDIR}/_ext/394045403/charger_cmds.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/394045403/charger_isr.o: ../src/common/charger_isr.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/394045403" 
	@${RM} ${OBJECTDIR}/_ext/394045403/charger_isr.o.d 
	@${RM} ${OBJECTDIR}/_ext/394045403/charger_isr.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../src/common/charger_isr.c  -o ${OBJECTDIR}/_ext/394045403/charger_isr.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/394045403/charger_isr.o.d"        -g -omf=elf -DMODEL_12LPC15_FW0058 -DXPRJ_12LPC15=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O0 -I"../src" -I"../src/common" -I"../src/common/Cals" -I"../src/common/CAN" -I"../src/common/Models" -DMODEL_12LPC15 -msmart-io=1 -Wall -msfr-warn=off -finline 
	@${FIXDEPS} "${OBJECTDIR}/_ext/394045403/charger_isr.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/394045403/charger_liion.o: ../src/common/charger_liion.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/394045403" 
	@${RM} ${OBJECTDIR}/_ext/394045403/charger_liion.o.d 
	@${RM} ${OBJECTDIR}/_ext/394045403/charger_liion.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../src/common/charger_liion.c  -o ${OBJECTDIR}/_ext/394045403/charger_liion.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/394045403/charger_liion.o.d"        -g -omf=elf -DMODEL_12LPC15_FW0058 -DXPRJ_12LPC15=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O0 -I"../src" -I"../src/common" -I"../src/common/Cals" -I"../src/common/CAN" -I"../src/common/Models" -DMODEL_12LPC15 -msmart-io=1 -Wall -msfr-warn=off -finline 
	@${FIXDEPS} "${OBJECTDIR}/_ext/394045403/charger_liion.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/394045403/config.o: ../src/common/config.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/394045403" 
	@${RM} ${OBJECTDIR}/_ext/394045403/config.o.d 
	@${RM} ${OBJECTDIR}/_ext/394045403/config.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../src/common/config.c  -o ${OBJECTDIR}/_ext/394045403/config.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/394045403/config.o.d"        -g -omf=elf -DMODEL_12LPC15_FW0058 -DXPRJ_12LPC15=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O0 -I"../src" -I"../src/common" -I"../src/common/Cals" -I"../src/common/CAN" -I"../src/common/Models" -DMODEL_12LPC15 -msmart-io=1 -Wall -msfr-warn=off -finline 
	@${FIXDEPS} "${OBJECTDIR}/_ext/394045403/config.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/394045403/converter_cmds.o: ../src/common/converter_cmds.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/394045403" 
	@${RM} ${OBJECTDIR}/_ext/394045403/converter_cmds.o.d 
	@${RM} ${OBJECTDIR}/_ext/394045403/converter_cmds.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../src/common/converter_cmds.c  -o ${OBJECTDIR}/_ext/394045403/converter_cmds.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/394045403/converter_cmds.o.d"        -g -omf=elf -DMODEL_12LPC15_FW0058 -DXPRJ_12LPC15=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O0 -I"../src" -I"../src/common" -I"../src/common/Cals" -I"../src/common/CAN" -I"../src/common/Models" -DMODEL_12LPC15 -msmart-io=1 -Wall -msfr-warn=off -finline 
	@${FIXDEPS} "${OBJECTDIR}/_ext/394045403/converter_cmds.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/394045403/dac.o: ../src/common/dac.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/394045403" 
	@${RM} ${OBJECTDIR}/_ext/394045403/dac.o.d 
	@${RM} ${OBJECTDIR}/_ext/394045403/dac.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../src/common/dac.c  -o ${OBJECTDIR}/_ext/394045403/dac.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/394045403/dac.o.d"        -g -omf=elf -DMODEL_12LPC15_FW0058 -DXPRJ_12LPC15=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O0 -I"../src" -I"../src/common" -I"../src/common/Cals" -I"../src/common/CAN" -I"../src/common/Models" -DMODEL_12LPC15 -msmart-io=1 -Wall -msfr-warn=off -finline 
	@${FIXDEPS} "${OBJECTDIR}/_ext/394045403/dac.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/394045403/dsPIC_serial.o: ../src/common/dsPIC_serial.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/394045403" 
	@${RM} ${OBJECTDIR}/_ext/394045403/dsPIC_serial.o.d 
	@${RM} ${OBJECTDIR}/_ext/394045403/dsPIC_serial.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../src/common/dsPIC_serial.c  -o ${OBJECTDIR}/_ext/394045403/dsPIC_serial.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/394045403/dsPIC_serial.o.d"        -g -omf=elf -DMODEL_12LPC15_FW0058 -DXPRJ_12LPC15=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O0 -I"../src" -I"../src/common" -I"../src/common/Cals" -I"../src/common/CAN" -I"../src/common/Models" -DMODEL_12LPC15 -msmart-io=1 -Wall -msfr-warn=off -finline 
	@${FIXDEPS} "${OBJECTDIR}/_ext/394045403/dsPIC_serial.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/394045403/hs_temp.o: ../src/common/hs_temp.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/394045403" 
	@${RM} ${OBJECTDIR}/_ext/394045403/hs_temp.o.d 
	@${RM} ${OBJECTDIR}/_ext/394045403/hs_temp.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../src/common/hs_temp.c  -o ${OBJECTDIR}/_ext/394045403/hs_temp.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/394045403/hs_temp.o.d"        -g -omf=elf -DMODEL_12LPC15_FW0058 -DXPRJ_12LPC15=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O0 -I"../src" -I"../src/common" -I"../src/common/Cals" -I"../src/common/CAN" -I"../src/common/Models" -DMODEL_12LPC15 -msmart-io=1 -Wall -msfr-warn=off -finline 
	@${FIXDEPS} "${OBJECTDIR}/_ext/394045403/hs_temp.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/394045403/inverter_cmds.o: ../src/common/inverter_cmds.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/394045403" 
	@${RM} ${OBJECTDIR}/_ext/394045403/inverter_cmds.o.d 
	@${RM} ${OBJECTDIR}/_ext/394045403/inverter_cmds.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../src/common/inverter_cmds.c  -o ${OBJECTDIR}/_ext/394045403/inverter_cmds.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/394045403/inverter_cmds.o.d"        -g -omf=elf -DMODEL_12LPC15_FW0058 -DXPRJ_12LPC15=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O0 -I"../src" -I"../src/common" -I"../src/common/Cals" -I"../src/common/CAN" -I"../src/common/Models" -DMODEL_12LPC15 -msmart-io=1 -Wall -msfr-warn=off -finline 
	@${FIXDEPS} "${OBJECTDIR}/_ext/394045403/inverter_cmds.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/394045403/inv_check_supply.o: ../src/common/inv_check_supply.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/394045403" 
	@${RM} ${OBJECTDIR}/_ext/394045403/inv_check_supply.o.d 
	@${RM} ${OBJECTDIR}/_ext/394045403/inv_check_supply.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../src/common/inv_check_supply.c  -o ${OBJECTDIR}/_ext/394045403/inv_check_supply.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/394045403/inv_check_supply.o.d"        -g -omf=elf -DMODEL_12LPC15_FW0058 -DXPRJ_12LPC15=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O0 -I"../src" -I"../src/common" -I"../src/common/Cals" -I"../src/common/CAN" -I"../src/common/Models" -DMODEL_12LPC15 -msmart-io=1 -Wall -msfr-warn=off -finline 
	@${FIXDEPS} "${OBJECTDIR}/_ext/394045403/inv_check_supply.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/394045403/itoa.o: ../src/common/itoa.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/394045403" 
	@${RM} ${OBJECTDIR}/_ext/394045403/itoa.o.d 
	@${RM} ${OBJECTDIR}/_ext/394045403/itoa.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../src/common/itoa.c  -o ${OBJECTDIR}/_ext/394045403/itoa.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/394045403/itoa.o.d"        -g -omf=elf -DMODEL_12LPC15_FW0058 -DXPRJ_12LPC15=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O0 -I"../src" -I"../src/common" -I"../src/common/Cals" -I"../src/common/CAN" -I"../src/common/Models" -DMODEL_12LPC15 -msmart-io=1 -Wall -msfr-warn=off -finline 
	@${FIXDEPS} "${OBJECTDIR}/_ext/394045403/itoa.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/394045403/log.o: ../src/common/log.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/394045403" 
	@${RM} ${OBJECTDIR}/_ext/394045403/log.o.d 
	@${RM} ${OBJECTDIR}/_ext/394045403/log.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../src/common/log.c  -o ${OBJECTDIR}/_ext/394045403/log.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/394045403/log.o.d"        -g -omf=elf -DMODEL_12LPC15_FW0058 -DXPRJ_12LPC15=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O0 -I"../src" -I"../src/common" -I"../src/common/Cals" -I"../src/common/CAN" -I"../src/common/Models" -DMODEL_12LPC15 -msmart-io=1 -Wall -msfr-warn=off -finline 
	@${FIXDEPS} "${OBJECTDIR}/_ext/394045403/log.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/394045403/nvm.o: ../src/common/nvm.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/394045403" 
	@${RM} ${OBJECTDIR}/_ext/394045403/nvm.o.d 
	@${RM} ${OBJECTDIR}/_ext/394045403/nvm.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../src/common/nvm.c  -o ${OBJECTDIR}/_ext/394045403/nvm.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/394045403/nvm.o.d"        -g -omf=elf -DMODEL_12LPC15_FW0058 -DXPRJ_12LPC15=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O0 -I"../src" -I"../src/common" -I"../src/common/Cals" -I"../src/common/CAN" -I"../src/common/Models" -DMODEL_12LPC15 -msmart-io=1 -Wall -msfr-warn=off -finline 
	@${FIXDEPS} "${OBJECTDIR}/_ext/394045403/nvm.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/394045403/options.o: ../src/common/options.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/394045403" 
	@${RM} ${OBJECTDIR}/_ext/394045403/options.o.d 
	@${RM} ${OBJECTDIR}/_ext/394045403/options.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../src/common/options.c  -o ${OBJECTDIR}/_ext/394045403/options.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/394045403/options.o.d"        -g -omf=elf -DMODEL_12LPC15_FW0058 -DXPRJ_12LPC15=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O0 -I"../src" -I"../src/common" -I"../src/common/Cals" -I"../src/common/CAN" -I"../src/common/Models" -DMODEL_12LPC15 -msmart-io=1 -Wall -msfr-warn=off -finline 
	@${FIXDEPS} "${OBJECTDIR}/_ext/394045403/options.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/394045403/pwm.o: ../src/common/pwm.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/394045403" 
	@${RM} ${OBJECTDIR}/_ext/394045403/pwm.o.d 
	@${RM} ${OBJECTDIR}/_ext/394045403/pwm.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../src/common/pwm.c  -o ${OBJECTDIR}/_ext/394045403/pwm.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/394045403/pwm.o.d"        -g -omf=elf -DMODEL_12LPC15_FW0058 -DXPRJ_12LPC15=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O0 -I"../src" -I"../src/common" -I"../src/common/Cals" -I"../src/common/CAN" -I"../src/common/Models" -DMODEL_12LPC15 -msmart-io=1 -Wall -msfr-warn=off -finline 
	@${FIXDEPS} "${OBJECTDIR}/_ext/394045403/pwm.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/394045403/rom.o: ../src/common/rom.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/394045403" 
	@${RM} ${OBJECTDIR}/_ext/394045403/rom.o.d 
	@${RM} ${OBJECTDIR}/_ext/394045403/rom.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../src/common/rom.c  -o ${OBJECTDIR}/_ext/394045403/rom.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/394045403/rom.o.d"        -g -omf=elf -DMODEL_12LPC15_FW0058 -DXPRJ_12LPC15=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O0 -I"../src" -I"../src/common" -I"../src/common/Cals" -I"../src/common/CAN" -I"../src/common/Models" -DMODEL_12LPC15 -msmart-io=1 -Wall -msfr-warn=off -finline 
	@${FIXDEPS} "${OBJECTDIR}/_ext/394045403/rom.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/394045403/signal_capture.o: ../src/common/signal_capture.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/394045403" 
	@${RM} ${OBJECTDIR}/_ext/394045403/signal_capture.o.d 
	@${RM} ${OBJECTDIR}/_ext/394045403/signal_capture.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../src/common/signal_capture.c  -o ${OBJECTDIR}/_ext/394045403/signal_capture.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/394045403/signal_capture.o.d"        -g -omf=elf -DMODEL_12LPC15_FW0058 -DXPRJ_12LPC15=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O0 -I"../src" -I"../src/common" -I"../src/common/Cals" -I"../src/common/CAN" -I"../src/common/Models" -DMODEL_12LPC15 -msmart-io=1 -Wall -msfr-warn=off -finline 
	@${FIXDEPS} "${OBJECTDIR}/_ext/394045403/signal_capture.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/394045403/sine_table.o: ../src/common/sine_table.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/394045403" 
	@${RM} ${OBJECTDIR}/_ext/394045403/sine_table.o.d 
	@${RM} ${OBJECTDIR}/_ext/394045403/sine_table.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../src/common/sine_table.c  -o ${OBJECTDIR}/_ext/394045403/sine_table.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/394045403/sine_table.o.d"        -g -omf=elf -DMODEL_12LPC15_FW0058 -DXPRJ_12LPC15=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O0 -I"../src" -I"../src/common" -I"../src/common/Cals" -I"../src/common/CAN" -I"../src/common/Models" -DMODEL_12LPC15 -msmart-io=1 -Wall -msfr-warn=off -finline 
	@${FIXDEPS} "${OBJECTDIR}/_ext/394045403/sine_table.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/394045403/spi.o: ../src/common/spi.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/394045403" 
	@${RM} ${OBJECTDIR}/_ext/394045403/spi.o.d 
	@${RM} ${OBJECTDIR}/_ext/394045403/spi.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../src/common/spi.c  -o ${OBJECTDIR}/_ext/394045403/spi.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/394045403/spi.o.d"        -g -omf=elf -DMODEL_12LPC15_FW0058 -DXPRJ_12LPC15=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O0 -I"../src" -I"../src/common" -I"../src/common/Cals" -I"../src/common/CAN" -I"../src/common/Models" -DMODEL_12LPC15 -msmart-io=1 -Wall -msfr-warn=off -finline 
	@${FIXDEPS} "${OBJECTDIR}/_ext/394045403/spi.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/394045403/sqrt.o: ../src/common/sqrt.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/394045403" 
	@${RM} ${OBJECTDIR}/_ext/394045403/sqrt.o.d 
	@${RM} ${OBJECTDIR}/_ext/394045403/sqrt.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../src/common/sqrt.c  -o ${OBJECTDIR}/_ext/394045403/sqrt.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/394045403/sqrt.o.d"        -g -omf=elf -DMODEL_12LPC15_FW0058 -DXPRJ_12LPC15=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O0 -I"../src" -I"../src/common" -I"../src/common/Cals" -I"../src/common/CAN" -I"../src/common/Models" -DMODEL_12LPC15 -msmart-io=1 -Wall -msfr-warn=off -finline 
	@${FIXDEPS} "${OBJECTDIR}/_ext/394045403/sqrt.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/394045403/ssr.o: ../src/common/ssr.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/394045403" 
	@${RM} ${OBJECTDIR}/_ext/394045403/ssr.o.d 
	@${RM} ${OBJECTDIR}/_ext/394045403/ssr.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../src/common/ssr.c  -o ${OBJECTDIR}/_ext/394045403/ssr.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/394045403/ssr.o.d"        -g -omf=elf -DMODEL_12LPC15_FW0058 -DXPRJ_12LPC15=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O0 -I"../src" -I"../src/common" -I"../src/common/Cals" -I"../src/common/CAN" -I"../src/common/Models" -DMODEL_12LPC15 -msmart-io=1 -Wall -msfr-warn=off -finline 
	@${FIXDEPS} "${OBJECTDIR}/_ext/394045403/ssr.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/394045403/tasker.o: ../src/common/tasker.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/394045403" 
	@${RM} ${OBJECTDIR}/_ext/394045403/tasker.o.d 
	@${RM} ${OBJECTDIR}/_ext/394045403/tasker.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../src/common/tasker.c  -o ${OBJECTDIR}/_ext/394045403/tasker.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/394045403/tasker.o.d"        -g -omf=elf -DMODEL_12LPC15_FW0058 -DXPRJ_12LPC15=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O0 -I"../src" -I"../src/common" -I"../src/common/Cals" -I"../src/common/CAN" -I"../src/common/Models" -DMODEL_12LPC15 -msmart-io=1 -Wall -msfr-warn=off -finline 
	@${FIXDEPS} "${OBJECTDIR}/_ext/394045403/tasker.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/394045403/timer1.o: ../src/common/timer1.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/394045403" 
	@${RM} ${OBJECTDIR}/_ext/394045403/timer1.o.d 
	@${RM} ${OBJECTDIR}/_ext/394045403/timer1.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../src/common/timer1.c  -o ${OBJECTDIR}/_ext/394045403/timer1.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/394045403/timer1.o.d"        -g -omf=elf -DMODEL_12LPC15_FW0058 -DXPRJ_12LPC15=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O0 -I"../src" -I"../src/common" -I"../src/common/Cals" -I"../src/common/CAN" -I"../src/common/Models" -DMODEL_12LPC15 -msmart-io=1 -Wall -msfr-warn=off -finline 
	@${FIXDEPS} "${OBJECTDIR}/_ext/394045403/timer1.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/394045403/timer3.o: ../src/common/timer3.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/394045403" 
	@${RM} ${OBJECTDIR}/_ext/394045403/timer3.o.d 
	@${RM} ${OBJECTDIR}/_ext/394045403/timer3.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../src/common/timer3.c  -o ${OBJECTDIR}/_ext/394045403/timer3.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/394045403/timer3.o.d"        -g -omf=elf -DMODEL_12LPC15_FW0058 -DXPRJ_12LPC15=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O0 -I"../src" -I"../src/common" -I"../src/common/Cals" -I"../src/common/CAN" -I"../src/common/Models" -DMODEL_12LPC15 -msmart-io=1 -Wall -msfr-warn=off -finline 
	@${FIXDEPS} "${OBJECTDIR}/_ext/394045403/timer3.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/394045403/traps.o: ../src/common/traps.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/394045403" 
	@${RM} ${OBJECTDIR}/_ext/394045403/traps.o.d 
	@${RM} ${OBJECTDIR}/_ext/394045403/traps.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../src/common/traps.c  -o ${OBJECTDIR}/_ext/394045403/traps.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/394045403/traps.o.d"        -g -omf=elf -DMODEL_12LPC15_FW0058 -DXPRJ_12LPC15=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O0 -I"../src" -I"../src/common" -I"../src/common/Cals" -I"../src/common/CAN" -I"../src/common/Models" -DMODEL_12LPC15 -msmart-io=1 -Wall -msfr-warn=off -finline 
	@${FIXDEPS} "${OBJECTDIR}/_ext/394045403/traps.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/394045403/fan_ctrl_lpc.o: ../src/common/fan_ctrl_lpc.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/394045403" 
	@${RM} ${OBJECTDIR}/_ext/394045403/fan_ctrl_lpc.o.d 
	@${RM} ${OBJECTDIR}/_ext/394045403/fan_ctrl_lpc.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../src/common/fan_ctrl_lpc.c  -o ${OBJECTDIR}/_ext/394045403/fan_ctrl_lpc.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/394045403/fan_ctrl_lpc.o.d"        -g -omf=elf -DMODEL_12LPC15_FW0058 -DXPRJ_12LPC15=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O0 -I"../src" -I"../src/common" -I"../src/common/Cals" -I"../src/common/CAN" -I"../src/common/Models" -DMODEL_12LPC15 -msmart-io=1 -Wall -msfr-warn=off -finline 
	@${FIXDEPS} "${OBJECTDIR}/_ext/394045403/fan_ctrl_lpc.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
endif

# ------------------------------------------------------------------------------------
# Rules for buildStep: assemble
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
${OBJECTDIR}/_ext/394045403/getErrLoc.o: ../src/common/getErrLoc.s  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/394045403" 
	@${RM} ${OBJECTDIR}/_ext/394045403/getErrLoc.o.d 
	@${RM} ${OBJECTDIR}/_ext/394045403/getErrLoc.o 
	${MP_CC} $(MP_EXTRA_AS_PRE)  ../src/common/getErrLoc.s  -o ${OBJECTDIR}/_ext/394045403/getErrLoc.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -D__DEBUG -D__MPLAB_DEBUGGER_PK3=1  -omf=elf -DMODEL_12LPC15_FW0058 -DXPRJ_12LPC15=$(CND_CONF)  -legacy-libc  -Wa,-MD,"${OBJECTDIR}/_ext/394045403/getErrLoc.o.d",--defsym=__MPLAB_BUILD=1,--defsym=__ICD2RAM=1,--defsym=__MPLAB_DEBUG=1,--defsym=__DEBUG=1,--defsym=__MPLAB_DEBUGGER_PK3=1,-g,--no-relax$(MP_EXTRA_AS_POST)
	@${FIXDEPS} "${OBJECTDIR}/_ext/394045403/getErrLoc.o.d"  $(SILENT)  -rsi ${MP_CC_DIR}../  
	
else
${OBJECTDIR}/_ext/394045403/getErrLoc.o: ../src/common/getErrLoc.s  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/394045403" 
	@${RM} ${OBJECTDIR}/_ext/394045403/getErrLoc.o.d 
	@${RM} ${OBJECTDIR}/_ext/394045403/getErrLoc.o 
	${MP_CC} $(MP_EXTRA_AS_PRE)  ../src/common/getErrLoc.s  -o ${OBJECTDIR}/_ext/394045403/getErrLoc.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -omf=elf -DMODEL_12LPC15_FW0058 -DXPRJ_12LPC15=$(CND_CONF)  -legacy-libc  -Wa,-MD,"${OBJECTDIR}/_ext/394045403/getErrLoc.o.d",--defsym=__MPLAB_BUILD=1,-g,--no-relax$(MP_EXTRA_AS_POST)
	@${FIXDEPS} "${OBJECTDIR}/_ext/394045403/getErrLoc.o.d"  $(SILENT)  -rsi ${MP_CC_DIR}../  
	
endif

# ------------------------------------------------------------------------------------
# Rules for buildStep: assemblePreproc
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
else
endif

# ------------------------------------------------------------------------------------
# Rules for buildStep: link
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
dist/${CND_CONF}/${IMAGE_TYPE}/dsPIC_LPC.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}: ${OBJECTFILES}  nbproject/Makefile-${CND_CONF}.mk    
	@${MKDIR} dist/${CND_CONF}/${IMAGE_TYPE} 
	${MP_CC} $(MP_EXTRA_LD_PRE)  -o dist/${CND_CONF}/${IMAGE_TYPE}/dsPIC_LPC.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}  ${OBJECTFILES_QUOTED_IF_SPACED}      -mcpu=$(MP_PROCESSOR_OPTION)        -D__DEBUG -D__MPLAB_DEBUGGER_PK3=1  -omf=elf -DMODEL_12LPC15_FW0058 -DXPRJ_12LPC15=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)   -mreserve=data@0x800:0x81F -mreserve=data@0x820:0x821 -mreserve=data@0x822:0x823 -mreserve=data@0x824:0x825 -mreserve=data@0x826:0x84F   -Wl,,,--defsym=__MPLAB_BUILD=1,--defsym=__MPLAB_DEBUG=1,--defsym=__DEBUG=1,--defsym=__MPLAB_DEBUGGER_PK3=1,$(MP_LINKER_FILE_OPTION),--stack=16,--check-sections,--data-init,--pack-data,--handles,--isr,--no-gc-sections,--fill-upper=0,--stackguard=16,--no-force-link,--smart-io,-Map="${DISTDIR}/${PROJECTNAME}.${IMAGE_TYPE}.map",--report-mem,--memorysummary,dist/${CND_CONF}/${IMAGE_TYPE}/memoryfile.xml$(MP_EXTRA_LD_POST) 
	
else
dist/${CND_CONF}/${IMAGE_TYPE}/dsPIC_LPC.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}: ${OBJECTFILES}  nbproject/Makefile-${CND_CONF}.mk   
	@${MKDIR} dist/${CND_CONF}/${IMAGE_TYPE} 
	${MP_CC} $(MP_EXTRA_LD_PRE)  -o dist/${CND_CONF}/${IMAGE_TYPE}/dsPIC_LPC.X.${IMAGE_TYPE}.${DEBUGGABLE_SUFFIX}  ${OBJECTFILES_QUOTED_IF_SPACED}      -mcpu=$(MP_PROCESSOR_OPTION)        -omf=elf -DMODEL_12LPC15_FW0058 -DXPRJ_12LPC15=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -Wl,,,--defsym=__MPLAB_BUILD=1,$(MP_LINKER_FILE_OPTION),--stack=16,--check-sections,--data-init,--pack-data,--handles,--isr,--no-gc-sections,--fill-upper=0,--stackguard=16,--no-force-link,--smart-io,-Map="${DISTDIR}/${PROJECTNAME}.${IMAGE_TYPE}.map",--report-mem,--memorysummary,dist/${CND_CONF}/${IMAGE_TYPE}/memoryfile.xml$(MP_EXTRA_LD_POST) 
	${MP_CC_DIR}\\xc16-bin2hex dist/${CND_CONF}/${IMAGE_TYPE}/dsPIC_LPC.X.${IMAGE_TYPE}.${DEBUGGABLE_SUFFIX} -a  -omf=elf  
	
endif


# Subprojects
.build-subprojects:


# Subprojects
.clean-subprojects:

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r build/12LPC15
	${RM} -r dist/12LPC15

# Enable dependency checking
.dep.inc: .depcheck-impl

DEPFILES=$(shell mplabwildcard ${POSSIBLE_DEPFILES})
ifneq (${DEPFILES},)
include ${DEPFILES}
endif
