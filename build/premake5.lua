solution "Pneumo-ABM"
	configurations { "Debug", "Release" }
	platforms { "x64" }
	location ("./" .. _ACTION)
	buildoptions {
		"-std=c++11",
		"-Wno-unknown-pragmas"
	}
	includedirs { "$(BOOST_INCLUDE)" }
	libdirs { "$(BOOST_LIB)"}
	configuration "Debug"
		flags { "Symbols" }
	  optimize "Off"
	configuration "Release"
		flags { "Symbols" }
		optimize "Full"

project "Pneumo-ABM"
	kind "ConsoleApp"
	language "C++"
	targetname "pneumo-abm"
	files {
		"../source/*.h",
		"../source/*.hpp",
	  "../source/*.cpp"
	}
	flags {
	  "Unicode",
	  "NoEditAndContinue",
	  "NoManifest",
	  "NoPCH"
	}
	links {
		"boost_system",
		"boost_filesystem",
		"boost_program_options"
	}		
	debugargs { "inputs=../inputs outputs=../outputs" }
	configuration "Debug"
		targetdir "../bin/debug"
	configuration "Release"
		flags { "LinkTimeOptimization" }
	  targetdir "../bin/release"
