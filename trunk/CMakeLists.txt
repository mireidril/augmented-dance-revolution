cmake_minimum_required(VERSION 2.6)

# Add all source files
file( GLOB sources src/*.cpp )
file( GLOB headers src/*.hpp src/*.h )

OPTION(32BITS "32BITS" ON) # set to OFF if your PC is 64bits

#Prevent Visual Studio to add its annoying /Debug /Release folders
if( MSVC )
		## Set the directory where the binaries will be stored  
		include_directories( ${CMAKE_SOURCE_DIR}/dependencies/include/ )
		link_directories( ${CMAKE_SOURCE_DIR}/dependencies/lib/ )
		
		set( EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin )
		set( WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/bin )
		
        # Replace the /MDd and /MD flags with respectively /MTd and /MT
        # Prevent Visual to switch to "debug mode" when Boost libraries are the release ones
		set( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin )
        set( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin )
        set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin )
        foreach( OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES} )
                string( TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG )
                set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_SOURCE_DIR}/bin )
                set( CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_SOURCE_DIR}/bin )
                set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_SOURCE_DIR}/bin )
        endforeach( OUTPUTCONFIG CMAKE_CONFIGURATION_TYPES )
		
		# copy WIN32 DLLs 
		if(32BITS)
			file(COPY ${CMAKE_SOURCE_DIR}/dependencies/dll/x86/libARvideod.dll 
			${CMAKE_SOURCE_DIR}/dependencies/dll/DSVLd.dll
			DESTINATION ${CMAKE_SOURCE_DIR}/bin)
		else()
			file(COPY ${CMAKE_SOURCE_DIR}/dependencies/dll/x64/libARvideod.dll 
			${CMAKE_SOURCE_DIR}/dependencies/dll/DSVLd.dll
			DESTINATION ${CMAKE_SOURCE_DIR}/bin)
		endif()
		
endif()

add_executable(AugmentedDanceRevolution ${sources} ${headers} )

if( MSVC )
	target_link_libraries(AugmentedDanceRevolution optimized libAR debug libARd)
	target_link_libraries(AugmentedDanceRevolution optimized libARvideo debug libARvideod)
	target_link_libraries(AugmentedDanceRevolution optimized libARgsub debug libARgsubd)
	target_link_libraries(AugmentedDanceRevolution opengl32)
	target_link_libraries(AugmentedDanceRevolution glut32)
endif()