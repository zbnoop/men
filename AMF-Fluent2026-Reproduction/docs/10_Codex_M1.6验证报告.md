# AMF-Fluent2026-Reproduction：M1.6 独立热流验证与最小流固耦合测试

## 1. 结论

本阶段在 `amf-fluent2026` 分支、Fluent 2026 R1 DEMO 环境完成了一个可重放的两区域最小模型。结论分开给出：

- 热输入定义、固体吸热和整体能量守恒：通过。
- Fluid/Solid 两区域与 Coupled Wall 拓扑：通过。
- 气体侧/固体侧界面热流一致性：通过 Fluent 原生边界热传递报告复核。
- 时间步长敏感性：通过本次最小测试。
- 网格敏感性：已真实执行，但局部热场尚未网格无关，不能宣称完全通过。
- M2 完整三维炉膛建模：未启动。

因此，M1.6 的热流和能量守恒门槛已通过；但在进入正式论文复现前，仍必须处理网格独立性和 DEMO 物性的替换/核准。演示物性、演示边界和本报告的结果不得用于正式论文结论。

## 2. 环境与分支

| 项目 | 实际记录 |
|---|---|
| 仓库 | `zbnoop/men`，本地 `D:\工作公用\men-amf-fluent2026` |
| 分支 | `amf-fluent2026`；未修改 `main` |
| 基线 | `bc722f9 Validate Fluent 2026 M1.5 DEMO benchmark` |
| Fluent | ANSYS Fluent 2026 R1，Build Id `10262` |
| 许可证 | `1055@localhost`，实际启动成功；本机允许 4-way，验证运行使用 `-t1` |
| UDF 编译 | Fluent 内置 Clang/`clang-cl`，Host/Node 编译并加载成功；最终库 `m16_demo_lib8` |
| PyFluent | 本次仍为 disconnected；实际运行使用 Fluent 直接 journal，未把 PyFluent 状态当成通过证据 |

## 3. 50,000 W 与约 250 W 读数的核对

### 3.1 规定输入的量纲与面积积分

本 DEMO 几何尺寸为 `0.10 m × 0.03 m × 0.01 m`，下部固体为 `0.10 m × 0.01 m × 0.01 m`。`solid_hot_bottom` 面积由实际 Fluent 网格几何得到：

```text
A_hot = 0.001 m^2
q_profile = 5.0e7 W/m^2
Q_prescribed = q_profile * A_hot = 5.0e4 W
```

因此 `50,000 W` 是已知热流 profile 的边界面积积分；UDF 的 profile 值本身是 `W/m^2`，不能与一个未说明定义的单点/面场读数直接比较。

### 3.2 原生 Fluent 报告复核

对真实 `m16_base_run10.cas.h5`/`.dat.h5` 执行 `/report/fluxes/heat-transfer`，只选择热面、耦合壁两侧、流体入口和出口，Fluent 输出：

| Fluent 边界 | Total Heat Transfer Rate |
|---|---:|
| `solid_hot_bottom` | `50000 W` |
| `coupled_wall_solid` | `-3.0283509 W` |
| `coupled_wall_fluid` | `+3.0283509 W` |
| `fluid_inlet` | `-0.041046229 W` |
| `fluid_outlet` | `-0.086596496 W` |
| Net | `49999.872 W` |

这给出两个独立检查：

1. 规定热面面积积分确实是 `50 kW`；
2. 耦合壁两侧大小相等、符号相反，界面配对残差在报告显示精度内为零。

加密网格的原生报告也保持同一结构：`solid_hot_bottom=50000 W`，`coupled_wall_solid=-3.5119037 W`，`coupled_wall_fluid=+3.5119037 W`，Net=`49999.853 W`。

