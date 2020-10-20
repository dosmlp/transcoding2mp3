#ifndef MP3DEF_H
#define MP3DEF_H
/* IDxVx 的头部结构 */
struct IDxVxHeader {
    unsigned char aucIDx[3]; /* 保存的值比如为"ID3"表示是ID3V2 */
    unsigned char ucVersion; /* 如果是ID3V2.3则保存3,如果是ID3V2.4则保存4 */
    unsigned char ucRevision; /* 副版本号 */
    unsigned char ucFlag; /* 存放标志的字节 */
    unsigned char aucIDxSize[4]; /* 整个 IDxVx 的大小，除去本结构体的 10 个字节 */
    /* 只有后面 7 位有用 */
};
struct FlagFrameHeader {
    char ID[4]; /*标识帧，说明其内容，例如作者/标题等*/
    char Size[4]; /*帧内容的大小，不包括帧头，不得小于1*/
    char Flags[2]; /*标志帧，只定义了6 位*/
};
struct DataFrameHeader {
    unsigned int bzFrameSyncFlag1: 8; /* 全为 1 */
    unsigned int bzProtectBit: 1; /* CRC */
    unsigned int bzVersionInfo: 4; /* 包括 mpeg 版本，layer 版本 */
    unsigned int bzFrameSyncFlag2: 3; /* 全为 1 */
    unsigned int bzPrivateBit: 1; /* 私有 */
    unsigned int bzPaddingBit: 1; /* 是否填充，1 填充，0 不填充,layer1 是 4 字节，其余的都是 1 字节 */
    unsigned int bzSampleIndex: 2; /* 采样率索引 */
    unsigned int bzBitRateIndex: 4; /* bit 率索引 */
    unsigned int bzExternBits: 6; /* 版权等，不关心 */
    unsigned int bzCahnnelMod: 2;
    /* 通道
    * 00 - Stereo 01 - Joint Stereo
    * 10 - Dual   11 - Single
    */
};

#endif // MP3DEF_H
