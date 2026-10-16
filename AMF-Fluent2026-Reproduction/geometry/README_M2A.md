# M2-A 参数化建模初版

目标：SpaceClaim 2026 R1 API V261 / IronPython宿主；静态测试使用Python 3.13。
源代码与scscript必须逐字节对应。本轮只通过宿主静态测试，没有执行SpaceClaim。

从项目根目录执行：

```powershell
python -X utf8 -m unittest discover -s tests -p test_m2a_geometry.py -v
python scripts/preflight.py
$env:AMF_M2_CONFIG = (Resolve-Path config/m2a_geometry_demo.json).Path
python -X utf8 geometry/m2a_spaceclaim.py --validate-only
$env:AMF_M2_CONFIG = (Resolve-Path config/m2a_geometry_production.json).Path
python -X utf8 geometry/m2a_spaceclaim.py --validate-only
```

最后一条预期非零退出并列出缺失尺寸，不是失败的仿真。测试不会修改配置或生产UDF。

## 重新打包（本机技能工具）

```powershell
python C:/Users/admin/.codex/skills/spaceclaim-vision-modeling/scripts/package_scscript.py geometry/m2a_spaceclaim.py geometry/m2a_spaceclaim.scscript --api 261 --force
python -X utf8 C:/Users/admin/.codex/skills/spaceclaim-vision-modeling/scripts/audit_scscript.py geometry/m2a_spaceclaim.scscript
```

force只覆盖本项目构建产物，不用于覆盖用户CAD。技能打包器的结构源自V21；本轮元数据使用261并通过归档审计，**仍须在V261宿主测试加载**。

## M2-B 可执行DEMO试验说明（尚未运行）

- 复制DEMO JSON至独立运行目录，填写新的绝对output_model (.scdocx) 和report_path (.json)，目录预先存在，两文件均不可存在；保留输入配置和脚本SHA256。
- 在启动SpaceClaim前设置AMF_M2_CONFIG为该配置绝对路径，确保进程继承变量；新建空的未保存设计，运行scscript。脚本不能对现有模型继续执行。
- 验证应有2个独立域、12个面、6个命名组：气体入口1面、出口1面、气侧界面1面、固侧界面1面、气壁3面、固壁5面。
- CAD体积/面积检查完成才保存；异常发生后放弃未保存设计，不在半成品上重试。
- 当前输出报告只覆盖CAD构造；重开、视觉与网格检查必须另外完成和留档。Group API已查证但动态绑定/宿主函数尚未运行测试。
- 空间夹具x为流向，y向上，z为厚度；与论文炉膛坐标不建立尺寸映射。
- 本脚本不含物性、燃烧、辐射、Fluent求解或共享拓扑操作。Coupled Wall是后续Meshing/Fluent验收对象，不由命名选择自动保证。

正式配置永远不会退化为DEMO。关键尺寸缺失首先阻断；补齐后仍需发布审核过的正式炉膛拓扑适配器。
