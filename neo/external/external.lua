--[[ 												MIT License

										Copyright (c) 2025 Krispy

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE. ]]

group("libs")

-- Miniz
local miniz_files = {
	"miniz/miniz.c",
	"miniz/miniz.h",
	"miniz/minizconf.h"
}

-- MiniZip
local minizip_files = {
	"minizip/ioapi.c",
	"minizip/ioapi.h",
	"minizip/unzip.cpp",
	"minizip/unzip.h",
	"minizip/zip.cpp",
	"minizip/zip.h"
}

-- BinkDeC
local binkdec_files = {
	"libbinkdec/src/BinkAudio.cpp",
	"libbinkdec/src/BinkDecoder.cpp",
	"libbinkdec/src/BinkVideo.cpp",
	"libbinkdec/src/BitReader.cpp",
	"libbinkdec/src/FileStream.cpp",
	"libbinkdec/src/HuffmanVLC.cpp",
	"libbinkdec/src/LogError.cpp",
	"libbinkdec/src/Util.cpp",
	"libbinkdec/src/avfft.c",
	"libbinkdec/src/dct.c",
	"libbinkdec/src/dct32.c",
	"libbinkdec/src/fft.c",
	"libbinkdec/src/mdct.c",
	"libbinkdec/src/rdft.c",
}

project("external")
	kind("StaticLib")
	language("C")
	warnings("Off")

	includedirs({"miniz", "minizip", "libbinkdec/include"})

	files({"external.lua", miniz_files, minizip_files, binkdec_files})

	filter("files:minizip/**.cpp")
		language("C++")
	filter({})
	
	filter("files:libbinkdec/src/**.cpp")
		language("C++")
	filter({})
group("")
