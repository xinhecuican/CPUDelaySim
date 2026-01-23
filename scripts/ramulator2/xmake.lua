
package("ramulator2_lib")
    
    on_load(function (package)
        local ramulator_dir = path.join(os.curdir(), "thirdparty/ramulator2")
        local build_dir = path.join(ramulator_dir, "build")
        package:set("ramulator_dir", ramulator_dir)
        package:set("ramulator_build_dir", build_dir)
        
        -- 确保 thirdparty 目录存在
        if not os.isdir("thirdparty") then
            os.mkdir("thirdparty")
        end
        -- 如果 ramulator2 不存在，克隆它
        if not os.isdir(ramulator_dir) then
            print("Cloning ramulator2...")
            local result = os.exec("git clone https://github.com/CMU-SAFARI/ramulator2 %s", ramulator_dir)
            if not result then
                error("Failed to clone ramulator2")
            end
            os.cd(ramulator_dir)
        end
        
        -- 构建 ramulator2
        if not os.isdir(build_dir) then
            os.mkdir(build_dir)
        end
        
        if not os.isfile(path.join(ramulator_dir, "libramulator.so")) then
            print("Building ramulator2...")
            os.cd(build_dir)
            os.exec("cmake ..")
            os.exec("make -j8")
            if not os.isfile(path.join(ramulator_dir, "libramulator.so")) then
                error("Failed to build ramulator2 library")
            end
        end
    end)

    on_fetch(function (package)
        local result = {}
        local ramulator_dir = package:get("ramulator_dir")
        result.links = "ramulator"
        result.includedirs = path.join(ramulator_dir, "src")
        result.linkdirs = package:get("ramulator_dir")
        result.rpathdirs = package:get("ramulator_dir")
        result.defines = "RAMULATOR"
        return result
    end)