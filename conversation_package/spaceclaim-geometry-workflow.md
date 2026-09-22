# SpaceClaim 几何建模操作方案

## 1. 建模目标

建立一个可以直接传递到 Fluent Meshing 的连通流体域，范围包括：

`主炉膛 + 冷灰斗 + 一次风携粉通道 + 二次风通道 + OFA + 四层 CO 回喷口 + 炉膛出口烟道`

建议保留一个母版文件 `boiler_geometry_master.scdoc`，将喷口实体、参考平面和驱动尺寸全部保留。不同 CO 工况从母版复制后再启用、封闭或改变对应喷口。

## 2. 坐标和练习参数

坐标定义：X 为宽度，Y 为深度，Z 为竖直方向；原点放在冷灰斗底部出口中心；单位为 m。

| 部件 | 练习参数 |
|---|---|
| 主炉膛 | 12.8 × 12.8 m，Z=8–58 m |
| 冷灰斗 | Z=0–8 m，上口12.8 × 12.8 m，下口约12.8 × 2.0 m |
| 一次风 | 每角六层，Z=14、16、18、20、22、24 m；单通道0.60 × 0.50 m |
| 二次风 | 每角七层，Z=13、15、17、19、21、23、25 m；单通道0.60 × 0.40 m |
| OFA | 每角两层，Z=36、38 m；单通道0.70 × 0.60 m |
| CO 回喷口 | H1–H4：Z=27、29、31、33 m；直径0.30 m |
| 喷管外伸长度 | 一次风、二次风、OFA 和 CO 均暂取1.5 m |
| 出口烟道 | 练习截面约8 m × 8 m，外伸6 m |

以上尺寸仅用于建立模板。实际模型必须替换为设计图纸数据。

## 3. 操作步骤

### 3.1 设置单位、组件和标高

1. 新建 SpaceClaim 文件，单位设为 Meter。
2. 保存为 `boiler_geometry_master.scdoc`。
3. 在结构树建立 `furnace_main`、`hopper_fluid`、`burners`、`ofa`、`co_ports`、`outlet_flue` 等组件。
4. 建立 Z=8、14、24、27、29、31、33、36、38、48、56、58 m 的参考平面。

### 3.2 建立主炉膛

1. 在 XY 平面绘制以原点为中心的12.8 m × 12.8 m矩形。
2. 放置在Z=8 m。
3. 使用 Pull 沿Z方向拉伸50 m。
4. 命名为 `furnace_main`。
5. 如需表示四角切圆斜面，对四条竖直角边按图纸切角；练习模型可先取每侧0.8 m。

### 3.3 建立冷灰斗

1. 在 YZ 平面绘制梯形：上口 Y=±6.4、Z=8；下口 Y=±1.0、Z=0。
2. 沿 X 方向对称拉伸12.8 m。
3. 命名为 `hopper_fluid`。
4. 修整角部，使其与主炉膛截面匹配。
5. 使用 Combine → Merge 与主炉膛合并。
6. 保留底部端面，后续命名为 `bottom_hopper`。

### 3.4 建立一个角的燃烧器通道

1. 在角部斜面外侧建立垂直于喷口轴线的参考平面。
2. 以指向炉膛中心的水平线为基准，设置切圆偏角；练习模型可取约8°。
3. 绘制0.60 m × 0.50 m的一次风矩形截面。
4. 沿喷口轴线 Pull 约1.5 m，形成一次风携粉通道。
5. 将一次风通道中心放置在Z=14 m。
6. 沿Z方向阵列，间距2 m，共六层。
7. 另建0.60 m × 0.40 m二次风通道，第一层中心Z=13 m。
8. 沿Z方向阵列，间距2 m，共七层。

一次风和二次风的入口面要独立保留。煤粉在 Fluent 中通过一次风入口的 DPM injection 定义，不需要在 SpaceClaim 中画“煤粉实体”。

### 3.5 复制到四角

1. 选中一个角的全部一次风和二次风实体。
2. 以炉膛中心 Z 轴为旋转轴。
3. 使用旋转阵列，数量4，间隔90°。
4. 俯视检查四角射流是否保持目标切圆方向。
5. 若实际锅炉有反切二次风，单独调整对应层的轴线，不要用统一方向替代。

### 3.6 建立 OFA

1. 在一个角绘制0.70 m × 0.60 m矩形通道。
2. 将中心放置在Z=36 m。
3. 沿Z方向复制到Z=38 m。
4. 绕中心Z轴复制到其他三个角。
5. 将入口端面与主二次风入口分开命名。

### 3.7 建立 CO 回喷口

CO 回喷口采用圆形流体通道。以 H1=27 m 前墙喷口为例：

1. 在前墙中心位置绘制直径0.30 m圆。
2. 沿墙面法线向炉外 Pull 1.5 m。
3. 让圆柱内端与炉膛流体域真实相交。
4. 绕中心Z轴复制，得到前、后、左、右四个喷口。
5. 将整层沿Z方向复制至H2=29、H3=31、H4=33 m。
6. 每层四个入口端面分别命名。

