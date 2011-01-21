#
# Generated Makefile - do not edit!
#
# Edit the Makefile in the project folder instead (../Makefile). Each target
# has a -pre and a -post target defined where you can add customized code.
#
# This makefile implements configuration specific macros and targets.


# Environment
MKDIR=mkdir
CP=cp
CCADMIN=CCadmin
RANLIB=ranlib
CC=gcc
CCC=g++
CXX=g++
FC=
AS=as

# Macros
CND_PLATFORM=GNU-Linux-x86
CND_CONF=Debug
CND_DISTDIR=dist

# Include project Makefile
include Makefile

# Object Directory
OBJECTDIR=build/${CND_CONF}/${CND_PLATFORM}

# Object Files
OBJECTFILES= \
	${OBJECTDIR}/jni.o \
	${OBJECTDIR}/LeakTracer.o \
	${OBJECTDIR}/thread.o \
	${OBJECTDIR}/cpinfo.o \
	${OBJECTDIR}/main.o \
	${OBJECTDIR}/jar.o \
	${OBJECTDIR}/sjvm.o \
	${OBJECTDIR}/mutex.o \
	${OBJECTDIR}/native.o \
	${OBJECTDIR}/classloader.o \
	${OBJECTDIR}/garbagecollector.o \
	${OBJECTDIR}/object.o \
	${OBJECTDIR}/fieldinfo.o \
	${OBJECTDIR}/gnuclasspath.o \
	${OBJECTDIR}/attributeinfo.o \
	${OBJECTDIR}/classfile.o \
	${OBJECTDIR}/inflate.o \
	${OBJECTDIR}/methodinfo.o \
	${OBJECTDIR}/exception.o

# C Compiler Flags
CFLAGS=

# CC Compiler Flags
CCFLAGS=
CXXFLAGS=

# Fortran Compiler Flags
FFLAGS=

# Assembler Flags
ASFLAGS=

# Link Libraries and Options
LDLIBSOPTIONS=

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	${MAKE}  -f nbproject/Makefile-Debug.mk dist/Debug/sjvm

dist/Debug/sjvm: ${OBJECTFILES}
	${MKDIR} -p dist/Debug
	${LINK.cc} -ldl -export-dynamic -o dist/Debug/sjvm ${OBJECTFILES} ${LDLIBSOPTIONS} 

${OBJECTDIR}/jni.o: nbproject/Makefile-${CND_CONF}.mk jni.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -MMD -MP -MF $@.d -o ${OBJECTDIR}/jni.o jni.cpp

${OBJECTDIR}/LeakTracer.o: nbproject/Makefile-${CND_CONF}.mk LeakTracer.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -MMD -MP -MF $@.d -o ${OBJECTDIR}/LeakTracer.o LeakTracer.cpp

${OBJECTDIR}/thread.o: nbproject/Makefile-${CND_CONF}.mk thread.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -MMD -MP -MF $@.d -o ${OBJECTDIR}/thread.o thread.cpp

${OBJECTDIR}/cpinfo.o: nbproject/Makefile-${CND_CONF}.mk cpinfo.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -MMD -MP -MF $@.d -o ${OBJECTDIR}/cpinfo.o cpinfo.cpp

${OBJECTDIR}/main.o: nbproject/Makefile-${CND_CONF}.mk main.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -MMD -MP -MF $@.d -o ${OBJECTDIR}/main.o main.cpp

${OBJECTDIR}/jar.o: nbproject/Makefile-${CND_CONF}.mk jar.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -MMD -MP -MF $@.d -o ${OBJECTDIR}/jar.o jar.cpp

${OBJECTDIR}/sjvm.o: nbproject/Makefile-${CND_CONF}.mk sjvm.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -MMD -MP -MF $@.d -o ${OBJECTDIR}/sjvm.o sjvm.cpp

${OBJECTDIR}/mutex.o: nbproject/Makefile-${CND_CONF}.mk mutex.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -MMD -MP -MF $@.d -o ${OBJECTDIR}/mutex.o mutex.cpp

${OBJECTDIR}/native.o: nbproject/Makefile-${CND_CONF}.mk native.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -MMD -MP -MF $@.d -o ${OBJECTDIR}/native.o native.cpp

${OBJECTDIR}/classloader.o: nbproject/Makefile-${CND_CONF}.mk classloader.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -MMD -MP -MF $@.d -o ${OBJECTDIR}/classloader.o classloader.cpp

${OBJECTDIR}/garbagecollector.o: nbproject/Makefile-${CND_CONF}.mk garbagecollector.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -MMD -MP -MF $@.d -o ${OBJECTDIR}/garbagecollector.o garbagecollector.cpp

${OBJECTDIR}/object.o: nbproject/Makefile-${CND_CONF}.mk object.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -MMD -MP -MF $@.d -o ${OBJECTDIR}/object.o object.cpp

${OBJECTDIR}/fieldinfo.o: nbproject/Makefile-${CND_CONF}.mk fieldinfo.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -MMD -MP -MF $@.d -o ${OBJECTDIR}/fieldinfo.o fieldinfo.cpp

${OBJECTDIR}/gnuclasspath.o: nbproject/Makefile-${CND_CONF}.mk gnuclasspath.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -MMD -MP -MF $@.d -o ${OBJECTDIR}/gnuclasspath.o gnuclasspath.cpp

${OBJECTDIR}/attributeinfo.o: nbproject/Makefile-${CND_CONF}.mk attributeinfo.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -MMD -MP -MF $@.d -o ${OBJECTDIR}/attributeinfo.o attributeinfo.cpp

${OBJECTDIR}/classfile.o: nbproject/Makefile-${CND_CONF}.mk classfile.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -MMD -MP -MF $@.d -o ${OBJECTDIR}/classfile.o classfile.cpp

${OBJECTDIR}/inflate.o: nbproject/Makefile-${CND_CONF}.mk inflate.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -g -MMD -MP -MF $@.d -o ${OBJECTDIR}/inflate.o inflate.c

${OBJECTDIR}/methodinfo.o: nbproject/Makefile-${CND_CONF}.mk methodinfo.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -MMD -MP -MF $@.d -o ${OBJECTDIR}/methodinfo.o methodinfo.cpp

${OBJECTDIR}/exception.o: nbproject/Makefile-${CND_CONF}.mk exception.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -MMD -MP -MF $@.d -o ${OBJECTDIR}/exception.o exception.cpp

# Subprojects
.build-subprojects:

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r build/Debug
	${RM} dist/Debug/sjvm

# Subprojects
.clean-subprojects:

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc
