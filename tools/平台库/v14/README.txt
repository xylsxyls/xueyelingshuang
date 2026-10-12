Microsoft Visual C++ v14 x64 应用本地运行库
版本：14.51.36247.0
用途：LumaPlayer的ONNX Runtime 1.24.4所需四份CRT；仅安装到产品EXE同目录，不注册或覆盖系统运行库。
文件：x64/msvcp140.dll、msvcp140_1.dll、vcruntime140.dll、vcruntime140_1.dll。
来源：同目录上层的官方vc_redist.x64.exe，原始下载地址https://aka.ms/vc14/vc_redist.x64.exe。
原包SHA-256：843068991DAAA1F73AD9F6239BCE4D0F6A07A51F18C37EA2A867E9BECA71295C。
提取：读取签名原包的Burn清单及附加CAB，校验CAB和负载SHA-1后直接展开x64 Minimum CAB；未执行安装、卸载或修复。四份DLL均为原字节，x64、14.51.36247.0，微软签名Valid。
msvcp140.dll：7C26614E1D733892C2DEAC7E245CE115504B1D80592DD0A01B08E3E5A55F89CA
msvcp140_1.dll：206C931BF90FDAD8816DE3B5E2EF80B2BCAA9406C89ECC05FE6FDDFFE251E982
vcruntime140.dll：D1F4225DF2CD877DBF130D5668A021DCE3F94118455FF5EC952061C30AFC9CE7
vcruntime140_1.dll：A7146C08F89FE5B04541AB507CDB59FF7B44534D4BA3C668A426C6450A03434E
依赖：上述四份闭合后，其余为Windows 10/11提供的UCRT/API-set及系统DLL；不要把System32里的系统组件复制入产品。
维护：四份作为同一版本组更新，重查导入表、签名及实际推理；不混合32/64位、不同版本或Debug DLL。打包可用VC14DIR指定另一份已核对的完整x64组。
微软本地部署及再发行条款：https://learn.microsoft.com/en-us/cpp/windows/determining-which-dlls-to-redistribute?view=msvc-170
微软许可与对应Redist清单：https://learn.microsoft.com/en-us/cpp/windows/redistributing-visual-cpp-files?view=msvc-170