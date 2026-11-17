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
ifeq "$(wildcard nbproject/Makefile-local-MK_GPK.mk)" "nbproject/Makefile-local-MK_GPK.mk"
include nbproject/Makefile-local-MK_GPK.mk
endif
endif

# Environment
MKDIR=mkdir -p
RM=rm -f 
MV=mv 
CP=cp 

# Macros
CND_CONF=MK_GPK
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
IMAGE_TYPE=debug
OUTPUT_SUFFIX=elf
DEBUGGABLE_SUFFIX=elf
FINAL_IMAGE=${DISTDIR}/pic32.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}
else
IMAGE_TYPE=production
OUTPUT_SUFFIX=hex
DEBUGGABLE_SUFFIX=elf
FINAL_IMAGE=${DISTDIR}/pic32.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}
endif

ifeq ($(COMPARE_BUILD), true)
COMPARISON_BUILD=-mafrlcsj
else
COMPARISON_BUILD=
endif

# Object Directory
OBJECTDIR=build/${CND_CONF}/${IMAGE_TYPE}

# Distribution Directory
DISTDIR=dist/${CND_CONF}/${IMAGE_TYPE}

# Source Files Quoted if spaced
SOURCEFILES_QUOTED_IF_SPACED=../cpustick/basic0.c ../cpustick/cpustick.c ../cpustick/parse2.c ../cpustick/run2.c ../cpustick/scope-command.c ../cpustick/fleascope-hal.c ../cpustick/stream-command.c ../cpustick/digital-stream-command.c ../sources/adc.c ../sources/cdcacm.c ../sources/flash.c ../sources/i2c.c ../sources/led.c ../sources/pin.c ../sources/printf.c ../sources/qspi.c ../sources/serial.c ../sources/terminal.c ../sources/text.c ../sources/timer.c ../sources/usb.c ../sources/util.c ../sources/zigflea.c ../stickos/basic.c ../stickos/code.c ../stickos/parse.c ../stickos/run.c ../stickos/vars.c ../main.c ../myplib.c

# Object Files Quoted if spaced
OBJECTFILES_QUOTED_IF_SPACED=${OBJECTDIR}/_ext/35980457/basic0.o ${OBJECTDIR}/_ext/35980457/cpustick.o ${OBJECTDIR}/_ext/35980457/parse2.o ${OBJECTDIR}/_ext/35980457/run2.o ${OBJECTDIR}/_ext/35980457/scope-command.o ${OBJECTDIR}/_ext/35980457/fleascope-hal.o ${OBJECTDIR}/_ext/35980457/stream-command.o ${OBJECTDIR}/_ext/35980457/digital-stream-command.o ${OBJECTDIR}/_ext/592584297/adc.o ${OBJECTDIR}/_ext/592584297/cdcacm.o ${OBJECTDIR}/_ext/592584297/flash.o ${OBJECTDIR}/_ext/592584297/i2c.o ${OBJECTDIR}/_ext/592584297/led.o ${OBJECTDIR}/_ext/592584297/pin.o ${OBJECTDIR}/_ext/592584297/printf.o ${OBJECTDIR}/_ext/592584297/qspi.o ${OBJECTDIR}/_ext/592584297/serial.o ${OBJECTDIR}/_ext/592584297/terminal.o ${OBJECTDIR}/_ext/592584297/text.o ${OBJECTDIR}/_ext/592584297/timer.o ${OBJECTDIR}/_ext/592584297/usb.o ${OBJECTDIR}/_ext/592584297/util.o ${OBJECTDIR}/_ext/592584297/zigflea.o ${OBJECTDIR}/_ext/724208933/basic.o ${OBJECTDIR}/_ext/724208933/code.o ${OBJECTDIR}/_ext/724208933/parse.o ${OBJECTDIR}/_ext/724208933/run.o ${OBJECTDIR}/_ext/724208933/vars.o ${OBJECTDIR}/_ext/1472/main.o ${OBJECTDIR}/_ext/1472/myplib.o
POSSIBLE_DEPFILES=${OBJECTDIR}/_ext/35980457/basic0.o.d ${OBJECTDIR}/_ext/35980457/cpustick.o.d ${OBJECTDIR}/_ext/35980457/parse2.o.d ${OBJECTDIR}/_ext/35980457/run2.o.d ${OBJECTDIR}/_ext/35980457/scope-command.o.d ${OBJECTDIR}/_ext/35980457/fleascope-hal.o.d ${OBJECTDIR}/_ext/35980457/stream-command.o.d ${OBJECTDIR}/_ext/35980457/digital-stream-command.o.d ${OBJECTDIR}/_ext/592584297/adc.o.d ${OBJECTDIR}/_ext/592584297/cdcacm.o.d ${OBJECTDIR}/_ext/592584297/flash.o.d ${OBJECTDIR}/_ext/592584297/i2c.o.d ${OBJECTDIR}/_ext/592584297/led.o.d ${OBJECTDIR}/_ext/592584297/pin.o.d ${OBJECTDIR}/_ext/592584297/printf.o.d ${OBJECTDIR}/_ext/592584297/qspi.o.d ${OBJECTDIR}/_ext/592584297/serial.o.d ${OBJECTDIR}/_ext/592584297/terminal.o.d ${OBJECTDIR}/_ext/592584297/text.o.d ${OBJECTDIR}/_ext/592584297/timer.o.d ${OBJECTDIR}/_ext/592584297/usb.o.d ${OBJECTDIR}/_ext/592584297/util.o.d ${OBJECTDIR}/_ext/592584297/zigflea.o.d ${OBJECTDIR}/_ext/724208933/basic.o.d ${OBJECTDIR}/_ext/724208933/code.o.d ${OBJECTDIR}/_ext/724208933/parse.o.d ${OBJECTDIR}/_ext/724208933/run.o.d ${OBJECTDIR}/_ext/724208933/vars.o.d ${OBJECTDIR}/_ext/1472/main.o.d ${OBJECTDIR}/_ext/1472/myplib.o.d

# Object Files
OBJECTFILES=${OBJECTDIR}/_ext/35980457/basic0.o ${OBJECTDIR}/_ext/35980457/cpustick.o ${OBJECTDIR}/_ext/35980457/parse2.o ${OBJECTDIR}/_ext/35980457/run2.o ${OBJECTDIR}/_ext/35980457/scope-command.o ${OBJECTDIR}/_ext/35980457/fleascope-hal.o ${OBJECTDIR}/_ext/35980457/stream-command.o ${OBJECTDIR}/_ext/35980457/digital-stream-command.o ${OBJECTDIR}/_ext/592584297/adc.o ${OBJECTDIR}/_ext/592584297/cdcacm.o ${OBJECTDIR}/_ext/592584297/flash.o ${OBJECTDIR}/_ext/592584297/i2c.o ${OBJECTDIR}/_ext/592584297/led.o ${OBJECTDIR}/_ext/592584297/pin.o ${OBJECTDIR}/_ext/592584297/printf.o ${OBJECTDIR}/_ext/592584297/qspi.o ${OBJECTDIR}/_ext/592584297/serial.o ${OBJECTDIR}/_ext/592584297/terminal.o ${OBJECTDIR}/_ext/592584297/text.o ${OBJECTDIR}/_ext/592584297/timer.o ${OBJECTDIR}/_ext/592584297/usb.o ${OBJECTDIR}/_ext/592584297/util.o ${OBJECTDIR}/_ext/592584297/zigflea.o ${OBJECTDIR}/_ext/724208933/basic.o ${OBJECTDIR}/_ext/724208933/code.o ${OBJECTDIR}/_ext/724208933/parse.o ${OBJECTDIR}/_ext/724208933/run.o ${OBJECTDIR}/_ext/724208933/vars.o ${OBJECTDIR}/_ext/1472/main.o ${OBJECTDIR}/_ext/1472/myplib.o

# Source Files
SOURCEFILES=../cpustick/basic0.c ../cpustick/cpustick.c ../cpustick/parse2.c ../cpustick/run2.c ../cpustick/scope-command.c ../cpustick/fleascope-hal.c ../cpustick/stream-command.c ../cpustick/digital-stream-command.c ../sources/adc.c ../sources/cdcacm.c ../sources/flash.c ../sources/i2c.c ../sources/led.c ../sources/pin.c ../sources/printf.c ../sources/qspi.c ../sources/serial.c ../sources/terminal.c ../sources/text.c ../sources/timer.c ../sources/usb.c ../sources/util.c ../sources/zigflea.c ../stickos/basic.c ../stickos/code.c ../stickos/parse.c ../stickos/run.c ../stickos/vars.c ../main.c ../myplib.c



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

.build-conf:  ${BUILD_SUBPROJECTS}
ifneq ($(INFORMATION_MESSAGE), )
	@echo $(INFORMATION_MESSAGE)
endif
	${MAKE}  -f nbproject/Makefile-MK_GPK.mk ${DISTDIR}/pic32.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}

MP_PROCESSOR_OPTION=32MK0512GPK064
MP_LINKER_FILE_OPTION=
# ------------------------------------------------------------------------------------
# Rules for buildStep: assemble
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
else
endif

# ------------------------------------------------------------------------------------
# Rules for buildStep: assembleWithPreprocess
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
else
endif

# ------------------------------------------------------------------------------------
# Rules for buildStep: compile
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
${OBJECTDIR}/_ext/35980457/basic0.o: ../cpustick/basic0.c  .generated_files/flags/MK_GPK/fb1a05ef4113613bba7ed8ffba1706c368e06663 .generated_files/flags/MK_GPK/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/35980457" 
	@${RM} ${OBJECTDIR}/_ext/35980457/basic0.o.d 
	@${RM} ${OBJECTDIR}/_ext/35980457/basic0.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -mmicromips -mno-float -O3 -funroll-loops -fomit-frame-pointer -fno-common -DSTICKOS -I".." -I"../pict-o-crypt" -I"../cpustick" -I"../headers" -I"../sources" -I"../stickos" -Wall -Waddress-attribute-use -MP -MMD -MF "${OBJECTDIR}/_ext/35980457/basic0.o.d" -o ${OBJECTDIR}/_ext/35980457/basic0.o ../cpustick/basic0.c    -DXPRJ_MK_GPK=$(CND_CONF)    $(COMPARISON_BUILD)  -fno-builtin -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/_ext/35980457/cpustick.o: ../cpustick/cpustick.c  .generated_files/flags/MK_GPK/dff74b58085c404c4b0e0803d1d5ebee58f75ac9 .generated_files/flags/MK_GPK/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/35980457" 
	@${RM} ${OBJECTDIR}/_ext/35980457/cpustick.o.d 
	@${RM} ${OBJECTDIR}/_ext/35980457/cpustick.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -mmicromips -mno-float -O3 -funroll-loops -fomit-frame-pointer -fno-common -DSTICKOS -I".." -I"../pict-o-crypt" -I"../cpustick" -I"../headers" -I"../sources" -I"../stickos" -Wall -Waddress-attribute-use -MP -MMD -MF "${OBJECTDIR}/_ext/35980457/cpustick.o.d" -o ${OBJECTDIR}/_ext/35980457/cpustick.o ../cpustick/cpustick.c    -DXPRJ_MK_GPK=$(CND_CONF)    $(COMPARISON_BUILD)  -fno-builtin -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/_ext/35980457/parse2.o: ../cpustick/parse2.c  .generated_files/flags/MK_GPK/8ea0b8ef1d76d8680534e4efc2283bed2946c965 .generated_files/flags/MK_GPK/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/35980457" 
	@${RM} ${OBJECTDIR}/_ext/35980457/parse2.o.d 
	@${RM} ${OBJECTDIR}/_ext/35980457/parse2.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -mmicromips -mno-float -O3 -funroll-loops -fomit-frame-pointer -fno-common -DSTICKOS -I".." -I"../pict-o-crypt" -I"../cpustick" -I"../headers" -I"../sources" -I"../stickos" -Wall -Waddress-attribute-use -MP -MMD -MF "${OBJECTDIR}/_ext/35980457/parse2.o.d" -o ${OBJECTDIR}/_ext/35980457/parse2.o ../cpustick/parse2.c    -DXPRJ_MK_GPK=$(CND_CONF)    $(COMPARISON_BUILD)  -fno-builtin -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/_ext/35980457/run2.o: ../cpustick/run2.c  .generated_files/flags/MK_GPK/542f4d5af86dfbab3e63664ed52fa5c7e552fd08 .generated_files/flags/MK_GPK/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/35980457" 
	@${RM} ${OBJECTDIR}/_ext/35980457/run2.o.d 
	@${RM} ${OBJECTDIR}/_ext/35980457/run2.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -mmicromips -mno-float -O3 -funroll-loops -fomit-frame-pointer -fno-common -DSTICKOS -I".." -I"../pict-o-crypt" -I"../cpustick" -I"../headers" -I"../sources" -I"../stickos" -Wall -Waddress-attribute-use -MP -MMD -MF "${OBJECTDIR}/_ext/35980457/run2.o.d" -o ${OBJECTDIR}/_ext/35980457/run2.o ../cpustick/run2.c    -DXPRJ_MK_GPK=$(CND_CONF)    $(COMPARISON_BUILD)  -fno-builtin -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/_ext/35980457/scope-command.o: ../cpustick/scope-command.c  .generated_files/flags/MK_GPK/3e36c85da89bdb5f7bde6344dc1c27a6b14c9e04 .generated_files/flags/MK_GPK/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/35980457" 
	@${RM} ${OBJECTDIR}/_ext/35980457/scope-command.o.d 
	@${RM} ${OBJECTDIR}/_ext/35980457/scope-command.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -mmicromips -mno-float -O3 -funroll-loops -fomit-frame-pointer -fno-common -DSTICKOS -I".." -I"../pict-o-crypt" -I"../cpustick" -I"../headers" -I"../sources" -I"../stickos" -Wall -Waddress-attribute-use -MP -MMD -MF "${OBJECTDIR}/_ext/35980457/scope-command.o.d" -o ${OBJECTDIR}/_ext/35980457/scope-command.o ../cpustick/scope-command.c    -DXPRJ_MK_GPK=$(CND_CONF)    $(COMPARISON_BUILD)  -fno-builtin -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/_ext/35980457/fleascope-hal.o: ../cpustick/fleascope-hal.c  .generated_files/flags/MK_GPK/a22f988e6294586f34aa19769a2c835a771046a4 .generated_files/flags/MK_GPK/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/35980457" 
	@${RM} ${OBJECTDIR}/_ext/35980457/fleascope-hal.o.d 
	@${RM} ${OBJECTDIR}/_ext/35980457/fleascope-hal.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -mmicromips -mno-float -O3 -funroll-loops -fomit-frame-pointer -fno-common -DSTICKOS -I".." -I"../pict-o-crypt" -I"../cpustick" -I"../headers" -I"../sources" -I"../stickos" -Wall -Waddress-attribute-use -MP -MMD -MF "${OBJECTDIR}/_ext/35980457/fleascope-hal.o.d" -o ${OBJECTDIR}/_ext/35980457/fleascope-hal.o ../cpustick/fleascope-hal.c    -DXPRJ_MK_GPK=$(CND_CONF)    $(COMPARISON_BUILD)  -fno-builtin -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/_ext/35980457/stream-command.o: ../cpustick/stream-command.c  .generated_files/flags/MK_GPK/aac945af4d3d5262a148905b83d7b1055dbb5958 .generated_files/flags/MK_GPK/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/35980457" 
	@${RM} ${OBJECTDIR}/_ext/35980457/stream-command.o.d 
	@${RM} ${OBJECTDIR}/_ext/35980457/stream-command.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -mmicromips -mno-float -O3 -funroll-loops -fomit-frame-pointer -fno-common -DSTICKOS -I".." -I"../pict-o-crypt" -I"../cpustick" -I"../headers" -I"../sources" -I"../stickos" -Wall -Waddress-attribute-use -MP -MMD -MF "${OBJECTDIR}/_ext/35980457/stream-command.o.d" -o ${OBJECTDIR}/_ext/35980457/stream-command.o ../cpustick/stream-command.c    -DXPRJ_MK_GPK=$(CND_CONF)    $(COMPARISON_BUILD)  -fno-builtin -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/_ext/35980457/digital-stream-command.o: ../cpustick/digital-stream-command.c  .generated_files/flags/MK_GPK/aac945af4d3d5262a148905b83d7b1055dbb5958 .generated_files/flags/MK_GPK/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/35980457" 
	@${RM} ${OBJECTDIR}/_ext/35980457/digital-stream-command.o.d 
	@${RM} ${OBJECTDIR}/_ext/35980457/digital-stream-command.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -mmicromips -mno-float -O3 -funroll-loops -fomit-frame-pointer -fno-common -DSTICKOS -I".." -I"../pict-o-crypt" -I"../cpustick" -I"../headers" -I"../sources" -I"../stickos" -Wall -Waddress-attribute-use -MP -MMD -MF "${OBJECTDIR}/_ext/35980457/digital-stream-command.o.d" -o ${OBJECTDIR}/_ext/35980457/digital-stream-command.o ../cpustick/digital-stream-command.c    -DXPRJ_MK_GPK=$(CND_CONF)    $(COMPARISON_BUILD)  -fno-builtin -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/_ext/592584297/adc.o: ../sources/adc.c  .generated_files/flags/MK_GPK/c5fbb739542e9b73e0ff95c0f3ca69ebfcb74c55 .generated_files/flags/MK_GPK/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/592584297" 
	@${RM} ${OBJECTDIR}/_ext/592584297/adc.o.d 
	@${RM} ${OBJECTDIR}/_ext/592584297/adc.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -mmicromips -mno-float -O3 -funroll-loops -fomit-frame-pointer -fno-common -DSTICKOS -I".." -I"../pict-o-crypt" -I"../cpustick" -I"../headers" -I"../sources" -I"../stickos" -Wall -Waddress-attribute-use -MP -MMD -MF "${OBJECTDIR}/_ext/592584297/adc.o.d" -o ${OBJECTDIR}/_ext/592584297/adc.o ../sources/adc.c    -DXPRJ_MK_GPK=$(CND_CONF)    $(COMPARISON_BUILD)  -fno-builtin -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/_ext/592584297/cdcacm.o: ../sources/cdcacm.c  .generated_files/flags/MK_GPK/e48d580339b69b9bd5da8f34ee96d0f1ea79d566 .generated_files/flags/MK_GPK/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/592584297" 
	@${RM} ${OBJECTDIR}/_ext/592584297/cdcacm.o.d 
	@${RM} ${OBJECTDIR}/_ext/592584297/cdcacm.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -mmicromips -mno-float -O3 -funroll-loops -fomit-frame-pointer -fno-common -DSTICKOS -I".." -I"../pict-o-crypt" -I"../cpustick" -I"../headers" -I"../sources" -I"../stickos" -Wall -Waddress-attribute-use -MP -MMD -MF "${OBJECTDIR}/_ext/592584297/cdcacm.o.d" -o ${OBJECTDIR}/_ext/592584297/cdcacm.o ../sources/cdcacm.c    -DXPRJ_MK_GPK=$(CND_CONF)    $(COMPARISON_BUILD)  -fno-builtin -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/_ext/592584297/flash.o: ../sources/flash.c  .generated_files/flags/MK_GPK/134b99028de2ca4bb198a9178f54daef81007b16 .generated_files/flags/MK_GPK/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/592584297" 
	@${RM} ${OBJECTDIR}/_ext/592584297/flash.o.d 
	@${RM} ${OBJECTDIR}/_ext/592584297/flash.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -mmicromips -mno-float -O3 -funroll-loops -fomit-frame-pointer -fno-common -DSTICKOS -I".." -I"../pict-o-crypt" -I"../cpustick" -I"../headers" -I"../sources" -I"../stickos" -Wall -Waddress-attribute-use -MP -MMD -MF "${OBJECTDIR}/_ext/592584297/flash.o.d" -o ${OBJECTDIR}/_ext/592584297/flash.o ../sources/flash.c    -DXPRJ_MK_GPK=$(CND_CONF)    $(COMPARISON_BUILD)  -fno-builtin -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/_ext/592584297/i2c.o: ../sources/i2c.c  .generated_files/flags/MK_GPK/7f2d7c4becd8a7d64818b08dc6b5b8fa7423f13 .generated_files/flags/MK_GPK/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/592584297" 
	@${RM} ${OBJECTDIR}/_ext/592584297/i2c.o.d 
	@${RM} ${OBJECTDIR}/_ext/592584297/i2c.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -mmicromips -mno-float -O3 -funroll-loops -fomit-frame-pointer -fno-common -DSTICKOS -I".." -I"../pict-o-crypt" -I"../cpustick" -I"../headers" -I"../sources" -I"../stickos" -Wall -Waddress-attribute-use -MP -MMD -MF "${OBJECTDIR}/_ext/592584297/i2c.o.d" -o ${OBJECTDIR}/_ext/592584297/i2c.o ../sources/i2c.c    -DXPRJ_MK_GPK=$(CND_CONF)    $(COMPARISON_BUILD)  -fno-builtin -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/_ext/592584297/led.o: ../sources/led.c  .generated_files/flags/MK_GPK/836514bd958e180dd596f1b17c60f4eaa2a9bfef .generated_files/flags/MK_GPK/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/592584297" 
	@${RM} ${OBJECTDIR}/_ext/592584297/led.o.d 
	@${RM} ${OBJECTDIR}/_ext/592584297/led.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -mmicromips -mno-float -O3 -funroll-loops -fomit-frame-pointer -fno-common -DSTICKOS -I".." -I"../pict-o-crypt" -I"../cpustick" -I"../headers" -I"../sources" -I"../stickos" -Wall -Waddress-attribute-use -MP -MMD -MF "${OBJECTDIR}/_ext/592584297/led.o.d" -o ${OBJECTDIR}/_ext/592584297/led.o ../sources/led.c    -DXPRJ_MK_GPK=$(CND_CONF)    $(COMPARISON_BUILD)  -fno-builtin -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/_ext/592584297/pin.o: ../sources/pin.c  .generated_files/flags/MK_GPK/31eceb2448cc7b5828137e2de12979d19f6fd640 .generated_files/flags/MK_GPK/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/592584297" 
	@${RM} ${OBJECTDIR}/_ext/592584297/pin.o.d 
	@${RM} ${OBJECTDIR}/_ext/592584297/pin.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -mmicromips -mno-float -O3 -funroll-loops -fomit-frame-pointer -fno-common -DSTICKOS -I".." -I"../pict-o-crypt" -I"../cpustick" -I"../headers" -I"../sources" -I"../stickos" -Wall -Waddress-attribute-use -MP -MMD -MF "${OBJECTDIR}/_ext/592584297/pin.o.d" -o ${OBJECTDIR}/_ext/592584297/pin.o ../sources/pin.c    -DXPRJ_MK_GPK=$(CND_CONF)    $(COMPARISON_BUILD)  -fno-builtin -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/_ext/592584297/printf.o: ../sources/printf.c  .generated_files/flags/MK_GPK/a6a73dc897efe937ba379d7fde209ada75578031 .generated_files/flags/MK_GPK/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/592584297" 
	@${RM} ${OBJECTDIR}/_ext/592584297/printf.o.d 
	@${RM} ${OBJECTDIR}/_ext/592584297/printf.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -mmicromips -mno-float -O3 -funroll-loops -fomit-frame-pointer -fno-common -DSTICKOS -I".." -I"../pict-o-crypt" -I"../cpustick" -I"../headers" -I"../sources" -I"../stickos" -Wall -Waddress-attribute-use -MP -MMD -MF "${OBJECTDIR}/_ext/592584297/printf.o.d" -o ${OBJECTDIR}/_ext/592584297/printf.o ../sources/printf.c    -DXPRJ_MK_GPK=$(CND_CONF)    $(COMPARISON_BUILD)  -fno-builtin -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/_ext/592584297/qspi.o: ../sources/qspi.c  .generated_files/flags/MK_GPK/b4ee21f8aea89ba0128ef0d58b08895c5352aad3 .generated_files/flags/MK_GPK/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/592584297" 
	@${RM} ${OBJECTDIR}/_ext/592584297/qspi.o.d 
	@${RM} ${OBJECTDIR}/_ext/592584297/qspi.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -mmicromips -mno-float -O3 -funroll-loops -fomit-frame-pointer -fno-common -DSTICKOS -I".." -I"../pict-o-crypt" -I"../cpustick" -I"../headers" -I"../sources" -I"../stickos" -Wall -Waddress-attribute-use -MP -MMD -MF "${OBJECTDIR}/_ext/592584297/qspi.o.d" -o ${OBJECTDIR}/_ext/592584297/qspi.o ../sources/qspi.c    -DXPRJ_MK_GPK=$(CND_CONF)    $(COMPARISON_BUILD)  -fno-builtin -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/_ext/592584297/serial.o: ../sources/serial.c  .generated_files/flags/MK_GPK/52eb057775f78a25c0b10d7196774f2632faa9c6 .generated_files/flags/MK_GPK/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/592584297" 
	@${RM} ${OBJECTDIR}/_ext/592584297/serial.o.d 
	@${RM} ${OBJECTDIR}/_ext/592584297/serial.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -mmicromips -mno-float -O3 -funroll-loops -fomit-frame-pointer -fno-common -DSTICKOS -I".." -I"../pict-o-crypt" -I"../cpustick" -I"../headers" -I"../sources" -I"../stickos" -Wall -Waddress-attribute-use -MP -MMD -MF "${OBJECTDIR}/_ext/592584297/serial.o.d" -o ${OBJECTDIR}/_ext/592584297/serial.o ../sources/serial.c    -DXPRJ_MK_GPK=$(CND_CONF)    $(COMPARISON_BUILD)  -fno-builtin -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/_ext/592584297/terminal.o: ../sources/terminal.c  .generated_files/flags/MK_GPK/f4923a4ebc14210ff8c246c5afa5fd9a1ea9b47f .generated_files/flags/MK_GPK/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/592584297" 
	@${RM} ${OBJECTDIR}/_ext/592584297/terminal.o.d 
	@${RM} ${OBJECTDIR}/_ext/592584297/terminal.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -mmicromips -mno-float -O3 -funroll-loops -fomit-frame-pointer -fno-common -DSTICKOS -I".." -I"../pict-o-crypt" -I"../cpustick" -I"../headers" -I"../sources" -I"../stickos" -Wall -Waddress-attribute-use -MP -MMD -MF "${OBJECTDIR}/_ext/592584297/terminal.o.d" -o ${OBJECTDIR}/_ext/592584297/terminal.o ../sources/terminal.c    -DXPRJ_MK_GPK=$(CND_CONF)    $(COMPARISON_BUILD)  -fno-builtin -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/_ext/592584297/text.o: ../sources/text.c  .generated_files/flags/MK_GPK/64a3fc20b4e654472cc89728c8c5092d324e908c .generated_files/flags/MK_GPK/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/592584297" 
	@${RM} ${OBJECTDIR}/_ext/592584297/text.o.d 
	@${RM} ${OBJECTDIR}/_ext/592584297/text.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -mmicromips -mno-float -O3 -funroll-loops -fomit-frame-pointer -fno-common -DSTICKOS -I".." -I"../pict-o-crypt" -I"../cpustick" -I"../headers" -I"../sources" -I"../stickos" -Wall -Waddress-attribute-use -MP -MMD -MF "${OBJECTDIR}/_ext/592584297/text.o.d" -o ${OBJECTDIR}/_ext/592584297/text.o ../sources/text.c    -DXPRJ_MK_GPK=$(CND_CONF)    $(COMPARISON_BUILD)  -fno-builtin -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/_ext/592584297/timer.o: ../sources/timer.c  .generated_files/flags/MK_GPK/399525eef3cdd0781dae5beb3045f21355e25dd2 .generated_files/flags/MK_GPK/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/592584297" 
	@${RM} ${OBJECTDIR}/_ext/592584297/timer.o.d 
	@${RM} ${OBJECTDIR}/_ext/592584297/timer.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -mmicromips -mno-float -O3 -funroll-loops -fomit-frame-pointer -fno-common -DSTICKOS -I".." -I"../pict-o-crypt" -I"../cpustick" -I"../headers" -I"../sources" -I"../stickos" -Wall -Waddress-attribute-use -MP -MMD -MF "${OBJECTDIR}/_ext/592584297/timer.o.d" -o ${OBJECTDIR}/_ext/592584297/timer.o ../sources/timer.c    -DXPRJ_MK_GPK=$(CND_CONF)    $(COMPARISON_BUILD)  -fno-builtin -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/_ext/592584297/usb.o: ../sources/usb.c  .generated_files/flags/MK_GPK/855aeef613b9e1845499e24bd1aecf16ba2df7ed .generated_files/flags/MK_GPK/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/592584297" 
	@${RM} ${OBJECTDIR}/_ext/592584297/usb.o.d 
	@${RM} ${OBJECTDIR}/_ext/592584297/usb.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -mmicromips -mno-float -O3 -funroll-loops -fomit-frame-pointer -fno-common -DSTICKOS -I".." -I"../pict-o-crypt" -I"../cpustick" -I"../headers" -I"../sources" -I"../stickos" -Wall -Waddress-attribute-use -MP -MMD -MF "${OBJECTDIR}/_ext/592584297/usb.o.d" -o ${OBJECTDIR}/_ext/592584297/usb.o ../sources/usb.c    -DXPRJ_MK_GPK=$(CND_CONF)    $(COMPARISON_BUILD)  -fno-builtin -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/_ext/592584297/util.o: ../sources/util.c  .generated_files/flags/MK_GPK/8bc8bcbd655e205e1ab75fd099ea7814c32114ba .generated_files/flags/MK_GPK/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/592584297" 
	@${RM} ${OBJECTDIR}/_ext/592584297/util.o.d 
	@${RM} ${OBJECTDIR}/_ext/592584297/util.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -mmicromips -mno-float -O3 -funroll-loops -fomit-frame-pointer -fno-common -DSTICKOS -I".." -I"../pict-o-crypt" -I"../cpustick" -I"../headers" -I"../sources" -I"../stickos" -Wall -Waddress-attribute-use -MP -MMD -MF "${OBJECTDIR}/_ext/592584297/util.o.d" -o ${OBJECTDIR}/_ext/592584297/util.o ../sources/util.c    -DXPRJ_MK_GPK=$(CND_CONF)    $(COMPARISON_BUILD)  -fno-builtin -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/_ext/592584297/zigflea.o: ../sources/zigflea.c  .generated_files/flags/MK_GPK/1697825ba19af56f11a9f80b50f5ff03fb874e5 .generated_files/flags/MK_GPK/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/592584297" 
	@${RM} ${OBJECTDIR}/_ext/592584297/zigflea.o.d 
	@${RM} ${OBJECTDIR}/_ext/592584297/zigflea.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -mmicromips -mno-float -O3 -funroll-loops -fomit-frame-pointer -fno-common -DSTICKOS -I".." -I"../pict-o-crypt" -I"../cpustick" -I"../headers" -I"../sources" -I"../stickos" -Wall -Waddress-attribute-use -MP -MMD -MF "${OBJECTDIR}/_ext/592584297/zigflea.o.d" -o ${OBJECTDIR}/_ext/592584297/zigflea.o ../sources/zigflea.c    -DXPRJ_MK_GPK=$(CND_CONF)    $(COMPARISON_BUILD)  -fno-builtin -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/_ext/724208933/basic.o: ../stickos/basic.c  .generated_files/flags/MK_GPK/b2770ed0d0218fb803c4847383dbed79c1c2acae .generated_files/flags/MK_GPK/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/724208933" 
	@${RM} ${OBJECTDIR}/_ext/724208933/basic.o.d 
	@${RM} ${OBJECTDIR}/_ext/724208933/basic.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -mmicromips -mno-float -O3 -funroll-loops -fomit-frame-pointer -fno-common -DSTICKOS -I".." -I"../pict-o-crypt" -I"../cpustick" -I"../headers" -I"../sources" -I"../stickos" -Wall -Waddress-attribute-use -MP -MMD -MF "${OBJECTDIR}/_ext/724208933/basic.o.d" -o ${OBJECTDIR}/_ext/724208933/basic.o ../stickos/basic.c    -DXPRJ_MK_GPK=$(CND_CONF)    $(COMPARISON_BUILD)  -fno-builtin -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/_ext/724208933/code.o: ../stickos/code.c  .generated_files/flags/MK_GPK/f7f6760fc26343d62bdde52e938e7147cb378796 .generated_files/flags/MK_GPK/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/724208933" 
	@${RM} ${OBJECTDIR}/_ext/724208933/code.o.d 
	@${RM} ${OBJECTDIR}/_ext/724208933/code.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -mmicromips -mno-float -O3 -funroll-loops -fomit-frame-pointer -fno-common -DSTICKOS -I".." -I"../pict-o-crypt" -I"../cpustick" -I"../headers" -I"../sources" -I"../stickos" -Wall -Waddress-attribute-use -MP -MMD -MF "${OBJECTDIR}/_ext/724208933/code.o.d" -o ${OBJECTDIR}/_ext/724208933/code.o ../stickos/code.c    -DXPRJ_MK_GPK=$(CND_CONF)    $(COMPARISON_BUILD)  -fno-builtin -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/_ext/724208933/parse.o: ../stickos/parse.c  .generated_files/flags/MK_GPK/f194eb4dd06feccedab93586d7070b093f662c55 .generated_files/flags/MK_GPK/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/724208933" 
	@${RM} ${OBJECTDIR}/_ext/724208933/parse.o.d 
	@${RM} ${OBJECTDIR}/_ext/724208933/parse.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -mmicromips -mno-float -O3 -funroll-loops -fomit-frame-pointer -fno-common -DSTICKOS -I".." -I"../pict-o-crypt" -I"../cpustick" -I"../headers" -I"../sources" -I"../stickos" -Wall -Waddress-attribute-use -MP -MMD -MF "${OBJECTDIR}/_ext/724208933/parse.o.d" -o ${OBJECTDIR}/_ext/724208933/parse.o ../stickos/parse.c    -DXPRJ_MK_GPK=$(CND_CONF)    $(COMPARISON_BUILD)  -fno-builtin -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/_ext/724208933/run.o: ../stickos/run.c  .generated_files/flags/MK_GPK/8acb639e3c39e7eab79dbfc7e9a62286191be88 .generated_files/flags/MK_GPK/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/724208933" 
	@${RM} ${OBJECTDIR}/_ext/724208933/run.o.d 
	@${RM} ${OBJECTDIR}/_ext/724208933/run.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -mmicromips -mno-float -O3 -funroll-loops -fomit-frame-pointer -fno-common -DSTICKOS -I".." -I"../pict-o-crypt" -I"../cpustick" -I"../headers" -I"../sources" -I"../stickos" -Wall -Waddress-attribute-use -MP -MMD -MF "${OBJECTDIR}/_ext/724208933/run.o.d" -o ${OBJECTDIR}/_ext/724208933/run.o ../stickos/run.c    -DXPRJ_MK_GPK=$(CND_CONF)    $(COMPARISON_BUILD)  -fno-builtin -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/_ext/724208933/vars.o: ../stickos/vars.c  .generated_files/flags/MK_GPK/8a0f86c848f4b792f556acd8bcfc3a6e440450a1 .generated_files/flags/MK_GPK/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/724208933" 
	@${RM} ${OBJECTDIR}/_ext/724208933/vars.o.d 
	@${RM} ${OBJECTDIR}/_ext/724208933/vars.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -mmicromips -mno-float -O3 -funroll-loops -fomit-frame-pointer -fno-common -DSTICKOS -I".." -I"../pict-o-crypt" -I"../cpustick" -I"../headers" -I"../sources" -I"../stickos" -Wall -Waddress-attribute-use -MP -MMD -MF "${OBJECTDIR}/_ext/724208933/vars.o.d" -o ${OBJECTDIR}/_ext/724208933/vars.o ../stickos/vars.c    -DXPRJ_MK_GPK=$(CND_CONF)    $(COMPARISON_BUILD)  -fno-builtin -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/_ext/1472/main.o: ../main.c  .generated_files/flags/MK_GPK/be7d7c7116a2a0662ac8ab6490c608f4c83f0bb0 .generated_files/flags/MK_GPK/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/1472" 
	@${RM} ${OBJECTDIR}/_ext/1472/main.o.d 
	@${RM} ${OBJECTDIR}/_ext/1472/main.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -mmicromips -mno-float -O3 -funroll-loops -fomit-frame-pointer -fno-common -DSTICKOS -I".." -I"../pict-o-crypt" -I"../cpustick" -I"../headers" -I"../sources" -I"../stickos" -Wall -Waddress-attribute-use -MP -MMD -MF "${OBJECTDIR}/_ext/1472/main.o.d" -o ${OBJECTDIR}/_ext/1472/main.o ../main.c    -DXPRJ_MK_GPK=$(CND_CONF)    $(COMPARISON_BUILD)  -fno-builtin -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/_ext/1472/myplib.o: ../myplib.c  .generated_files/flags/MK_GPK/93560c4cd38b6a9ecb7c1660fe937add90a0c3e7 .generated_files/flags/MK_GPK/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/1472" 
	@${RM} ${OBJECTDIR}/_ext/1472/myplib.o.d 
	@${RM} ${OBJECTDIR}/_ext/1472/myplib.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -mmicromips -mno-float -O3 -funroll-loops -fomit-frame-pointer -fno-common -DSTICKOS -I".." -I"../pict-o-crypt" -I"../cpustick" -I"../headers" -I"../sources" -I"../stickos" -Wall -Waddress-attribute-use -MP -MMD -MF "${OBJECTDIR}/_ext/1472/myplib.o.d" -o ${OBJECTDIR}/_ext/1472/myplib.o ../myplib.c    -DXPRJ_MK_GPK=$(CND_CONF)    $(COMPARISON_BUILD)  -fno-builtin -mdfp="${DFP_DIR}"  
	
