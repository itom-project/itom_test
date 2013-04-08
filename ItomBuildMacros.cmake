#SET(ITOM_SDK_DIR "${ITOM_DIR}/SDK" CACHE PATH "base path to the sdk directory of itom")

#########################################################################
#set general things
#########################################################################
OPTION(BUILD_TARGET64 "Build for 64 bit target if set to ON or 32 bit if set to OFF." OFF) 

if (BUILD_TARGET64)
	set(CMAKE_SIZEOF_VOID_P 8)
else (BUILD_TARGET64)
	set(CMAKE_SIZEOF_VOID_P 4)
endif (BUILD_TARGET64)


#on windows systems, replace WIN32 preprocessor by _WIN64 if on 64bit
if(CMAKE_HOST_WIN32)
	if(CMAKE_SIZEOF_VOID_P EQUAL 8)
		string (REPLACE "/DWIN32" "/D_WIN64" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS}) 
	else() 
		#ok
	endif()
endif()

#on MSVC enable build using OpenMP for compiling
if(MSVC)
	ADD_DEFINITIONS(/MP)

	# set some optimization compiler flags
	# i.e.:
	#   - Ox full optimization (replaces standard O2 set by cmake)
	#	- Oi enable intrinsic functions
	#	- Ot favor fast code
	#	- Oy omit frame pointers
	#	- GL whole program optimization
	# 	- GT fibre safe optimization
	#	- openmp enable openmp support, isn't enabled globally here as it breaks opencv
	set ( CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Oi /Ot /Oy /GL /openmp" )
endif (MSVC)

MACRO (BUILD_PARALLEL_LINUX targetName)
  if(CMAKE_COMPILER_IS_GNUCXX)
	  message(STATUS "GNUCXX pipe flag enabled")
	  set_target_properties(${targetName} PROPERTIES COMPILE_FLAGS "-pipe")
  endif(CMAKE_COMPILER_IS_GNUCXX)
ENDMACRO (BUILD_PARALLEL_LINUX)


###########################################################################
# useful macros
###########################################################################

# using custom macro for qtCreator compability, i.e. put ui files into GeneratedFiles/ folder
MACRO (QT4_WRAP_UI_ITOM outfiles)
	QT4_EXTRACT_OPTIONS(ui_files ui_options ${ARGN})

	file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/GeneratedFiles)

	FOREACH (it ${ui_files})
	GET_FILENAME_COMPONENT(outfile ${it} NAME_WE)
	GET_FILENAME_COMPONENT(infile ${it} ABSOLUTE)
	SET(outfile ${CMAKE_CURRENT_BINARY_DIR}/ui_${outfile}.h) # Here we set output
	ADD_CUSTOM_COMMAND(OUTPUT ${outfile}
		COMMAND ${QT_UIC_EXECUTABLE}
		ARGS ${ui_options} -o ${outfile} ${infile}
		MAIN_DEPENDENCY ${infile})
	SET(${outfiles} ${${outfiles}} ${outfile})
	ENDFOREACH (it)

	SOURCE_GROUP("GeneratedFiles" FILES ${${outfiles}})
ENDMACRO (QT4_WRAP_UI_ITOM)


MACRO (QT4_WRAP_CPP_ITOM outfiles )
	# get include dirs
	QT4_GET_MOC_FLAGS(moc_flags)
	QT4_EXTRACT_OPTIONS(moc_files moc_options ${ARGN})

	foreach (it ${moc_files})
		GET_FILENAME_COMPONENT(it ${it} ABSOLUTE)
		QT4_MAKE_OUTPUT_FILE(${it} moc_ cxx outfile)
		QT4_CREATE_MOC_COMMAND(${it} ${outfile} "${moc_flags}" "${moc_options}")
		set(${outfiles} ${${outfiles}} ${outfile})
	endforeach()

	SOURCE_GROUP("GeneratedFiles" FILES ${${outfiles}})
ENDMACRO ()


