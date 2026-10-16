# M1.7 气体→铝料最小 CHT 验证报告

日期：2026-10-16  
分支：`amf-fluent2026`  
阶段：M1.7；本报告只覆盖隔离 DEMO 环境，不构成 7050 合金论文复现结果。

## 1. 结论

M1.7 的最小气体→固体 CHT 热流方向、界面连续性和末时间步整体能量闭合通过。三档网格（228、1824、14592 cells）和两个时间步长（0.01、0.005 s）均完成；每个工况均保存了 Fluent 2026 R1 case/data、journal、transcript 和 UDF CSV。

通过的判据是：

1. `solid_hot_bottom` 及铝料其余外壁的原生热传递报告为 0 W；
2. 气体侧与固体侧 Coupled Wall 报告大小相等、符号相反；统一为“进入铝料为正”后，界面热流连续；
3. 对每个工况最后一个瞬态增量，用 UDF 记录的系统总能量差分 `dE/dt` 与 Fluent 原生入口/出口净热传递报告比较，最大相对差为约 `0.00005%`。

限定：原生 `/report/fluxes/heat-transfer` 是末态独立报告；UDF 中 `BOUNDARY_HEAT_FLUX` 对 velocity-inlet 没有提供原生入口总热传递，故 UDF CSV 的 `native_*` 宏列不用于判定，入口/出口和界面判定全部采用独立 Fluent 原生报告。UDF 的质量焓流列仍保留用于审计。

本次 0.5 s 演示中铝料最高温度仅约 300.12–300.28 K，液相率始终为 0，因而没有声称已经验证潜热吸收；等效比热容的潜热分支已加载，但本短时工况没有进入糊状区。

## 2. 实际环境与复现命令

- Fluent：ANSYS Fluent 2026 R1，Build Id `10262`。
- License server：`1055@localhost`。
- 启动方式：

  ```powershell
  & 'C:\Program Files\ANSYS Inc\v261\fluent\ntbin\win64\fluent.exe' 3ddp -g -t1 -mpi=intel -r26.1.0 -i <journal>
  ```

  实际执行使用了 `Start-Process`，工作目录为 `C:\Users\admin\m17_demo`，以避免带空格和中文的路径影响 Fluent journal。

- 新 UDF：`validation/m1_7_gas_solid_demo/m17_gas_solid_udf.c`。
- 编译/加载 journal：`m17_compile_udf.jou`、`m17_recompile_udf.jou`。
- 编译 transcript：`raw/final/m17_udf_compile.trn`、`raw/final/m17_udf_recompile.trn`。
- 初次编译实际列出并加载：`m17_equivalent_cp`、`m17_thermal_conductivity`、`m17_gas_solid_energy_audit`。

仓库预检查 `python scripts/preflight.py`：PASS，但继续报告原有的 `400 vs 800` 冲突和 7050 物性/CAD/燃烧开关缺失。仓库 `tests/run_tests.sh` 在本机未能执行：PowerShell 没有 Bash，WSL 仅显示未安装发行版，系统也没有可用的 `cc`；因此不能把 C 单元测试说成通过。Fluent 2026 R1 内部的 UDF 编译、加载和六个实际求解工况是本阶段可执行的编译/运行验证。

第一次从 M1.6 case 直接读入时，Fluent 因 case 中残留的 `m16_demo_lib8` 依赖中止；随后保留旧失败记录，并改从已验证的 M1.6 原始网格重建 M1.7 case。压力出口输入第一次多了一个空行也导致真实 journal 错位；修正后设置 case 成功写出。这些 transcript 保存在 `raw/diagnostic_*`，未被当作有效计算结果。

## 3. 物理、几何与边界设置

### 3.1 复用的两区域模型

复用 M1.6 已通过的 V261 SpaceClaim/Workbench 网格流程和尺寸，仅在 Fluent 中重新配置边界与物性：

