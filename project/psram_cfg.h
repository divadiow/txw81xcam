#ifndef __PSRAM_CFG
#define __PSRAM_CFG

/**********************************************************************************
该文件主要为了配置psram的
————PSRAM_DEF:定义了psram初始化的文件名,内部会进行psram的初始化
————PSRAM_SELECT:是选择了哪个psram的参数文件信息,所以要从PSRAM_DEF中选择

____E_PSRAM:这个定义代表是外部psram,请用E开头来代表,新增加外部的psram定义
            添加到后面

注意:该文件只要修改PSRAM_SELECT的值,其他不要修改,不然可能会生成失败
***********************************************************************/
#define PSRAM_DEF(name)	psrammode##name
#define PSRAM_SELECT(name)	NOW_PSRAM = psrammode##name
enum psram_cfg_enum
{
	PSRAM_DEF(NONE),
    PSRAM_DEF(2MByte),
    PSRAM_DEF(4MByte),
	PSRAM_DEF(4MByte_dri0_240M),
	PSRAM_DEF(4MByteV2),
    PSRAM_DEF(8MByte),
	PSRAM_DEF(8MByte_240M),
    PSRAM_DEF(16MByte),
    PSRAM_DEF(E_PSRAM),
    PSRAM_DEF(E2MByte),
	PSRAM_SELECT(8MByte),
};


#endif