MACRO(QT4_CREATE_TRANSLATION_ITOM outputFiles tsFiles target languages)
	QT4_EXTRACT_OPTIONS(_lupdate_files _lupdate_options ${ARGN})
	
	set(_my_sources)
	set(_my_dirs)
	set(_my_tsfiles)
	set(_ts_pro)
	
	#reset tsFiles
	set(${tsFiles} "")
	
	foreach (_file ${_lupdate_files})
		get_filename_component(_ext ${_file} EXT)
		get_filename_component(_abs_FILE ${_file} ABSOLUTE)
		if(_ext MATCHES "ts")
			list(APPEND _my_tsfiles ${_abs_FILE})
		else()
			if(NOT _ext)
				list(APPEND _my_dirs ${_abs_FILE})
			else()
				list(APPEND _my_sources ${_abs_FILE})
			endif()
		endif()
	endforeach()
	
	#message(STATUS "native ts ${_my_tsfiles}")
	
	foreach( _lang ${${languages}})
		set(_tsFile ${CMAKE_CURRENT_SOURCE_DIR}/translation/${target}_${_lang}.ts)
		#message(STATUS "scan ${_tsFile}")
		get_filename_component(_ext ${_tsFile} EXT)
		get_filename_component(_abs_FILE ${_tsFile} ABSOLUTE)
		if(EXISTS ${_abs_FILE})
			list(APPEND _my_tsfiles ${_abs_FILE})
		else()
			#message(STATUS "...ist aber nicht da")
			#create new ts file
			add_custom_command(OUTPUT ${_abs_FILE}_new
				COMMAND ${QT_LUPDATE_EXECUTABLE}
				ARGS ${_lupdate_options} ${_my_dirs} -locations relative -no-ui-lines -target-language ${_lang} -ts ${_abs_FILE}
				DEPENDS ${_my_sources} VERBATIM)
			list(APPEND _my_tsfiles ${_abs_FILE})
			set(${outputFiles} ${${outputFiles}} ${_abs_FILE}_new) #add output file for custom command to outputFiles list
		endif()
	endforeach()
	
	set(${tsFiles} ${${tsFiles}} ${_my_tsfiles}) #add translation files (*.ts) to tsFiles list

	foreach(_ts_file ${_my_tsfiles})
		#message(STATUS "update ${_ts_file}")
		if(_my_sources)
			# make a .pro file to call lupdate on, so we don't make our commands too
			# long for some systems
			get_filename_component(_ts_name ${_ts_file} NAME_WE)
			set(_ts_pro ${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/${_ts_name}_lupdate.pro)
			set(_pro_srcs)
			foreach(_pro_src ${_my_sources})
				set(_pro_srcs "${_pro_srcs} \"${_pro_src}\"")
			endforeach()
			set(_pro_includes)
			get_directory_property(_inc_DIRS INCLUDE_DIRECTORIES)
			foreach(_pro_include ${_inc_DIRS})
				get_filename_component(_abs_include "${_pro_include}" ABSOLUTE)
				set(_pro_includes "${_pro_includes} \"${_abs_include}\"")
			endforeach()
			file(WRITE ${_ts_pro} "SOURCES = ${_pro_srcs}\nINCLUDEPATH = ${_pro_includes}\n")
		endif()
		add_custom_command(OUTPUT ${_ts_file}_update
			COMMAND ${QT_LUPDATE_EXECUTABLE}
			ARGS ${_lupdate_options} ${_ts_pro} ${_my_dirs} -locations relative -no-ui-lines -ts ${_ts_file}
			DEPENDS ${_my_sources} ${_ts_pro} VERBATIM)
		set(${outputFiles} ${${outputFiles}} ${_ts_file}_update) #add output file for custom command to outputFiles list
	endforeach()
   
	#QT4_ADD_TRANSLATION_ITOM(${_qm_files} ${_my_tsfiles})
ENDMACRO()


MACRO(QT4_ADD_TRANSLATION_ITOM _qm_files output_location target)
	foreach (_current_FILE ${ARGN})
		#message(STATUS "release ${_current_FILE}, target: ${target}")
		get_filename_component(_abs_FILE ${_current_FILE} ABSOLUTE)
		get_filename_component(qm ${_abs_FILE} NAME_WE)
		#get_source_file_property(output_location ${_abs_FILE} OUTPUT_LOCATION)

		#if(output_location)
		file(MAKE_DIRECTORY "${output_location}")
		set(qm "${output_location}/${qm}.qm")
		#else()
		#set(qm "${CMAKE_CURRENT_BINARY_DIR}/${qm}.qm")
		#endif()
		
		add_custom_command(TARGET ${target}
                     PRE_BUILD
                     COMMAND ${QT_LRELEASE_EXECUTABLE}
					 ARGS ${_abs_FILE} -qm ${qm}
					 VERBATIM
					 )

		#add_custom_command(OUTPUT ${qm}
		#	COMMAND ${QT_LRELEASE_EXECUTABLE}
		#	ARGS ${_abs_FILE} -qm ${qm}
		#	DEPENDS ${_abs_FILE} VERBATIM
		#)
		set(${_qm_files} ${${_qm_files}} ${qm})
	endforeach ()
ENDMACRO()


#use this macro in order to append to the sources and destinations
#list the library file for your designer plugin, that is finally
#copied to the designer folder of itom.
#
# example:
# set(COPY_SOURCES "")
# set(COPY_DESTINATIONS "")
# ADD_DESIGNERLIBRARY_TO_COPY_LIST(targetNameOfYourDesignerPlugin COPY_SOURCES COPY_DESTINATIONS)
#
# Now the length of COPY_SOURCES and COPY_DESTINATIONS is 1. You can append further entries
# and finally call POST_BUILD_COPY_FILES-macro, to initialize the copying.
#
MACRO (ADD_DESIGNERLIBRARY_TO_COPY_LIST target sources destinations)
    IF(${ITOM_APP_DIR} STREQUAL "")
        message(SEND_ERROR "ITOM_DIR is not indicated")
    ENDIF()
	# message(STATUS "target: ${target}")
	
	#GET_TARGET_PROPERTY(VAR_LOCATION ${target} LOCATION)
	#STRING(REGEX REPLACE "\\(Configuration\\)" "<CONFIGURATION>" VAR_LOCATION ${VAR_LOCATION})
	#SET(VAR_LOCATION "$<TARGET_FILE:${target}>")
	LIST(APPEND ${sources} "$<TARGET_FILE:${target}>") #adds the complete source path including filename of the dll (configuration-dependent) to the list 'sources'
	LIST(APPEND ${destinations} ${ITOM_APP_DIR}/designer)
#	message(STATUS "sources:  ${${sources}}")
#	message(STATUS "destinations:  ${${destinations}}")

ENDMACRO (ADD_DESIGNERLIBRARY_TO_COPY_LIST)


#use this macro in order to append to the sources and destinations
#list the library file for your itom plugin, that is finally
#copied to the plugin/target folder of itom.
#
# example:
# set(COPY_SOURCES "")
# set(COPY_DESTINATIONS "")
# ADD_PLUGINLIBRARY_TO_COPY_LIST(targetNameOfYourPlugin COPY_SOURCES COPY_DESTINATIONS)
#
# Now the length of COPY_SOURCES and COPY_DESTINATIONS is 1. You can append further entries
# and finally call POST_BUILD_COPY_FILES-macro, to initialize the copying.
#
MACRO (ADD_PLUGINLIBRARY_TO_COPY_LIST target sources destinations)
	IF(${ITOM_APP_DIR} STREQUAL "")
        message(SEND_ERROR "ITOM_DIR is not indicated")
    ENDIF()
	
	#GET_TARGET_PROPERTY(VAR_LOCATION ${target} LOCATION)
	#STRING(REGEX REPLACE "\\(Configuration\\)" "<CONFIGURATION>" VAR_LOCATION ${VAR_LOCATION})
	#SET(VAR_LOCATION "$<TARGET_FILE:${target}>")
	LIST(APPEND ${sources} "$<TARGET_FILE:${target}>") #adds the complete source path including filename of the dll (configuration-dependent) to the list 'sources'
	LIST(APPEND ${destinations} ${ITOM_APP_DIR}/plugins/${target})

ENDMACRO (ADD_PLUGINLIBRARY_TO_COPY_LIST)

MACRO (ADD_QM_FILES_TO_COPY_LIST target qm_files sources destinations)
	IF(${ITOM_APP_DIR} STREQUAL "")
        message(SEND_ERROR "ITOM_DIR is not indicated")
    ENDIF()
	
	foreach(_qmfile ${${qm_files}})
		LIST(APPEND ${sources} ${_qmfile}) #adds the complete source path including filename of the dll (configuration-dependent) to the list 'sources'
		LIST(APPEND ${destinations} ${ITOM_APP_DIR}/plugins/${target}/translation)
	endforeach()
ENDMACRO (ADD_QM_FILES_TO_COPY_LIST)


MACRO (ADD_OUTPUTLIBRARY_TO_SDK_LIB target sources destinations)
#    message(STATUS "target: ${target} ${ITOM_SDK_DIR} ${CMAKE_SIZEOF_VOID_P}")
    
    IF(${ITOM_SDK_DIR} STREQUAL "")
        message(SEND_ERROR "ITOM_SDK_DIR is not indicated")
    ENDIF()
    
    if ( CMAKE_SIZEOF_VOID_P EQUAL 4 )
      SET(SDK_PLATFORM "x86")
    else ( CMAKE_SIZEOF_VOID_P EQUAL 4 )
      SET(SDK_PLATFORM "x64")
    endif ( CMAKE_SIZEOF_VOID_P EQUAL 4 )
    
    IF(MSVC10)
        SET(SDK_COMPILER "vc10")
    ELSEIF(MSVC9)
        SET(SDK_COMPILER "vc9")
    ELSEIF(MSVC8)
        SET(SDK_COMPILER "vc8")
    ELSEIF(CMAKE_COMPILER_IS_GNUCXX)
	SET(SDK_COMPILER "gnucxx")
    ELSE(MSVC10)
        SET(SDK_COMPILER "unknown")
    ENDIF(MSVC10)
    
    SET( destination "${ITOM_SDK_DIR}/lib/${SDK_COMPILER}_${SDK_PLATFORM}" )
    
    LIST(APPEND ${sources} "$<TARGET_FILE:${target}>") #adds the complete source path including filename of the dll (configuration-dependent) to the list 'sources'
	LIST(APPEND ${destinations} ${destination}) 
ENDMACRO (ADD_OUTPUTLIBRARY_TO_SDK_LIB target sources destinations)


MACRO (POST_BUILD_COPY_FILES target sources destinations)
	list(LENGTH ${sources} temp)
	math(EXPR len1 "${temp} - 1")
	list(LENGTH ${destinations} temp)
	math(EXPR len2 "${temp} - 1")
	
	#message(STATUS "sources LEN: ${len1}")
	#message(STATUS "destinations LEN: ${len2}")

	IF( NOT len1 EQUAL len2 )
		message(SEND_ERROR "POST_BUILD_COPY_FILES len(sources) must be equal to len(destinations)")
	ENDIF( NOT len1 EQUAL len2 )
	
	SET (destPathes "")
	foreach(dest ${${destinations}})
		#if dest is a full name to a file:
		#GET_FILENAME_COMPONENT(destPath ${dest} PATH)
		#LIST(APPEND destPathes ${destPath})
		LIST(APPEND destPathes ${dest})
	endforeach(dest ${${destinations}})
	LIST(REMOVE_DUPLICATES destPathes)
	
#	message(STATUS "destPathes: ${destPathes}")
	
	#try to create all pathes
	foreach(destPath ${destPathes})
		#first try to create the directory
		ADD_CUSTOM_COMMAND(TARGET ${target} POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E make_directory
				"${destPath}"
		)
	endforeach(destPath ${destPathes})
	
	foreach(val RANGE ${len1})
		list(GET ${sources} ${val} val1)
		list(GET ${destinations} ${val} val2)
#		message(STATUS "POST_BUILD: COPY ${val1} TO ${val2}")
		
		ADD_CUSTOM_COMMAND(TARGET ${target} POST_BUILD                 # Adds a post-build event to MyTest
			COMMAND ${CMAKE_COMMAND} -E copy_if_different  			   # which executes "cmake - E copy_if_different..."
				"${val1}"      										   # <--this is in-file
				"${val2}"                 				               # <--this is out-file path
		)
	endforeach()
ENDMACRO (POST_BUILD_COPY_FILES target sources destinations)


MACRO (POST_BUILD_COPY_FILE_TO_LIB_FOLDER target sources)

    IF(${ITOM_APP_DIR} STREQUAL "")
        message(SEND_ERROR "ITOM_DIR is not indicated")
    ENDIF()
    
	list(LENGTH ${sources} temp)
	math(EXPR len1 "${temp} - 1")
	
	#message(STATUS "sources LEN: ${len1}")
	#message(STATUS "destinations LEN: ${len2}")
	
	
	foreach(val RANGE ${len1})
		list(GET ${sources} ${val} val1)
#		message(STATUS "POST_BUILD: COPY ${val1} TO ${ITOM_APP_DIR}/lib")
		
		ADD_CUSTOM_COMMAND(TARGET ${target} POST_BUILD                 # Adds a post-build event to MyTest
			COMMAND ${CMAKE_COMMAND} -E copy_if_different  			   # which executes "cmake - E copy_if_different..."
				"${val1}"      										   # <--this is in-file
				"${ITOM_APP_DIR}/lib"                 				               # <--this is out-file path
		)
	endforeach()
ENDMACRO (POST_BUILD_COPY_FILE_TO_LIB_FOLDER target sources)


MACRO (ADD_SOURCE_GROUP subfolder)
    FILE(GLOB GROUP_FILES_H
        ${CMAKE_CURRENT_SOURCE_DIR}/${subfolder}/*.h
        ${CMAKE_CURRENT_SOURCE_DIR}/${subfolder}/*.ui
    )
    SOURCE_GROUP("Header Files\\${subfolder}" FILES ${GROUP_FILES_H})
    
    FILE(GLOB GROUP_FILES_S
        ${CMAKE_CURRENT_SOURCE_DIR}/${subfolder}/*.cpp
    )
    SOURCE_GROUP("Source Files\\${subfolder}" FILES ${GROUP_FILES_S})
ENDMACRO (ADD_SOURCE_GROUP subfolder)

#some unused macros

MACRO(COPY_FILE_IF_CHANGED in_file out_file target)
#  MESSAGE(STATUS "copy command: " ${in_file} " " ${out_file} " " ${target})
	IF(${in_file} IS_NEWER_THAN ${out_file})    
  #    message("COpying file: ${in_file} to: ${out_file}")
		ADD_CUSTOM_COMMAND (
	#    OUTPUT     ${out_file}
			TARGET ${target}
			POST_BUILD
			COMMAND    ${CMAKE_COMMAND}
			ARGS       -E copy ${in_file} ${out_file}
	#    DEPENDS 	qitom
	#    DEPENDS	${in_file}
	#    MAIN_DEPENDENCY ${in_file}
		)
	ENDIF(${in_file} IS_NEWER_THAN ${out_file})
ENDMACRO(COPY_FILE_IF_CHANGED)


MACRO(COPY_FILE_INTO_DIRECTORY_IF_CHANGED in_file out_dir target)
	GET_FILENAME_COMPONENT(file_name ${in_file} NAME) 
	COPY_FILE_IF_CHANGED(${in_file} ${out_dir}/${file_name} ${target})
ENDMACRO(COPY_FILE_INTO_DIRECTORY_IF_CHANGED)


#Copies all the files from in_file_list into the out_dir. 
# sub-trees are ignored (files are stored in same out_dir)
MACRO(COPY_FILES_INTO_DIRECTORY_IF_CHANGED in_file_list out_dir target)
	FOREACH(in_file ${in_file_list})
		COPY_FILE_INTO_DIRECTORY_IF_CHANGED(${in_file} ${out_dir} ${target})
	ENDFOREACH(in_file)     
ENDMACRO(COPY_FILES_INTO_DIRECTORY_IF_CHANGED)


#Copy all files and directories in in_dir to out_dir. 
# Subtrees remain intact.
MACRO(COPY_DIRECTORY_IF_CHANGED in_dir out_dir target pattern recurse)
	#message("Copying directory ${in_dir}")
	FILE(${recurse} in_file_list ${in_dir}/${pattern})
	FOREACH(in_file ${in_file_list})
		if(NOT ${in_file} MATCHES ".*svn.*")
			STRING(REGEX REPLACE ${in_dir} ${out_dir} out_file ${in_file}) 
			COPY_FILE_IF_CHANGED(${in_file} ${out_file} ${target})
		endif(NOT ${in_file} MATCHES ".*svn.*")
	ENDFOREACH(in_file)     
ENDMACRO(COPY_DIRECTORY_IF_CHANGED)