else
${OBJECTDIR}/_ext/35980457/basic0.o: ../cpustick/basic0.c  .generated_files/flags/MK_GPK/954f19a350821f60d8d62454cd9b45bcbf3b0724 .generated_files/flags/MK_GPK/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/35980457" 
	@${RM} ${OBJECTDIR}/_ext/35980457/basic0.o.d 
	@${RM} ${OBJECTDIR}/_ext/35980457/basic0.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -mmicromips -mno-float -O3 -funroll-loops -fomit-frame-pointer -fno-common -DSTICKOS -I".." -I"../pict-o-crypt" -I"../cpustick" -I"../headers" -I"../sources" -I"../stickos" -Wall -Waddress-attribute-use -MP -MMD -MF "${OBJECTDIR}/_ext/35980457/basic0.o.d" -o ${OBJECTDIR}/_ext/35980457/basic0.o ../cpustick/basic0.c    -DXPRJ_MK_GPK=$(CND_CONF)    $(COMPARISON_BUILD)  -fno-builtin -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/_ext/35980457/cpustick.o: ../cpustick/cpustick.c  .generated_files/flags/MK_GPK/aba73bb3a24433100f974e3618e4381c0263bac4 .generated_files/flags/MK_GPK/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/35980457" 
	@${RM} ${OBJECTDIR}/_ext/35980457/cpustick.o.d 
	@${RM} ${OBJECTDIR}/_ext/35980457/cpustick.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -mmicromips -mno-float -O3 -funroll-loops -fomit-frame-pointer -fno-common -DSTICKOS -I".." -I"../pict-o-crypt" -I"../cpustick" -I"../headers" -I"../sources" -I"../stickos" -Wall -Waddress-attribute-use -MP -MMD -MF "${OBJECTDIR}/_ext/35980457/cpustick.o.d" -o ${OBJECTDIR}/_ext/35980457/cpustick.o ../cpustick/cpustick.c    -DXPRJ_MK_GPK=$(CND_CONF)    $(COMPARISON_BUILD)  -fno-builtin -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/_ext/35980457/parse2.o: ../cpustick/parse2.c  .generated_files/flags/MK_GPK/dcb6437c39190edc6f0abb9ffb6c9159360a1063 .generated_files/flags/MK_GPK/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/35980457" 
	@${RM} ${OBJECTDIR}/_ext/35980457/parse2.o.d 
	@${RM} ${OBJECTDIR}/_ext/35980457/parse2.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -mmicromips -mno-float -O3 -funroll-loops -fomit-frame-pointer -fno-common -DSTICKOS -I".." -I"../pict-o-crypt" -I"../cpustick" -I"../headers" -I"../sources" -I"../stickos" -Wall -Waddress-attribute-use -MP -MMD -MF "${OBJECTDIR}/_ext/35980457/parse2.o.d" -o ${OBJECTDIR}/_ext/35980457/parse2.o ../cpustick/parse2.c    -DXPRJ_MK_GPK=$(CND_CONF)    $(COMPARISON_BUILD)  -fno-builtin -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/_ext/35980457/run2.o: ../cpustick/run2.c  .generated_files/flags/MK_GPK/cd256f5bfa13f7c84f88b4c7a21da54ed22f8836 .generated_files/flags/MK_GPK/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/35980457" 
	@${RM} ${OBJECTDIR}/_ext/35980457/run2.o.d 
	@${RM} ${OBJECTDIR}/_ext/35980457/run2.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -mmicromips -mno-float -O3 -funroll-loops -fomit-frame-pointer -fno-common -DSTICKOS -I".." -I"../pict-o-crypt" -I"../cpustick" -I"../headers" -I"../sources" -I"../stickos" -Wall -Waddress-attribute-use -MP -MMD -MF "${OBJECTDIR}/_ext/35980457/run2.o.d" -o ${OBJECTDIR}/_ext/35980457/run2.o ../cpustick/run2.c    -DXPRJ_MK_GPK=$(CND_CONF)    $(COMPARISON_BUILD)  -fno-builtin -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/_ext/35980457/scope-command.o: ../cpustick/scope-command.c  .generated_files/flags/MK_GPK/99205ab540474a2bdb981cf20a64a0f805cf9f96 .generated_files/flags/MK_GPK/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/35980457" 
	@${RM} ${OBJECTDIR}/_ext/35980457/scope-command.o.d 
	@${RM} ${OBJECTDIR}/_ext/35980457/scope-command.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -mmicromips -mno-float -O3 -funroll-loops -fomit-frame-pointer -fno-common -DSTICKOS -I".." -I"../pict-o-crypt" -I"../cpustick" -I"../headers" -I"../sources" -I"../stickos" -Wall -Waddress-attribute-use -MP -MMD -MF "${OBJECTDIR}/_ext/35980457/scope-command.o.d" -o ${OBJECTDIR}/_ext/35980457/scope-command.o ../cpustick/scope-command.c    -DXPRJ_MK_GPK=$(CND_CONF)    $(COMPARISON_BUILD)  -fno-builtin -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/_ext/35980457/fleascope-hal.o: ../cpustick/fleascope-hal.c  .generated_files/flags/MK_GPK/5846771a8a609b7e6125f40f772f903b9ebb5a99 .generated_files/flags/MK_GPK/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/35980457" 
	@${RM} ${OBJECTDIR}/_ext/35980457/fleascope-hal.o.d 
	@${RM} ${OBJECTDIR}/_ext/35980457/fleascope-hal.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -mmicromips -mno-float -O3 -funroll-loops -fomit-frame-pointer -fno-common -DSTICKOS -I".." -I"../pict-o-crypt" -I"../cpustick" -I"../headers" -I"../sources" -I"../stickos" -Wall -Waddress-attribute-use -MP -MMD -MF "${OBJECTDIR}/_ext/35980457/fleascope-hal.o.d" -o ${OBJECTDIR}/_ext/35980457/fleascope-hal.o ../cpustick/fleascope-hal.c    -DXPRJ_MK_GPK=$(CND_CONF)    $(COMPARISON_BUILD)  -fno-builtin -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/_ext/35980457/stream-command.o: ../cpustick/stream-command.c  .generated_files/flags/MK_GPK/1dd2b15685f723897bb488495e7e5a0f7bf2b2f7 .generated_files/flags/MK_GPK/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/35980457" 
	@${RM} ${OBJECTDIR}/_ext/35980457/stream-command.o.d 
	@${RM} ${OBJECTDIR}/_ext/35980457/stream-command.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -mmicromips -mno-float -O3 -funroll-loops -fomit-frame-pointer -fno-common -DSTICKOS -I".." -I"../pict-o-crypt" -I"../cpustick" -I"../headers" -I"../sources" -I"../stickos" -Wall -Waddress-attribute-use -MP -MMD -MF "${OBJECTDIR}/_ext/35980457/stream-command.o.d" -o ${OBJECTDIR}/_ext/35980457/stream-command.o ../cpustick/stream-command.c    -DXPRJ_MK_GPK=$(CND_CONF)    $(COMPARISON_BUILD)  -fno-builtin -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/_ext/35980457/digital-stream-command.o: ../cpustick/digital-stream-command.c  .generated_files/flags/MK_GPK/1dd2b15685f723897bb488495e7e5a0f7bf2b2f7 .generated_files/flags/MK_GPK/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/35980457" 
	@${RM} ${OBJECTDIR}/_ext/35980457/digital-stream-command.o.d 
	@${RM} ${OBJECTDIR}/_ext/35980457/digital-stream-command.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -mmicromips -mno-float -O3 -funroll-loops -fomit-frame-pointer -fno-common -DSTICKOS -I".." -I"../pict-o-crypt" -I"../cpustick" -I"../headers" -I"../sources" -I"../stickos" -Wall -Waddress-attribute-use -MP -MMD -MF "${OBJECTDIR}/_ext/35980457/digital-stream-command.o.d" -o ${OBJECTDIR}/_ext/35980457/digital-stream-command.o ../cpustick/digital-stream-command.c    -DXPRJ_MK_GPK=$(CND_CONF)    $(COMPARISON_BUILD)  -fno-builtin -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/_ext/592584297/adc.o: ../sources/adc.c  .generated_files/flags/MK_GPK/e546ece33052d000bceaa19ce0674fe5c9b26d18 .generated_files/flags/MK_GPK/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/592584297" 
	@${RM} ${OBJECTDIR}/_ext/592584297/adc.o.d 
	@${RM} ${OBJECTDIR}/_ext/592584297/adc.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -mmicromips -mno-float -O3 -funroll-loops -fomit-frame-pointer -fno-common -DSTICKOS -I".." -I"../pict-o-crypt" -I"../cpustick" -I"../headers" -I"../sources" -I"../stickos" -Wall -Waddress-attribute-use -MP -MMD -MF "${OBJECTDIR}/_ext/592584297/adc.o.d" -o ${OBJECTDIR}/_ext/592584297/adc.o ../sources/adc.c    -DXPRJ_MK_GPK=$(CND_CONF)    $(COMPARISON_BUILD)  -fno-builtin -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/_ext/592584297/cdcacm.o: ../sources/cdcacm.c  .generated_files/flags/MK_GPK/1c0341d64fa71ce72d7b0ccf53f4e8f1f2a6b93 .generated_files/flags/MK_GPK/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/592584297" 
	@${RM} ${OBJECTDIR}/_ext/592584297/cdcacm.o.d 
	@${RM} ${OBJECTDIR}/_ext/592584297/cdcacm.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -mmicromips -mno-float -O3 -funroll-loops -fomit-frame-pointer -fno-common -DSTICKOS -I".." -I"../pict-o-crypt" -I"../cpustick" -I"../headers" -I"../sources" -I"../stickos" -Wall -Waddress-attribute-use -MP -MMD -MF "${OBJECTDIR}/_ext/592584297/cdcacm.o.d" -o ${OBJECTDIR}/_ext/592584297/cdcacm.o ../sources/cdcacm.c    -DXPRJ_MK_GPK=$(CND_CONF)    $(COMPARISON_BUILD)  -fno-builtin -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/_ext/592584297/flash.o: ../sources/flash.c  .generated_files/flags/MK_GPK/108202e3a006996a1d172a55ed223809ef878088 .generated_files/flags/MK_GPK/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/592584297" 
	@${RM} ${OBJECTDIR}/_ext/592584297/flash.o.d 
	@${RM} ${OBJECTDIR}/_ext/592584297/flash.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -mmicromips -mno-float -O3 -funroll-loops -fomit-frame-pointer -fno-common -DSTICKOS -I".." -I"../pict-o-crypt" -I"../cpustick" -I"../headers" -I"../sources" -I"../stickos" -Wall -Waddress-attribute-use -MP -MMD -MF "${OBJECTDIR}/_ext/592584297/flash.o.d" -o ${OBJECTDIR}/_ext/592584297/flash.o ../sources/flash.c    -DXPRJ_MK_GPK=$(CND_CONF)    $(COMPARISON_BUILD)  -fno-builtin -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/_ext/592584297/i2c.o: ../sources/i2c.c  .generated_files/flags/MK_GPK/69086416fdc4bc264b8bde9bac7840ac8d5f75ea .generated_files/flags/MK_GPK/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/592584297" 
	@${RM} ${OBJECTDIR}/_ext/592584297/i2c.o.d 
	@${RM} ${OBJECTDIR}/_ext/592584297/i2c.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -mmicromips -mno-float -O3 -funroll-loops -fomit-frame-pointer -fno-common -DSTICKOS -I".." -I"../pict-o-crypt" -I"../cpustick" -I"../headers" -I"../sources" -I"../stickos" -Wall -Waddress-attribute-use -MP -MMD -MF "${OBJECTDIR}/_ext/592584297/i2c.o.d" -o ${OBJECTDIR}/_ext/592584297/i2c.o ../sources/i2c.c    -DXPRJ_MK_GPK=$(CND_CONF)    $(COMPARISON_BUILD)  -fno-builtin -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/_ext/592584297/led.o: ../sources/led.c  .generated_files/flags/MK_GPK/26873e2260eefb3793189c7a12df25458b22144b .generated_files/flags/MK_GPK/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/592584297" 
	@${RM} ${OBJECTDIR}/_ext/592584297/led.o.d 
	@${RM} ${OBJECTDIR}/_ext/592584297/led.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -mmicromips -mno-float -O3 -funroll-loops -fomit-frame-pointer -fno-common -DSTICKOS -I".." -I"../pict-o-crypt" -I"../cpustick" -I"../headers" -I"../sources" -I"../stickos" -Wall -Waddress-attribute-use -MP -MMD -MF "${OBJECTDIR}/_ext/592584297/led.o.d" -o ${OBJECTDIR}/_ext/592584297/led.o ../sources/led.c    -DXPRJ_MK_GPK=$(CND_CONF)    $(COMPARISON_BUILD)  -fno-builtin -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/_ext/592584297/pin.o: ../sources/pin.c  .generated_files/flags/MK_GPK/8b6ce82677ac11fad46253de1ac28d169e827a79 .generated_files/flags/MK_GPK/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/592584297" 
	@${RM} ${OBJECTDIR}/_ext/592584297/pin.o.d 
	@${RM} ${OBJECTDIR}/_ext/592584297/pin.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -mmicromips -mno-float -O3 -funroll-loops -fomit-frame-pointer -fno-common -DSTICKOS -I".." -I"../pict-o-crypt" -I"../cpustick" -I"../headers" -I"../sources" -I"../stickos" -Wall -Waddress-attribute-use -MP -MMD -MF "${OBJECTDIR}/_ext/592584297/pin.o.d" -o ${OBJECTDIR}/_ext/592584297/pin.o ../sources/pin.c    -DXPRJ_MK_GPK=$(CND_CONF)    $(COMPARISON_BUILD)  -fno-builtin -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/_ext/592584297/printf.o: ../sources/printf.c  .generated_files/flags/MK_GPK/6da0329079f90f1148f969dfa14b79559a0a5f80 .generated_files/flags/MK_GPK/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/592584297" 
	@${RM} ${OBJECTDIR}/_ext/592584297/printf.o.d 
	@${RM} ${OBJECTDIR}/_ext/592584297/printf.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -mmicromips -mno-float -O3 -funroll-loops -fomit-frame-pointer -fno-common -DSTICKOS -I".." -I"../pict-o-crypt" -I"../cpustick" -I"../headers" -I"../sources" -I"../stickos" -Wall -Waddress-attribute-use -MP -MMD -MF "${OBJECTDIR}/_ext/592584297/printf.o.d" -o ${OBJECTDIR}/_ext/592584297/printf.o ../sources/printf.c    -DXPRJ_MK_GPK=$(CND_CONF)    $(COMPARISON_BUILD)  -fno-builtin -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/_ext/592584297/qspi.o: ../sources/qspi.c  .generated_files/flags/MK_GPK/16ab457b6d142756c8c682857ee27739dc127145 .generated_files/flags/MK_GPK/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/592584297" 
	@${RM} ${OBJECTDIR}/_ext/592584297/qspi.o.d 
	@${RM} ${OBJECTDIR}/_ext/592584297/qspi.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -mmicromips -mno-float -O3 -funroll-loops -fomit-frame-pointer -fno-common -DSTICKOS -I".." -I"../pict-o-crypt" -I"../cpustick" -I"../headers" -I"../sources" -I"../stickos" -Wall -Waddress-attribute-use -MP -MMD -MF "${OBJECTDIR}/_ext/592584297/qspi.o.d" -o ${OBJECTDIR}/_ext/592584297/qspi.o ../sources/qspi.c    -DXPRJ_MK_GPK=$(CND_CONF)    $(COMPARISON_BUILD)  -fno-builtin -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/_ext/592584297/serial.o: ../sources/serial.c  .generated_files/flags/MK_GPK/cbe889b221792ca113f5ae87c0c0ea83e307086e .generated_files/flags/MK_GPK/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/592584297" 
	@${RM} ${OBJECTDIR}/_ext/592584297/serial.o.d 
	@${RM} ${OBJECTDIR}/_ext/592584297/serial.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -mmicromips -mno-float -O3 -funroll-loops -fomit-frame-pointer -fno-common -DSTICKOS -I".." -I"../pict-o-crypt" -I"../cpustick" -I"../headers" -I"../sources" -I"../stickos" -Wall -Waddress-attribute-use -MP -MMD -MF "${OBJECTDIR}/_ext/592584297/serial.o.d" -o ${OBJECTDIR}/_ext/592584297/serial.o ../sources/serial.c    -DXPRJ_MK_GPK=$(CND_CONF)    $(COMPARISON_BUILD)  -fno-builtin -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/_ext/592584297/terminal.o: ../sources/terminal.c  .generated_files/flags/MK_GPK/87627ef0cb010b22b62c77b442d3eeb6e52c0fe3 .generated_files/flags/MK_GPK/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/592584297" 
	@${RM} ${OBJECTDIR}/_ext/592584297/terminal.o.d 
	@${RM} ${OBJECTDIR}/_ext/592584297/terminal.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -mmicromips -mno-float -O3 -funroll-loops -fomit-frame-pointer -fno-common -DSTICKOS -I".." -I"../pict-o-crypt" -I"../cpustick" -I"../headers" -I"../sources" -I"../stickos" -Wall -Waddress-attribute-use -MP -MMD -MF "${OBJECTDIR}/_ext/592584297/terminal.o.d" -o ${OBJECTDIR}/_ext/592584297/terminal.o ../sources/terminal.c    -DXPRJ_MK_GPK=$(CND_CONF)    $(COMPARISON_BUILD)  -fno-builtin -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/_ext/592584297/text.o: ../sources/text.c  .generated_files/flags/MK_GPK/c6a879c2c783216386abf1bb319b75fdb9144068 .generated_files/flags/MK_GPK/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/592584297" 
	@${RM} ${OBJECTDIR}/_ext/592584297/text.o.d 
	@${RM} ${OBJECTDIR}/_ext/592584297/text.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -mmicromips -mno-float -O3 -funroll-loops -fomit-frame-pointer -fno-common -DSTICKOS -I".." -I"../pict-o-crypt" -I"../cpustick" -I"../headers" -I"../sources" -I"../stickos" -Wall -Waddress-attribute-use -MP -MMD -MF "${OBJECTDIR}/_ext/592584297/text.o.d" -o ${OBJECTDIR}/_ext/592584297/text.o ../sources/text.c    -DXPRJ_MK_GPK=$(CND_CONF)    $(COMPARISON_BUILD)  -fno-builtin -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/_ext/592584297/timer.o: ../sources/timer.c  .generated_files/flags/MK_GPK/8c09edf82b37e6b0456533f88d5d8262d120e184 .generated_files/flags/MK_GPK/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/592584297" 
	@${RM} ${OBJECTDIR}/_ext/592584297/timer.o.d 
	@${RM} ${OBJECTDIR}/_ext/592584297/timer.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -mmicromips -mno-float -O3 -funroll-loops -fomit-frame-pointer -fno-common -DSTICKOS -I".." -I"../pict-o-crypt" -I"../cpustick" -I"../headers" -I"../sources" -I"../stickos" -Wall -Waddress-attribute-use -MP -MMD -MF "${OBJECTDIR}/_ext/592584297/timer.o.d" -o ${OBJECTDIR}/_ext/592584297/timer.o ../sources/timer.c    -DXPRJ_MK_GPK=$(CND_CONF)    $(COMPARISON_BUILD)  -fno-builtin -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/_ext/592584297/usb.o: ../sources/usb.c  .generated_files/flags/MK_GPK/4ad55e86d9f606b7dd6e4536f592b8d8f7dbfeb0 .generated_files/flags/MK_GPK/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/592584297" 
	@${RM} ${OBJECTDIR}/_ext/592584297/usb.o.d 
	@${RM} ${OBJECTDIR}/_ext/592584297/usb.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -mmicromips -mno-float -O3 -funroll-loops -fomit-frame-pointer -fno-common -DSTICKOS -I".." -I"../pict-o-crypt" -I"../cpustick" -I"../headers" -I"../sources" -I"../stickos" -Wall -Waddress-attribute-use -MP -MMD -MF "${OBJECTDIR}/_ext/592584297/usb.o.d" -o ${OBJECTDIR}/_ext/592584297/usb.o ../sources/usb.c    -DXPRJ_MK_GPK=$(CND_CONF)    $(COMPARISON_BUILD)  -fno-builtin -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/_ext/592584297/util.o: ../sources/util.c  .generated_files/flags/MK_GPK/797d6397d634d05a23cbbcd51cf0860201a237d9 .generated_files/flags/MK_GPK/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/592584297" 
	@${RM} ${OBJECTDIR}/_ext/592584297/util.o.d 
	@${RM} ${OBJECTDIR}/_ext/592584297/util.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -mmicromips -mno-float -O3 -funroll-loops -fomit-frame-pointer -fno-common -DSTICKOS -I".." -I"../pict-o-crypt" -I"../cpustick" -I"../headers" -I"../sources" -I"../stickos" -Wall -Waddress-attribute-use -MP -MMD -MF "${OBJECTDIR}/_ext/592584297/util.o.d" -o ${OBJECTDIR}/_ext/592584297/util.o ../sources/util.c    -DXPRJ_MK_GPK=$(CND_CONF)    $(COMPARISON_BUILD)  -fno-builtin -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/_ext/592584297/zigflea.o: ../sources/zigflea.c  .generated_files/flags/MK_GPK/9d64832f1a17b042d0e0bb18a81184718c870d56 .generated_files/flags/MK_GPK/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/592584297" 
	@${RM} ${OBJECTDIR}/_ext/592584297/zigflea.o.d 
	@${RM} ${OBJECTDIR}/_ext/592584297/zigflea.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -mmicromips -mno-float -O3 -funroll-loops -fomit-frame-pointer -fno-common -DSTICKOS -I".." -I"../pict-o-crypt" -I"../cpustick" -I"../headers" -I"../sources" -I"../stickos" -Wall -Waddress-attribute-use -MP -MMD -MF "${OBJECTDIR}/_ext/592584297/zigflea.o.d" -o ${OBJECTDIR}/_ext/592584297/zigflea.o ../sources/zigflea.c    -DXPRJ_MK_GPK=$(CND_CONF)    $(COMPARISON_BUILD)  -fno-builtin -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/_ext/724208933/basic.o: ../stickos/basic.c  .generated_files/flags/MK_GPK/c14fa0cb8959e1c1d302b1a4bc15c47bcb9eed86 .generated_files/flags/MK_GPK/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/724208933" 
	@${RM} ${OBJECTDIR}/_ext/724208933/basic.o.d 
	@${RM} ${OBJECTDIR}/_ext/724208933/basic.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -mmicromips -mno-float -O3 -funroll-loops -fomit-frame-pointer -fno-common -DSTICKOS -I".." -I"../pict-o-crypt" -I"../cpustick" -I"../headers" -I"../sources" -I"../stickos" -Wall -Waddress-attribute-use -MP -MMD -MF "${OBJECTDIR}/_ext/724208933/basic.o.d" -o ${OBJECTDIR}/_ext/724208933/basic.o ../stickos/basic.c    -DXPRJ_MK_GPK=$(CND_CONF)    $(COMPARISON_BUILD)  -fno-builtin -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/_ext/724208933/code.o: ../stickos/code.c  .generated_files/flags/MK_GPK/2fce8ee6c64454c8f1ade245f170419549695fc1 .generated_files/flags/MK_GPK/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/724208933" 
	@${RM} ${OBJECTDIR}/_ext/724208933/code.o.d 
	@${RM} ${OBJECTDIR}/_ext/724208933/code.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -mmicromips -mno-float -O3 -funroll-loops -fomit-frame-pointer -fno-common -DSTICKOS -I".." -I"../pict-o-crypt" -I"../cpustick" -I"../headers" -I"../sources" -I"../stickos" -Wall -Waddress-attribute-use -MP -MMD -MF "${OBJECTDIR}/_ext/724208933/code.o.d" -o ${OBJECTDIR}/_ext/724208933/code.o ../stickos/code.c    -DXPRJ_MK_GPK=$(CND_CONF)    $(COMPARISON_BUILD)  -fno-builtin -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/_ext/724208933/parse.o: ../stickos/parse.c  .generated_files/flags/MK_GPK/1c7224275f0f9843b55383ad6018a3bbda8f385d .generated_files/flags/MK_GPK/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/724208933" 
	@${RM} ${OBJECTDIR}/_ext/724208933/parse.o.d 
	@${RM} ${OBJECTDIR}/_ext/724208933/parse.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -mmicromips -mno-float -O3 -funroll-loops -fomit-frame-pointer -fno-common -DSTICKOS -I".." -I"../pict-o-crypt" -I"../cpustick" -I"../headers" -I"../sources" -I"../stickos" -Wall -Waddress-attribute-use -MP -MMD -MF "${OBJECTDIR}/_ext/724208933/parse.o.d" -o ${OBJECTDIR}/_ext/724208933/parse.o ../stickos/parse.c    -DXPRJ_MK_GPK=$(CND_CONF)    $(COMPARISON_BUILD)  -fno-builtin -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/_ext/724208933/run.o: ../stickos/run.c  .generated_files/flags/MK_GPK/5fdddc69dc78e8e12e821885ba82823cfe00981 .generated_files/flags/MK_GPK/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/724208933" 
	@${RM} ${OBJECTDIR}/_ext/724208933/run.o.d 
	@${RM} ${OBJECTDIR}/_ext/724208933/run.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -mmicromips -mno-float -O3 -funroll-loops -fomit-frame-pointer -fno-common -DSTICKOS -I".." -I"../pict-o-crypt" -I"../cpustick" -I"../headers" -I"../sources" -I"../stickos" -Wall -Waddress-attribute-use -MP -MMD -MF "${OBJECTDIR}/_ext/724208933/run.o.d" -o ${OBJECTDIR}/_ext/724208933/run.o ../stickos/run.c    -DXPRJ_MK_GPK=$(CND_CONF)    $(COMPARISON_BUILD)  -fno-builtin -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/_ext/724208933/vars.o: ../stickos/vars.c  .generated_files/flags/MK_GPK/623e28011aaae868223c3194510f645e9e47c8d6 .generated_files/flags/MK_GPK/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/724208933" 
	@${RM} ${OBJECTDIR}/_ext/724208933/vars.o.d 
	@${RM} ${OBJECTDIR}/_ext/724208933/vars.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -mmicromips -mno-float -O3 -funroll-loops -fomit-frame-pointer -fno-common -DSTICKOS -I".." -I"../pict-o-crypt" -I"../cpustick" -I"../headers" -I"../sources" -I"../stickos" -Wall -Waddress-attribute-use -MP -MMD -MF "${OBJECTDIR}/_ext/724208933/vars.o.d" -o ${OBJECTDIR}/_ext/724208933/vars.o ../stickos/vars.c    -DXPRJ_MK_GPK=$(CND_CONF)    $(COMPARISON_BUILD)  -fno-builtin -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/_ext/1472/main.o: ../main.c  .generated_files/flags/MK_GPK/df3e948d259c913ab6d563858f2bf43d8ee47710 .generated_files/flags/MK_GPK/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/1472" 
	@${RM} ${OBJECTDIR}/_ext/1472/main.o.d 
	@${RM} ${OBJECTDIR}/_ext/1472/main.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -mmicromips -mno-float -O3 -funroll-loops -fomit-frame-pointer -fno-common -DSTICKOS -I".." -I"../pict-o-crypt" -I"../cpustick" -I"../headers" -I"../sources" -I"../stickos" -Wall -Waddress-attribute-use -MP -MMD -MF "${OBJECTDIR}/_ext/1472/main.o.d" -o ${OBJECTDIR}/_ext/1472/main.o ../main.c    -DXPRJ_MK_GPK=$(CND_CONF)    $(COMPARISON_BUILD)  -fno-builtin -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/_ext/1472/myplib.o: ../myplib.c  .generated_files/flags/MK_GPK/a12b929c082bd430145d3e2d45b4de46ff01b56c .generated_files/flags/MK_GPK/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/1472" 
	@${RM} ${OBJECTDIR}/_ext/1472/myplib.o.d 
	@${RM} ${OBJECTDIR}/_ext/1472/myplib.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -mmicromips -mno-float -O3 -funroll-loops -fomit-frame-pointer -fno-common -DSTICKOS -I".." -I"../pict-o-crypt" -I"../cpustick" -I"../headers" -I"../sources" -I"../stickos" -Wall -Waddress-attribute-use -MP -MMD -MF "${OBJECTDIR}/_ext/1472/myplib.o.d" -o ${OBJECTDIR}/_ext/1472/myplib.o ../myplib.c    -DXPRJ_MK_GPK=$(CND_CONF)    $(COMPARISON_BUILD)  -fno-builtin -mdfp="${DFP_DIR}"  
	
