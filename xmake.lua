add_rules("mode.debug", "mode.release", "mode.profile")
-- add_rules("c++.unity_build")
set_languages("c++20")

includes("scripts/dramsim3")
includes("scripts/ramulator2")

add_requires("nlohmann_json")
add_requires("yaml-cpp")
add_requires("boost")
add_requires("spdlog")
add_requires("thread-pool")
add_requires("lz4")
add_requires("dramsim3_lib")
add_requires("ramulator2_lib")
add_requires("softfloat_lib")
add_includedirs("inc")

option("arch")
    set_default("riscv")
    set_showmenu(true)
    set_values("riscv", "arm", "x86")

option("config")
    set_default("atomic")
    set_showmenu(true)

option("ram_sim")
    set_default("ramulator2")
    set_showmenu(true)
    set_values("ramulator2", "dramsim3")

rule("mode.relWithDebInfo")
    after_load(function (target)
        target:set("symbols", "debug")
        target:set("optimize", "faster")
    end)

local arch = get_config("arch") or "riscv"
local config = get_config("config") or "atomic"

package("softfloat_lib")
    on_load(function (package)
        local softfloat_dir = path.join(os.curdir(), "thirdparty/softfloat")
        local build_dir = path.join(softfloat_dir, "build")
        package:set("softfloat_dir", softfloat_dir)
        package:set("build_dir", build_dir)
        if not os.isdir(build_dir) then
            os.mkdir(build_dir)
        end

        if not os.isfile(path.join(build_dir, "libsoftfloat.a")) then
            print("Building softfloat...")
            os.cd(build_dir)
            os.exec("cmake -D COSIM=1 ..")
            os.exec("make -j")
            if not os.isfile(path.join(build_dir, "libsoftfloat.a")) then
                error("Failed to build softfloat library")
            end
        end
    end)

    on_fetch(function (package)
        local result = {}
        result.links = "softfloat"
        result.includedirs = path.join(package:get("softfloat_dir"), "inc")
        result.linkdirs = package:get("build_dir")
        return result
    end)

target("CPUDelaySim")
    set_kind("binary")
    set_default(true)
    -- set_optimize("fastest")
    set_rundir("$(projectdir)")
    add_files("src/**.cpp|arch/**.cpp")
    add_includedirs("inc")
    add_deps(arch .. "_decode")
    add_deps("parse_param")
    add_files("configs/" .. config .. "/obj/**.cpp")
    add_includedirs("configs/" .. config .. "/obj/")
    add_defines("ARCH_" .. arch:upper())
    add_files("src/arch/" .. arch .. "/**.cpp")
    -- if get_config("arch") == "riscv" then
    --     add_deps("riscv_decode")
    --     add_files("arch/riscv/**.cpp")
    -- end
    if get_config("ram_sim") == "ramulator2" then
        add_packages("ramulator2_lib")
    else
        add_packages("dramsim3_lib")
    end
    add_packages("nlohmann_json", "yaml-cpp", "boost", "spdlog", "softfloat_lib", "thread-pool", "lz4")

target("riscv_decode")
    set_kind("phony")
    on_load(function (target)
        if not os.isfile("inc/arch/riscv/trans_rv16.h") then
            os.exec("python ./scripts/decodetree.py --static-decode=decode_insn16 --insnwidth=16 inc/arch/riscv/insn16.decode -o build/trans_rv16_tmp.c.inc")
            os.execv("python", {"./scripts/emu_cpu_put_ic.py", "build/trans_rv16_tmp.c.inc"}, {stdout = "inc/arch/riscv/trans_rv16.h"})
        end
        if not os.isfile("inc/arch/riscv/trans_rv32.h") then
            os.exec("python ./scripts/decodetree.py --static-decode=decode_insn32 --insnwidth=32 inc/arch/riscv/insn32.decode -o build/trans_rv32_tmp.c.inc")
            os.execv("python", {"./scripts/emu_cpu_put_ic.py", "build/trans_rv32_tmp.c.inc"}, {stdout = "inc/arch/riscv/trans_rv32.h"})
        end
    end)

target("parse_param")
    set_kind("phony")
    on_load(function (target)
        os.execv("python", {"./scripts/parse.py", "--filepath", "configs/" .. config})
    end)
--
-- If you want to known more usage about xmake, please see https://xmake.io
--
-- ## FAQ
--
-- You can enter the project directory firstly before building project.
--
--   $ cd projectdir
--
-- 1. How to build project?
--
--   $ xmake
--
-- 2. How to configure project?
--
--   $ xmake f -p [macosx|linux|iphoneos ..] -a [x86_64|i386|arm64 ..] -m [debug|release]
--
-- 3. Where is the build output directory?
--
--   The default output directory is `./build` and you can configure the output directory.
--
--   $ xmake f -o outputdir
--   $ xmake
--
-- 4. How to run and debug target after building project?
--
--   $ xmake run [targetname]
--   $ xmake run -d [targetname]
--
-- 5. How to install target to the system directory or other output directory?
--
--   $ xmake install
--   $ xmake install -o installdir
--
-- 6. Add some frequently-used compilation flags in xmake.lua
--
-- @code
--    -- add debug and release modes
--    add_rules("mode.debug", "mode.release")
--
--    -- add macro definition
--    add_defines("NDEBUG", "_GNU_SOURCE=1")
--
--    -- set warning all as error
--    set_warnings("all", "error")
--
--    -- set language: c99, c++11
--    set_languages("c99", "c++11")
--
--    -- set optimization: none, faster, fastest, smallest
--    set_optimize("fastest")
--
--    -- add include search directories
--    add_includedirs("/usr/include", "/usr/local/include")
--
--    -- add link libraries and search directories
--    add_links("tbox")
--    add_linkdirs("/usr/local/lib", "/usr/lib")
--
--    -- add system link libraries
--    add_syslinks("z", "pthread")
--
--    -- add compilation and link flags
--    add_cxflags("-stdnolib", "-fno-strict-aliasing")
--    add_ldflags("-L/usr/local/lib", "-lpthread", {force = true})
--
-- @endcode
--

