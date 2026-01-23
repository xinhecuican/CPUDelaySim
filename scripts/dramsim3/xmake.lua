
package("dramsim3_lib")
    
    on_load(function (package)
        local dramsim3_dir = path.join(os.curdir(), "thirdparty/DRAMsim3")
        local build_dir = path.join(dramsim3_dir, "build")
        package:set("dramsim3_dir", dramsim3_dir)
        package:set("dramsim3_build_dir", build_dir)
        
        -- 确保 thirdparty 目录存在
        if not os.isdir("thirdparty") then
            os.mkdir("thirdparty")
        end
        -- 如果 DRAMsim3 不存在，克隆它
        if not os.isdir(dramsim3_dir) then
            print("Cloning DRAMsim3...")
            local result = os.exec("git clone https://github.com/OpenXiangShan/DRAMsim3.git %s", dramsim3_dir)
            if not result then
                error("Failed to clone DRAMsim3")
            end
            os.cd(dramsim3_dir)
            os.exec("git checkout -f fca1245acfff01a4f18830cd15675e904564aa2a")
        end
        
        -- 构建 DRAMsim3
        if not os.isdir(build_dir) then
            os.mkdir(build_dir)
        end
        
        if not os.isfile(path.join(build_dir, "libdramsim3.a")) then
            print("Building DRAMsim3...")
            os.cd(build_dir)
            os.exec("cmake -D COSIM=1 -DCMAKE_BUILD_TYPE=Release ..")
            os.exec("make -j")
            if not os.isfile(path.join(build_dir, "libdramsim3.a")) then
                error("Failed to build DRAMsim3 library")
            end
        end
    end)

    on_fetch(function (package)
        local result = {}
        result.links = "dramsim3"
        result.includedirs = path.join(package:get("dramsim3_dir"), "src")
        result.linkdirs = package:get("dramsim3_build_dir")
        result.defines = "DRAMSIM"
        return result
    end)