CO 喷口的角度参数化时，应以喷口与炉墙交点为旋转中心，分别控制水平偏角和竖直倾角。所有高度和角度都保留在母版中。

### 3.8 建立出口烟道

1. 在后墙上部建立约8 m × 8 m的矩形出口。
2. 沿烟气流向拉伸6 m。
3. 与主炉膛合并。
4. 仅将烟道最外端面命名为 `outlet_flue`。

正式模型应按图纸保留折焰角、截面收缩和后烟道转弯。屏式受热面如果明显改变流动，应保留占据空间或以有说明的等效结构表示。

## 4. 流体域和拓扑

推荐直接建立流体体积并合并成一个连通实体。如果导入的是完整金属壳体，则使用 Prepare → Volume Extract：选择封闭面或边界环，必要时指定内部 Seed Face，预览内部面后完成抽取。

SpaceClaim 的 Volume Extract 支持端盖、种子面、内部面预览和共享拓扑选项：[官方文档](https://ansyshelp.ansys.com/public/Views/Secured/corp/v242/en/spaceclaim/Discovery/user_manual/prepare/t_creating_volume_body.html)。

几何编辑完成后再执行 Share Topology。多个相邻流体实体需要共形接口时，使用 Share Topology；单一合并流体实体通常不需要额外操作。[官方文档](https://ansyshelp.ansys.com/public/Views/Secured/corp/v251/en/spaceclaim/Discovery/user_manual/meshing/mesh/t_mesh_conformal_mesh_sharetopo.html)

## 5. Named Selections

使用 Groups 面板的 Create NS 或 Ctrl+G 建立命名选择。建议采用以下命名规则：

| Named Selection | 含义 |
|---|---|
| `pa_c1_l1` | 第1角第1层一次风入口 |
| `sa_c1_l1` | 第1角第1层二次风入口 |
| `ofa_c1_l1` | 第1角第1层OFA入口 |
| `co_h1_front` | H1前墙CO入口 |
| `co_h1_rear` | H1后墙CO入口 |
| `co_h1_left` | H1左墙CO入口 |
| `co_h1_right` | H1右墙CO入口 |
| `outlet_flue` | 炉膛出口烟道末端 |
| `wall_furnace` | 炉膛连续壁面 |
| `wall_hopper` | 冷灰斗壁面 |
| `wall_nozzles` | 喷口侧壁 |
| `bottom_hopper` | 灰斗底部端面 |
| `fluid_domain` | 流体实体 |

入口端面和喷管侧壁不要放进同一个入口组。每个CO高度至少保留独立的四个入口面，便于 Fluent 中分别改变流量、温度和组分。

## 6. 几何验收

导入 Fluent Meshing 前检查：

- 长度单位为米，炉膛尺寸没有出现1000倍缩放；
- 炉膛、灰斗、喷口和出口连通；
- 未启用的喷口已经封闭或按工况抑制；
- 没有重复体、重叠体、孤立小体和极小狭缝；
- 所有入口面、出口面和壁面分组互不混淆；
- 四角射流方向满足切圆要求；
- CO 喷口位置、直径、角度和编号可追溯；
- 炉膛出口没有被屏式受热面或封堵面截断；
- 体积检查显示只有目标流体区域；
- 生成一张剖切图，确认灰斗、燃烧器、OFA、CO口和出口均位于同一流体域。

完整启用练习布局时，入口数量应为：24个一次风、28个二次风、8个OFA和16个CO，共76个入口端面。

## 7. 推荐文件结构

```text
boiler_geometry_master.scdoc
geometry/
  baseline_no_co.scdoc
  co_h1.scdoc
  co_h2.scdoc
  co_h3.scdoc
  co_h4.scdoc
  co_parametric_master.scdoc
notes/
  dimensions.csv
  named-selections.csv
  geometry-checklist.md
```

## 8. 版本和文档参考

- [SpaceClaim Volume Extract](https://ansyshelp.ansys.com/public/Views/Secured/corp/v242/en/spaceclaim/Discovery/user_manual/prepare/t_creating_volume_body.html)
- [SpaceClaim Combine/Split](https://ansyshelp.ansys.com/public/Views/Secured/corp/v242/en/spaceclaim/Discovery/user_manual/intersecting_combining_and_splitting.html)
- [SpaceClaim Named Selections](https://ansyshelp.ansys.com/public/Views/Secured/corp/v242/en/spaceclaim/Discovery/user_manual/intro/t_intro_ui-groups_create-namedselection-group.html)
- [SpaceClaim Share Topology](https://ansyshelp.ansys.com/public/Views/Secured/corp/v251/en/spaceclaim/Discovery/user_manual/meshing/mesh/t_mesh_conformal_mesh_sharetopo.html)
- [Fluent Watertight Geometry workflow](https://ansyshelp.ansys.com/public/Views/Secured/corp/v242/en/flu_ug/tgd_user_workflow_guided_watertight.html)