Fluent 官方文本命令清单将 `/report/fluxes/heat-transfer` 定义为边界热传递率报告；报告系统本身区分面积、积分量和边界/表面报告量，参见 [Fluent Text Command List](https://ansyshelp.ansys.com/public/Views/Secured/corp/v261/en/pdf/Ansys_Fluent_Text_Command_List.pdf) 和 [Reporting Alphanumeric Data](https://ansyshelp.ansys.com/public/Views/Secured/corp/v251/en/flu_th/flu_th_chp_report.html)。

### 3.3 对约 250 W 读数的判定

M1.5 旧探针曾读到约 `250 W`。本阶段没有把该值强行解释成 `50,000 W`，而是实际检查了边界、函数名、面积和报告定义。run8 的真实日志进一步暴露出一个可复现的选择风险：UDF 数据名列表同时包含 M1.5 与 M1.6 profile，若接受默认项，Fluent 会选中 `m15_demo_wall_heat_flux`；此时 profile 面积审计仍可显示 50 kW，但固体温度不升高。run10 显式选择 `m16_bottom_heat_flux::m16_demo_lib8` 后，固体才实际吸热。

因此，约 `250 W` 不能作为本 M1.6 的热输入或界面热流结论；它属于旧探针/不同报告口径，准确根因应按旧 journal 重放后再单独追溯。当前 M1.6 的可审计口径是：

- `W/m^2` profile 乘以选定边界面积得到规定输入；
- Fluent 原生 `/report/fluxes/heat-transfer` 输出用于交叉核对边界总热传递率；
- `m16_energy.csv` 中的 `BOUNDARY_HEAT_FLUX` 耦合壁字段仅作诊断，因为该字段在本 DEMO 的 execute-at-end 时刻对 coupled-wall thread 读到接近零，不能取代原生报告。

## 4. 两区域模型与边界定义

实际几何由 SpaceClaim V261 脚本生成，并由 Workbench Fluid Flow 系统生成 Fluent 网格。区域和命名如下：

| 区域/边界 | 实际定义 |
|---|---|
| `solid_zone` | 下部 Solid Zone，76 个基准 hexa cells，体积 `1.0e-5 m^3`，aluminum DEMO material |
| `fluid_zone` | 上部 Fluid Zone，152 个基准 hexa cells，体积 `2.0e-5 m^3`，air DEMO material |
| `coupled_wall_solid` | Solid 侧 coupled wall |
| `coupled_wall_fluid` | Fluid 侧 coupled-wall shadow |
| `solid_hot_bottom` | 固体底部热流边界，面积 `0.001 m^2` |
| `fluid_inlet` | velocity inlet，`0.01 m/s`，`300 K` |
| `fluid_outlet` | pressure outlet，表压 `0 Pa`，回流温度 `300 K` |

本阶段没有启用燃烧、组分输运或辐射；只验证流体侧对流、固体侧导热和耦合壁传热。耦合壁的边界分离、命名和 zone type 证据保存在 `m16_boundary_setup_probe.jou` 以及真实 Fluent transcript 中。

## 5. 等效比热 UDF 与能量审计

使用 M1.5 的 DEMO 等效比热模型：

```text
rho = 2700 kg/m^3
Tsolidus = 748 K
Tliquidus = 911 K
cp_solid = 900 J/(kg K)
cp_liquid = 1100 J/(kg K)
latent heat = 390000 J/kg
k_solid = 235 W/(m K)
k_liquid = 62.8 W/(m K)
```

上述值是演示物性，不是论文正式物性。`m16_combined_udf.c` 同时提供等效比热、导热系数、底部热流 profile 和 execute-at-end 审计。审计包括固体焓、流体显热、入口/出口焓流、规定热流时间积分、液相体积分数和温度极值。

基准 `dt=0.01 s`、50 步、总时间 `0.5 s` 的真实结果：

```text
solid delta-H       = 24999.094134 J
total delta-H       = 24999.9715166 J
profile heat input  = 25000 J
overall residual    = -0.0238830993812 J
max relative resid. = 9.55323975248e-7
liquid volume frac. = 0.5
Tmin/Tmax           = 728.9243 K / 1374.2274 K
```

能量残差相对 25 kJ 输入低于 `1e-6`，因此本 DEMO 的热输入到总能量闭合通过。这里的 `Qin` 是 profile 面积积分，原生 report 交叉结果见第 3 节；没有伪造或反推 Fluent 结果。

## 6. 网格与时间步长敏感性

机器可读数据保存在 [`m16_validation_summary.csv`](../validation/m1_6_coupled_demo/m16_validation_summary.csv)。

| 算例 | cells | dt | 末时刻 | 总焓增量 | 能量残差 | 液相率 | Tmin/Tmax |
|---|---:|---:|---:|---:|---:|---:|---:|
| base | 228 | 0.010 s | 0.5 s | 24999.9715 J | −0.02388 J | 0.5000 | 728.9 / 1374.2 K |
| time refined | 228 | 0.005 s | 0.5 s | 24999.9734 J | −0.02254 J | 0.5000 | 728.3 / 1374.8 K |
| mesh refined | 1824 | 0.010 s | 0.5 s | 24999.9777 J | −0.01929 J | 0.47995 | 697.5 / 1892.7 K |

时间步长加密保持液相率不变，最高温度只改变约 `0.54 K`，说明本测试时间离散影响较小。网格加密后总能量仍闭合，但最高温度改变约 `518 K`、液相率改变约 `0.020`，说明局部热场对网格仍敏感。本项已记录为剩余问题，不能宣称网格无关。

## 7. 真实运行文件与日志

已保存的可重放入口：

- `validation/m1_6_coupled_demo/m16_build_two_region.py` / `.scscript`：SpaceClaim 两区域几何；
- `validation/m1_6_coupled_demo/m16_workbench_mesh2.wbjn`：Workbench 几何刷新与网格更新；
- `validation/m1_6_coupled_demo/m16_compile_udf.jou`：Fluent UDF 编译/加载；
- `validation/m1_6_coupled_demo/m16_base_run.jou`：基准算例；
- `validation/m1_6_coupled_demo/m16_refined_run.jou`：全体 cell register 一次加密后的网格敏感性算例；
- `validation/m1_6_coupled_demo/m16_time_refined_run.jou`：时间步长敏感性算例；
- `validation/m1_6_coupled_demo/m16_report_probe.jou`、`m16_refined_report.jou`、`m16_time_report.jou`：原生热传递报告；
- `validation/m1_6_coupled_demo/m16_combined_udf.c`：DEMO UDF；
- `validation/m1_6_coupled_demo/m16_validation_summary.csv`：验证汇总数据。

本机还保留真实 Fluent case/data 和 transcript，例如：

```text
validation/m1_6_coupled_demo/m16_base_run10.cas.h5
validation/m1_6_coupled_demo/m16_base_run10.dat.h5
validation/m1_6_coupled_demo/m16_refined_run.cas.h5
validation/m1_6_coupled_demo/m16_refined_run.dat.h5
validation/m1_6_coupled_demo/m16_time_refined_run.cas.h5
validation/m1_6_coupled_demo/m16_time_refined_run.dat.h5
validation/m1_6_coupled_demo/m16_base_run10.trn
validation/m1_6_coupled_demo/m16_refined_run.trn
validation/m1_6_coupled_demo/m16_time_refined_run.trn
validation/m1_6_coupled_demo/m16_report_probe4.trn
validation/m1_6_coupled_demo/m16_refined_report.trn
validation/m1_6_coupled_demo/m16_time_report.trn
```

这些二进制结果和大型 transcript 按验证目录的 `.gitignore` 保留在本机；journal、源代码、报告和汇总 CSV 提交到分支。实际 Workbench/SpaceClaim 原生文件位于 `C:\Users\admin\m16_demo\`，Fluent 读取的 ASCII staging mesh 位于 `C:\Users\admin\m16_demo\m16_msh_stage\m16_base.msh`。

## 8. 失败、修复与剩余问题

| 现象 | 真实处理 |
|---|---|
| SpaceClaim V21 API 不兼容 | 改用 V261 API，几何脚本和体积报告通过 |
| Workbench 首次请求不存在的 `Model` container | 改用真实 `Mesh` container，网格成功更新 |
| UDF 初始 include/header/重复符号编译失败 | 逐次改为单一 `m16_combined_udf.c`，最终 `m16_demo_lib8` Host/Node 编译加载成功 |
| run7 hook 设置在第二槽位停住 | 在第二个 execute-at-end 槽位显式留空 |
| run8 固体不升温 | 日志显示默认选中 M1.5 wall profile；run10 显式选择 M1.6 profile 修复 |
| run9 UDF 名称填入 profile 名而非 data 名 | 在 profile 名槽位先留空，再填入 `m16_bottom_heat_flux::m16_demo_lib8` |
| UDF coupled-wall `BOUNDARY_HEAT_FLUX` 诊断值接近零 | 不用它替代原生 report；以 `/report/fluxes/heat-transfer` 的成对壁面结果为界面闭合证据 |

剩余问题：

1. 网格加密改变局部温度和液相率，正式复现仍需独立网格研究和合理的界面/底部解析度。
2. DEMO 物性必须替换为经用户确认的论文物性；不得把本报告的铝热物性写入正式复现结论。
3. 250 W 旧探针应按其原始 journal 单独重放，明确它的 zone、场函数、面积平均/积分和符号定义；当前 M1.6 已完成正确边界的原生报告闭合。
4. many-to-many mesh interface 在 Fluent 2026 R1 日志中提示未来版本弃用；正式模型应按版本建议转换为 one-to-one pairing 并重新验收。
5. 仓库 `tests/run_tests.sh` 依赖 Bash 和 `cc`；本机均不可用，Fluent 自带 Clang 又缺少 Visual Studio 标准头文件，因此本轮没有把 C 单元测试伪报为通过。Fluent 内部 UDF 编译/加载则有真实成功 transcript。

在上述剩余问题解决前，不开始 M2 完整三维炉膛、燃烧/辐射模型或 20 组参数优化。
