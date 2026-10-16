# 蓄热式熔铝炉论文复现｜Fluent 2026 + Codex

研究对象：Qiu et al. (2018), *Numerical simulation and optimization of the melting process for the regenerative aluminum melting furnace*, Applied Thermal Engineering 145, 315–327. DOI: https://doi.org/10.1016/j.applthermaleng.2018.09.060

## 范围与状态
目标：**论文 A 路线**——炉气 Fluid Zone（湍流、燃烧、P1 辐射）＋Coupled Wall（CHT）＋铝料固定 Solid Zone（只导热，等效比热容 UDF 处理潜热）。**不**使用内置 Solidification/Melting、VOF、铝液流动或动态网格。

M1.5–M1.7 已有隔离 DEMO 的真实 Fluent 验证，证据及限制见 docs/09–11。M2-A 已完成原文几何审计、候选物性登记及参数化脚本初版，见 [M2-A报告](docs/13_Codex_M2-A几何与物性审计报告.md) 和 [建模测试说明](geometry/README_M2A.md)。正式几何仍因关键尺寸缺失而阻塞；本轮未运行CAD内核或网格测试。演示物性和外部候选数据均未获准进入正式7050模型，生产UDF门禁保留。

## 给 Codex 的使用说明
1. 阅读 AGENTS.md、CODEX_START_HERE.md、docs/03_论文疑点与输入缺口.md。
2. 运行 python scripts/preflight.py，及 bash tests/run_tests.sh。
3. 先完成 M0（论文审计）和 M1（相变 UDF 验证），之后才进行真实 Fluent 仿真。
4. 原始论文 PDF/原图**没有上传到公开仓库**。用户本地持有论文；需要核对原文时，在本地 source/ 目录提供 PDF 或用 DOI 查阅具合法权限的版本。
5. 复现参数的来源、单位、可信度保存在 config/paper_parameters.json。
6. 汇报材料为 presentation/复现方案提纲.md；其余工作文件在 docs、data、udf、tests、scripts。

## 当前重要风险
- 表 3 的基准燃料流量 400 与表 5 的 800（Nm³/h）不一致；表 3 单位栏写 Nm³/s。不要默默改动。
- 缺完整几何、7050 固液相比热、密度、潜热及烧嘴换向具体控制过程。
- 20 组工况与论文的响应面优化只在基准验证通过后开始。