endif

# ------------------------------------------------------------------------------------
# Rules for buildStep: compileCPP
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
else
endif

# ------------------------------------------------------------------------------------
# Rules for buildStep: link
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
${DISTDIR}/pic32.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}: ${OBJECTFILES}  nbproject/Makefile-${CND_CONF}.mk    
	@${MKDIR} ${DISTDIR} 
	${MP_CC} $(MP_EXTRA_LD_PRE) -g   -mprocessor=$(MP_PROCESSOR_OPTION)  -o ${DISTDIR}/pic32.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX} ${OBJECTFILES_QUOTED_IF_SPACED}          -DXPRJ_MK_GPK=$(CND_CONF)    $(COMPARISON_BUILD)      -Wl,--defsym=__MPLAB_BUILD=1$(MP_EXTRA_LD_POST)$(MP_LINKER_FILE_OPTION),--defsym=__MPLAB_DEBUG=1,--defsym=__DEBUG=1,-D=__DEBUG_D,--defsym=_min_heap_size=0,--defsym=_min_stack_size=3072,--gc-sections,--no-code-in-dinit,--no-dinit-in-serial-mem,-L"../../../../Program Files/Microchip/MPLAB C32/lib",-L"../../../../Program Files/Microchip/MPLAB C32/pic32mx/lib",-Map="$(BINDIR_)$(TARGETBASE).map",--memorysummary,${DISTDIR}/memoryfile.xml -mdfp="${DFP_DIR}"
	
