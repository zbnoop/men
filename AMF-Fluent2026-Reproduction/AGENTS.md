# Codex 项目规则

- 目标是物理和数值意义上的**可复现**，不要生成貌似正确的云图假装完成。
- 使用 Fluent 2026；所有未核实的版本特定菜单、TUI、Scheme、PyFluent 命令，先对照用户机器上的官方文档或实际环境。
- A 路线：炉气 Fluid Zone + 铝料固定 Solid Zone + Coupled Wall + UDF 等效比热容。铝内部忽略液体动量、自然对流和自由界面。
- 明确区别 paper / derived / demo_only / unknown / conflict。示例 cp=900/1100 J/(kg K)、潜热 390000 J/kg **不是论文给出的物性**。
- 原始 PDF 由用户本地持有，**不可**向这个公开仓库上传论文全文或原图，也不可自动复制/引用不存在的 source/*.pdf。
- 原文需要重点确认：表 3 基准 U=400 与表 5 U=800，表 3 单位标注 Nm³/s，表 4 参数错列；可能是单烧嘴/双烧嘴区别，但目前未经证实。
- 固相线 748 K，液相线 911 K；液相率后处理 f=clip((T-748)/163,0,1)，不等于 Fluent 内置熔化模块输出。
- DEFINE_SPECIFIC_HEAT 返回 cp(T) 且通过 *h 返回与 cp 积分一致的焓差；测试不能代替 Fluent 编译。
- 此项目 UDF 以编译防护阻止未经授权/验证的 demo 物性用于真实 Fluent。
- 每阶段记录运行命令、材料数据版本、文件变更、输出、日志、验证和剩余风险；缺 Fluent 2026 可用环境时标注 BLOCKED_BY_FLUENT_ENV。
- 不改 main 分支；此项目全部改动留在 amf-fluent2026 分支；不要强推。
