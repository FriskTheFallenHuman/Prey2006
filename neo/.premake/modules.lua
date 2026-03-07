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

--[[
	Adress Stack Size
	@purpose: Sets the size of  both stack and commit stack (MSVC)
	@inputs: int size, int commit
]]
function adressstacksize(size,commit)
	-- Not valid for StaticLibs
    if not (_OPTIONS['target']) or _OPTIONS['target'] == 'StaticLib' then
        return
    end

    buildoptions({"/STACK:" .. size .. "," .. commit})
end

--[[
	Extra optimizations
	@purpose: Sets even more optimizations that are not covered by premake's optimize()
	@inputs: none
]]
function extraoptimization()
	-- Not valid for StaticLibs
    if not (_OPTIONS['target']) or _OPTIONS['target'] == 'StaticLib' then
        return
    end

	linkoptions({"/OPT:REF", "/OPT:ICF"})
end

--[[
	Link Time Optimizations Extended
	@purpose: Replacement for linktimeoptimization for windows mainly
	@inputs: none
]]
function linktimeoptimizationex(value)
	-- Not valid for StaticLibs
	if not (_OPTIONS['target']) or _OPTIONS['target'] == 'StaticLib' then
		return
	end

	linkoptions({"/LTCG:incremental"})
end

--[[
	Link Time Optimizations Extended
	@purpose: Replacement for linktimeoptimization for windows mainly
	@inputs: none
]]
function linktimeoptimizationex(value)
	-- Not valid for StaticLibs
	if not (_OPTIONS['target']) or _OPTIONS['target'] == 'StaticLib' then
		return
	end

	linkoptions({"/LTCG:incremental"})
end


--[[
	Get Architecture String
	@purpose: Function to get the architecture has a string
	@inputs: none
]]
function getArchitectureString()
    local arch = os.hostarch()
    if arch == "x86" or arch == "x86_64" then
        return arch
    elseif arch == "x86_64" then
        return "x64"
    elseif arch == "i386" then
        return "x86"
    else
        return "unknown_arch"
    end
end
