---
AIGC:
    Label: "1"
    ContentProducer: 001191440300708461136T1XGW3
    ProduceID: 4dcff8c4f0c4eb682e9fb9e4b0ef72a3_e05f93b8b55e11f193fb525400393706
    ReservedCode1: NHmU9brKh9ifJrbfDLgtFcBOKExNJ9YNtch7xGhgjdxJWbqzT5/HmZz4aZ/K5TPoStxrV4iIwO+WF4VxZp7joxiwkg3sfhmIp45/C+7aADUdeTG/APvA/0rRI5+WkdW2YFBZAfz6cqtK9q+0eKT6AwVz2ZvtMn6IxjjCfPBAl1C60rpprSx5j7VuoRE=
    ContentPropagator: 001191440300708461136T1XGW3
    PropagateID: 4dcff8c4f0c4eb682e9fb9e4b0ef72a3_e05f93b8b55e11f193fb525400393706
    ReservedCode2: NHmU9brKh9ifJrbfDLgtFcBOKExNJ9YNtch7xGhgjdxJWbqzT5/HmZz4aZ/K5TPoStxrV4iIwO+WF4VxZp7joxiwkg3sfhmIp45/C+7aADUdeTG/APvA/0rRI5+WkdW2YFBZAfz6cqtK9q+0eKT6AwVz2ZvtMn6IxjjCfPBAl1C60rpprSx5j7VuoRE=
---

# Skill: PDF 文献翻译整理三步法（PDF → 中文 Markdown + 图片提取 + 归档 fuel）

## 触发场景

用户提供 PDF 文献（附件或路径），要求：
- "翻译并整理成 markdown"
- "提取图片放入 markdown 对应位置"
- "同样操作 / same act / 同上"（指代本流程）
- 需要将产出归档到 `/Users/zhangbaining/Documents/fuel` 目录

## 完整操作流程（三步法）

### 第一步：翻译 PDF 为中文 Markdown

1. 读取 PDF，判断类型：
   - **矢量 PDF（有文本层）**：直接提取文本翻译。
   - **扫描版 PDF（每页为整页图像）**：整页渲染 + 视觉模型识别/OCR 获取全文后翻译。
   - **乱码文本层 PDF**：同样按整页渲染 + 视觉识别处理。
2. 翻译要求：
   - 完整翻译：摘要、引言、实验、结果与讨论、结论、致谢、参考文献、附录全部覆盖。
   - 结构清晰：标题层级（# / ## / ###）、表格转 Markdown 表。
   - 专业术语规范译法（如再燃、化学计量比、脱挥发分、总固定氮、再循环等）。
   - 公式保留 LaTeX 样式；同位素标记（N¹⁵/N¹⁴）及化学式保留原文格式。
   - 关键公式经整页渲染视觉复核，避免 OCR 误识。

### 第二步：提取图片并插入 Markdown 对应位置

1. 从 PDF 提取全部图表（Fig.1 ~ Fig.N）：
   - **矢量图**：按图题坐标定位，从页面渲染图裁剪出图表区域。
   - **扫描版/内嵌位图**：定位图题坐标后从整页图像精确裁剪。
   - 渲染建议 4 倍高清以保证清晰度。
2. 命名与校验：
   - 图片命名为 `figN.png`（N 为图号），保存到与 markdown 同目录的 `images/` 文件夹。
   - 用视觉模型逐张验证图题文字与图号（Fig. N）完全对应，防止图号错位。
3. 插入引用：
   - 在 markdown 对应图题标题（`### 图 N`）下方插入：`![图N](images/figN.png)`（相对路径）。

### 第三步：归档到 fuel 目录并校验

1. 在 `/Users/zhangbaining/Documents/fuel/` 下建立**独立子目录**（以文献名命名，如 `Hu2001_recycled_NOx_low_recycling_ratio/`）。
2. 将 markdown 与 images 文件夹整体移入该子目录。
3. 校验 markdown 中所有 `images/figN.png` 相对引用有效（引用数与图片数一致，无缺失、无多余、无未被引用）。

## 关键踩坑点（必须遵守）

- **禁止多篇文献共用同一 images 目录**：不同文献图号命名相同（fig1.png 等），共用目录会互相覆盖，导致先处理文献的图片引用失效。**每篇文献必须建独立子目录**。
- **扫描版 PDF 禁止整页截图**：应定位图题坐标精确裁剪，否则图表不清、图号错位。
- **图号必须视觉核验**：裁剪后用视觉模型确认图题与图号一致。
- **产出物声明**：完成后用 `yyb-product` 卡片输出最终文件路径（markdown + 图片列表）。

## 产出物卡片格式

```<yyb-product>
[文献名_中文翻译.md](</Users/zhangbaining/Documents/fuel/<文献名>/<文献名>_中文翻译.md>)
[images/fig1.png](</Users/zhangbaining/Documents/fuel/<文献名>/images/fig1.png>)
...
```

## 示例：已按本流程处理的文献

| 文献 | 子目录 | 图片数 |
|---|---|---|
| burch1994 (NOx再燃) | burch1994_中文翻译.md + images | 11 |
| Hu 2001 (recycled NOx) | Hu2001_recycled_NOx_low_recycling_ratio/ | 6 |
| Liu 2005 (煤粉燃烧NOx再循环) | Liu2005_pulverized_coal_combustion_NOx_recycle/ | 6 |
| Glarborg 2018 (氮化学建模) | Glarborg_2018_modeling_nitrogen_chemistry/ | 49 |
| Zhou 1996 (NOx再循环控制) | Control_of_NOx_emissions_by_NOx_recycle_approach/ | 9 |
*（内容由AI生成，仅供参考）*
