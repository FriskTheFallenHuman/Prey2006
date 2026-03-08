--[[						MIT License

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

if _OPTIONS["dll"] then
	group( "dll" )
else
	group( "libs" )
end

project( "Game-D3XP" )
	targetname("gamex86")
	kind("SharedLib")
	if _OPTIONS["dll"] then
		kind("SharedLib")
	else
		kind("StaticLib")
	end
	language("C++")
	links({"idLib"})

	defines({"GAME_DLL", "HUMANHEAD"})

	pchsource("d3xp/precompiled.cpp")
	pchheader( "" )

	files({"gamedll.lua", "d3xp/**"})

	removefiles({"d3xp/EndLevel.*", "d3xp/gamesys/Callbacks.cpp"})

group("")