- DEMO 几何尺寸：`0.10 m × 0.03 m × 0.01 m`；铝料位于 `y=0…0.01 m`，气体位于 `y=0.01…0.03 m`。
- `solid_zone`：铝料；`fluid_zone`：气体。
- `coupled_wall_solid` / `coupled_wall_fluid`：同一固-流界面的两侧。
- 铝料外壁：`solid_hot_bottom`、`solid_inlet_side`、`solid_front_side`、`solid_outlet_side`、`solid_back_side`。
- 气体入口/出口：`fluid_inlet`、`fluid_outlet`。
- 入口速度：`0.01 m/s`；入口温度：`1200 K`；铝料初始化：`300 K`。
- 取消固体底部 50 kW 热流；所有铝料外壁的恒定热流均为 `0 W/m²`。
- 不启用燃烧、辐射和液相流动；只验证流体对流、固体导热和 Coupled Wall 热传递。
- 能量方程开启，演示设置保留 Fluent 原生入口热扩散选项；入口/出口总热传递最终以原生报告为准。

### 3.2 DEMO 物性门禁

UDF 使用的是 M1.5/M1.6 DEMO 常数：`rho=2700 kg/m³`、`cp_s=900`、`cp_l=1100 J/(kg·K)`、`L=390000 J/kg`、`Tsolidus=748 K`、`Tliquidus=911 K`、`k_s=235`、`k_l=62.8 W/(m·K)`。这些值明确写在 UDF 注释中，不能当作 7050 合金论文物性。生产 UDF 的正式物性门禁没有修改。

## 4. 50 kW 与约 250 W 差异审计

M1.6 中规定的 50 kW 是边界总热输入时，应满足：

```text
Q = q'' × A
  = 5.0×10^7 W/m² × 1.0×10^-3 m²
  = 5.0×10^4 W
```

因此，不能把 `50000 W` 和 `50000 W/m²` 混称。M1.6 的 Fluent 原生热传递报告对 `solid_hot_bottom` 给出约 `50000 W`，而 `WALL_HEAT_FLUX` 探针约 `250 W` 的读数若未做完整边界面积积分，就不能与总热输入比较；它可能是局部面/局部面积、错误边界选择或不同符号/报告定义的结果。复核时必须同时记录边界名称、单位、面积、积分方式和符号。

M1.7 已取消该热流输入。六个最终工况的原生报告均给出 `solid_hot_bottom ≈ -0 W`，所以 M1.7 的铝料热量只能来自高温气体侧 Coupled Wall。

## 5. 原生 Fluent 热传递报告

Fluent 原生报告的原始符号按所选边界/相邻区域保留。本报告统一定义：

```text
Q_into_solid = Q_native(coupled_wall_solid)
Q_out_of_fluid = -Q_native(coupled_wall_fluid)
Q_interface_residual = Q_native(coupled_wall_solid)
                    + Q_native(coupled_wall_fluid)
```

在本 M1.7 气体→铝料方向，`coupled_wall_solid` 为正、`coupled_wall_fluid` 为负。以下数值均来自 `m17_native_report_*.trn`，不是手工伪造：

| 网格 | Δt (s) | 固体侧 (W) | 气体侧 (W) | 入口 (W) | 出口 (W) | 底部 (W) | 外边界 Net (W) |
|---|---:|---:|---:|---:|---:|---:|---:|
| 228 | 0.010 | +0.61708297 | -0.61708297 | +2.5205772 | -0.0045616456 | -0 | +2.5160156 |
| 228 | 0.005 | +0.60752854 | -0.60752854 | +2.5155621 | -0.0045616443 | -0 | +2.5110005 |
| 1824 | 0.010 | +0.66140412 | -0.66140412 | +2.5369139 | -0.0045616496 | -0 | +2.5323523 |
| 1824 | 0.005 | +0.65725514 | -0.65725514 | +2.5337925 | -0.0045616436 | -0 | +2.5292308 |
| 14592 | 0.010 | +0.69648150 | -0.69648150 | +2.6157034 | -0.0045617002 | -0 | +2.6111417 |
| 14592 | 0.005 | +0.70566216 | -0.70566216 | +2.6077191 | -0.0045616242 | -0 | +2.6031575 |

六个工况的界面残差在报告显示精度内为 0；这同时确认了气体侧、固体侧不是同一侧数据的重复抄录。