else
${DISTDIR}/pic32.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}: ${OBJECTFILES}  nbproject/Makefile-${CND_CONF}.mk   
	@${MKDIR} ${DISTDIR} 
	${MP_CC} $(MP_EXTRA_LD_PRE)  -mprocessor=$(MP_PROCESSOR_OPTION)  -o ${DISTDIR}/pic32.X.${IMAGE_TYPE}.${DEBUGGABLE_SUFFIX} ${OBJECTFILES_QUOTED_IF_SPACED}          -DXPRJ_MK_GPK=$(CND_CONF)    $(COMPARISON_BUILD)  -Wl,--defsym=__MPLAB_BUILD=1$(MP_EXTRA_LD_POST)$(MP_LINKER_FILE_OPTION),--defsym=_min_heap_size=0,--defsym=_min_stack_size=3072,--gc-sections,--no-code-in-dinit,--no-dinit-in-serial-mem,-L"../../../../Program Files/Microchip/MPLAB C32/lib",-L"../../../../Program Files/Microchip/MPLAB C32/pic32mx/lib",-Map="$(BINDIR_)$(TARGETBASE).map",--memorysummary,${DISTDIR}/memoryfile.xml -mdfp="${DFP_DIR}"
	${MP_CC_DIR}/xc32-bin2hex ${DISTDIR}/pic32.X.${IMAGE_TYPE}.${DEBUGGABLE_SUFFIX} 
endif


# Subprojects
.build-subprojects:


# Subprojects
.clean-subprojects:

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r ${OBJECTDIR}
	${RM} -r ${DISTDIR}

# Enable dependency checking
.dep.inc: .depcheck-impl

DEPFILES=$(wildcard ${POSSIBLE_DEPFILES})
ifneq (${DEPFILES},)
include ${DEPFILES}
endif
