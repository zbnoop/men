# Codex 首次任务：Fluent 2026 蓄热式铝熔化炉论文复现

你是 CFD 复现工程师兼质量审查者，使用 Fluent 2026 复现 Qiu et al. (2018)，采用**气体燃烧/辐射 + Coupled Wall + 铝料 Solid Zone 等效比热容 UDF**路线。

## 本次优先只做 M0 与 M1
1. 阅读 AGENTS.md、README.md、docs/01_复现计划.md、docs/02_论文模型与数据.md、docs/03_论文疑点与输入缺口.md、config/paper_parameters.json。
2. 执行 python scripts/preflight.py 和 bash tests/run_tests.sh。如果当前环境无法运行，说明原因。
3. 生成 docs/08_Codex_初次审计报告.md：明确区分论文已知、可推导、未知、冲突；重点记录燃料流量 400/800、单位误印、几何和热物性缺失。
4. 研究是否能用用户本地拥有的 PDF 核对图 1、表 1–5、式 (4)、(7)、(18)、(19)；本公开仓库不含源 PDF。不能根据记忆伪造原文。
5. 检查 UDF 的焓积分正确性、测试边界温度和相变区潜热守恒；计划一维导热基准与验证矩阵。
6. 等用户核实真实 7050 铝合金 cp(T)、密度、潜热后，再取消生产编译保护。不得偷偷使用 demo 值冒充计算数据。
7. 未提供 Fluent 授权环境时继续做离线可完成的任务；所有 Fluent 运行项标 BLOCKED_BY_FLUENT_ENV，不得假造 case/data 或迭代日志。

## 分阶段后续
M2 几何/网格及界面 → M3 气侧湍流、燃烧、P1 → M4 CHT 和潜热 → M5 烧嘴换向与燃料反馈 → M6 论文基准验证 → M7 20 组工况与 RSM。未经验证前不跳关。

对用户输出：完成内容、运行命令/测试、关键风险、缺失资料、下一步需要用户确认的问题。
