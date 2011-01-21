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
CC=i586-mingw32msvc-gcc
CCC=i586-mingw32msvc-g++
CXX=i586-mingw32msvc-g++
FC=

# Macros
PLATFORM=MinGW-Windows

# Include project Makefile
include Makefile

# Object Directory
OBJECTDIR=build/Release/${PLATFORM}

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

# Link Libraries and Options
LDLIBSOPTIONS=

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	${MAKE}  -f nbproject/Makefile-Release.mk dist/Release/${PLATFORM}/sjvm.exe

dist/Release/${PLATFORM}/sjvm.exe: ${OBJECTFILES}
	${MKDIR} -p dist/Release/${PLATFORM}
	${LINK.cc} -o dist/Release/${PLATFORM}/sjvm ${OBJECTFILES} ${LDLIBSOPTIONS} 

${OBJECTDIR}/jni.o: jni.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -O2 -MMD -MP -MF $@.d -o ${OBJECTDIR}/jni.o jni.cpp

${OBJECTDIR}/LeakTracer.o: LeakTracer.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -O2 -MMD -MP -MF $@.d -o ${OBJECTDIR}/LeakTracer.o LeakTracer.cpp

${OBJECTDIR}/thread.o: thread.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -O2 -MMD -MP -MF $@.d -o ${OBJECTDIR}/thread.o thread.cpp

${OBJECTDIR}/cpinfo.o: cpinfo.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -O2 -MMD -MP -MF $@.d -o ${OBJECTDIR}/cpinfo.o cpinfo.cpp

${OBJECTDIR}/main.o: main.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -O2 -MMD -MP -MF $@.d -o ${OBJECTDIR}/main.o main.cpp

${OBJECTDIR}/jar.o: jar.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -O2 -MMD -MP -MF $@.d -o ${OBJECTDIR}/jar.o jar.cpp

${OBJECTDIR}/sjvm.o: sjvm.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -O2 -MMD -MP -MF $@.d -o ${OBJECTDIR}/sjvm.o sjvm.cpp

${OBJECTDIR}/mutex.o: mutex.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -O2 -MMD -MP -MF $@.d -o ${OBJECTDIR}/mutex.o mutex.cpp

${OBJECTDIR}/native.o: native.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -O2 -MMD -MP -MF $@.d -o ${OBJECTDIR}/native.o native.cpp

${OBJECTDIR}/classloader.o: classloader.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -O2 -MMD -MP -MF $@.d -o ${OBJECTDIR}/classloader.o classloader.cpp

${OBJECTDIR}/garbagecollector.o: garbagecollector.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -O2 -MMD -MP -MF $@.d -o ${OBJECTDIR}/garbagecollector.o garbagecollector.cpp

${OBJECTDIR}/object.o: object.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -O2 -MMD -MP -MF $@.d -o ${OBJECTDIR}/object.o object.cpp

${OBJECTDIR}/fieldinfo.o: fieldinfo.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -O2 -MMD -MP -MF $@.d -o ${OBJECTDIR}/fieldinfo.o fieldinfo.cpp

${OBJECTDIR}/gnuclasspath.o: gnuclasspath.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -O2 -MMD -MP -MF $@.d -o ${OBJECTDIR}/gnuclasspath.o gnuclasspath.cpp

${OBJECTDIR}/attributeinfo.o: attributeinfo.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -O2 -MMD -MP -MF $@.d -o ${OBJECTDIR}/attributeinfo.o attributeinfo.cpp

${OBJECTDIR}/classfile.o: classfile.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -O2 -MMD -MP -MF $@.d -o ${OBJECTDIR}/classfile.o classfile.cpp

${OBJECTDIR}/inflate.o: inflate.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -O2 -MMD -MP -MF $@.d -o ${OBJECTDIR}/inflate.o inflate.c

${OBJECTDIR}/methodinfo.o: methodinfo.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -O2 -MMD -MP -MF $@.d -o ${OBJECTDIR}/methodinfo.o methodinfo.cpp

${OBJECTDIR}/exception.o: exception.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -O2 -MMD -MP -MF $@.d -o ${OBJECTDIR}/exception.o exception.cpp

# Subprojects
.build-subprojects:

# Clean Targets
.clean-conf:
	${RM} -r build/Release
	${RM} dist/Release/${PLATFORM}/sjvm.exe

# Subprojects
.clean-subprojects:

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc
