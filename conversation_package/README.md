# 煤粉炉 SpaceClaim–Fluent 对话包

这是本次对话的可复用摘要包，主题为：

> 传统四角切圆煤粉锅炉的 SpaceClaim 几何建模，服务于 Fluent Meshing/Fluent 仿真，并预留高浓度 CO 回喷口参数化研究。

## 文件

- `conversation-summary.md`：需求、关键判断、范围和待确认事项。
- `dialogue-record.md`：压缩版用户—助手对话记录，不包含内部推理或工具日志。
- `spaceclaim-geometry-workflow.md`：可执行的 SpaceClaim 建模流程、练习尺寸、命名和验收标准。
- `manifest.json`：对话包元数据和文件索引。

## 当前结论

主建模软件采用 ANSYS SpaceClaim，几何交付链为：

`SpaceClaim → Fluent Meshing → Fluent`

首版几何以流体域为中心，保留炉膛、冷灰斗、一次风携粉通道、二次风、OFA、CO 回喷口和炉膛出口；水冷壁管、螺栓、支撑件等不影响整体流动的细部采用连续壁面或等效边界。

## 使用说明

1. 先阅读 `conversation-summary.md`，确认研究对象和哪些尺寸只是练习假设。
2. 按 `spaceclaim-geometry-workflow.md` 在 SpaceClaim 中建立母版文件。
3. 将实际锅炉图纸、DCS 工况和 CO 气源组成替换练习参数。
4. 检查 Named Selections 和流体域连通性后，再传递到 Fluent Meshing。

## 证据边界

包内引用的公开文献参数用于结构参考，不代表目标电厂的真实设计值。真实模型应优先采用锅炉总图、燃烧器布置图、喷口尺寸、风量测点和煤质分析数据。
