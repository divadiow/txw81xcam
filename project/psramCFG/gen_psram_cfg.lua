local base_path = arg[1]
local file_table = {}
local select_name = nil
local file = io.open("psram_cfg.h","rb")
local buf
for line in file:lines() do
	match_str = string.match(line,"[%s]*PSRAM_DEF%((.*)%)")
	--解析是否为需要的文件名
	if match_str ~= nil then
		file_table[match_str] = true
		
	--解析是否为选中的文件名
	else
		match_str = string.match(line,"[%s]*PSRAM_SELECT%((.*)%)")
		if match_str ~= nil then
			select_name = match_str
		end
	end

end
file:close()

if select_name ~= nil then
	filename = base_path .. "parameter-" .. select_name .. ".bincfg"
	print(filename)
	
	--将文件写入到parameter.bincfg中,外部拷贝该文件到project目录下
	file = io.open(filename,"rb")
	if file ~= nil then
		buf = file:read("*a")
		file:close()
		file = io.open("parameter.bincfg","wb")
		file:write(buf)
		file:close()
		print("config psram file success:",filename)
	else
		print("config psram file err,maybe this is extern psram")
	end
end