## 6. 瞬态能量审计

UDF CSV 对每个时间步记录：固体显热、固体潜热、固体总焓、流体显热、入口/出口质量焓流、温度极值和液相率。最后一个时间步用 Fluent 原生报告做独立闭合：

```text
dE/dt = [E(t_n) - E(t_{n-1})] / Δt
Q_external = Q_native(fluid_inlet)
            + Q_native(fluid_outlet)
            + Q_native(solid_hot_bottom)
```

| 网格 | Δt (s) | `dE/dt` (W) | 原生 `Q_external` (W) | 差值 (W) | 相对差 |
|---|---:|---:|---:|---:|---:|
| 228 | 0.010 | 2.5160158 | 2.5160156 | +0.0000002 | 0.00001% |
| 228 | 0.005 | 2.5110004 | 2.5110005 | -0.0000001 | 0.00000% |
| 1824 | 0.010 | 2.5323530 | 2.5323523 | +0.0000007 | 0.00003% |
| 1824 | 0.005 | 2.5292310 | 2.5292308 | +0.0000002 | 0.00001% |
| 14592 | 0.010 | 2.6111430 | 2.6111417 | +0.0000013 | 0.00005% |
| 14592 | 0.005 | 2.6031577 | 2.6031575 | +0.0000002 | 0.00001% |

这项闭合包括气体体积能量、铝料显热和 Coupled Wall 界面传热；界面在系统总账中相互抵消。UDF CSV 中的潜热均为 0 J、液相率均为 0，表示本 0.5 s 短时 DEMO 尚未跨过 748 K 固相线，不能解读为“潜热模型已被数值激活”。

## 7. 网格与时间步长敏感性

网格由同一 228-cell 基准 case 做全域体积细化：

| 级别 | cells | 说明 |
|---|---:|---|
| Base | 228 | 76 solid + 152 fluid |
| Refined-1 | 1824 | 一次全域细化 |
| Refined-2 | 14592 | 两次全域细化 |

末态统计：

| 网格 | Δt (s) | 铝料 Tmax (K) | 液相率 | 固体侧界面热流 (W) | 固体显热 (J) | 潜热 (J) |
|---|---:|---:|---:|---:|---:|---:|
| 228 | 0.010 | 300.12575 | 0 | 0.61708 | 0.29386 | 0 |
| 228 | 0.005 | 300.12300 | 0 | 0.60753 | 0.28263 | 0 |
| 1824 | 0.010 | 300.19920 | 0 | 0.66140 | 0.31148 | 0 |
| 1824 | 0.005 | 300.19769 | 0 | 0.65726 | 0.30511 | 0 |
| 14592 | 0.010 | 300.27397 | 0 | 0.69648 | 0.30138 | 0 |
| 14592 | 0.005 | 300.28022 | 0 | 0.70566 | 0.31701 | 0 |

网格加密仍改变界面热流和固体温升，说明该最小 CHT 还没有达到可用于论文定量结论的网格独立性；本阶段只把它作为 M1.7 环境/方向/守恒 gate，通过后再进入 M2 方案设计。

## 8. 真实文件清单

- 源码和可复现 journals：`validation/m1_7_gas_solid_demo/`。
- 最终 Fluent 二进制、transcript、CSV 和 native report：`validation/m1_7_gas_solid_demo/raw/final/`。
- 真实失败/修复记录：`validation/m1_7_gas_solid_demo/raw/diagnostic_diffusion_yes/`、`raw/diagnostic_no_diffusion_hang/`，以及对应的 setup journals/transcript。
- `raw/final` 中的 case/data 没有被转换、平滑或手工修改；报告只读取 Fluent 写出的文件。

## 9. 后续边界

M1.7 不运行完整炉膛，不启用燃烧/辐射，不运行 20 组参数优化。M2 只建立参数化三维建模方案、尺寸台账和缺失数据清单，见 `docs/12_Codex_M2三维炉膛参数化建模方案.md`；正式 7050 物性门禁仍保持关闭，直到物性来源、单位和温度范围完成审计。
