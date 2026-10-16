# M1.5 Fluent 2026 环境验证与一维熔化基准测试报告

- 项目：AMF-Fluent2026-Reproduction
- 仓库：`zbnoop/men`
- 工作分支：`amf-fluent2026`
- 本次开始提交：`ac3d6a8b864c43e9b189343d2c71cadbb5a1b23f`
- 日期：2026-10-16
- 状态：**部分完成；Fluent 一维瞬态被本机网格入口阻断**

## 1. 范围和结论

本阶段只验证 Fluent 2026 R1、本项目演示 UDF 编译链和最小一维 DEMO。没有建立完整炉膛，没有启动 20 组参数优化，也没有把离线参考计算或 UDF 编译日志写成 Fluent 仿真结果。

已完成：

1. 本机 Fluent 2026 R1 安装和许可证真实启动验证。
2. `m15_demo_udf.c` 通过 Fluent 2026 R1 内置 Clang/LLD 编译并加载，三个函数符号被 Fluent 识别。
3. 隔离 DEMO 一维焓法参考测试通过：等效比热容跨固相线到液相线的潜热积分为 `390000 J/kg`，离散热输入与内部焓增的最大绝对差为 `3.7253e-6 J`。
4. 记录了真实的网格导入错误、CGNS 导入崩溃、Workbench Mesh 脚本错误和 PyFluent 连接失败。

未完成：

- 没有获得可由当前 v261 Fluent 求解器稳定读取的网格/案例，因此没有执行 Fluent 网格检查、材料挂接、边界条件设置或瞬态迭代。
- 因此本报告不声称已经得到 Fluent 的潜热吸收曲线、残差曲线或能量守恒结果。

## 2. Fluent 2026 安装、许可证和编译环境

### 2.1 本机安装检查

由本机 ANSYS 检查工具和文件版本检查确认：

| 项目 | 实测值 | 结果 |
|---|---|---|
| Fluent 可执行文件 | `C:\Program Files\ANSYS Inc\v261\fluent\ntbin\win64\fluent.exe` | 存在 |
| Fluent 版本 | `26.1.0`；启动横幅为 `ANSYS Fluent 2026 R1`，Build Id `10262` | PASS |
| Workbench | `C:\Program Files\ANSYS Inc\v261\Framework\bin\Win64\RunWB2.exe`，版本 `26,01,00,2026020218` | 存在 |
| 许可证服务器 | `1055@localhost` | 可连接 |
| 并行许可 | Fluent 启动横幅：`Your license enables 4-way parallel execution.` | PASS |
| MPI | 直接 Fluent 启动实际选择 `msmpi` | PASS |

直接许可证 journal 为 `validation/m1_5_demo/license_probe.jou`；实测 Fluent 退出码为 `0`，并打印了上述许可证信息。

### 2.2 UDF 编译与加载

在隔离目录 `validation/m1_5_demo` 中执行了 `compile_udf_probe.jou`：

- Fluent 编译器：安装内置 `Clang (builtin)`。
- 编译器：`...\v261\fluent\ntbin\clang\bin\clang-cl`。
- 链接器：`...\v261\fluent\ntbin\clang\bin\lld-link`。
- 编译库：`m15_batch_lib`。
- Fluent 加载时实际列出：

  ```text
  m15_equivalent_cp
  m15_thermal_conductivity
  m15_energy_audit
  Done.
  ```

这证明的是 UDF 的编译和动态库加载，不证明 UDF 已在网格上运行。对应可重放文件是 `validation/m1_5_demo/compile_udf_probe.jou`，生成的 Fluent transcript 被 `.gitignore` 排除，关键行已记录在本报告。

### 2.3 PyFluent 连接器

曾按本机 Fluent 2026 配置尝试 PyFluent 隐藏会话，并分别测试 `-mpi=intel` 和 `-mpi=msmpi`。两次均返回 `pyfluent_connect_failed`，没有建立 Python 服务端点；随后检查确认会话为 disconnected。该连接失败没有被绕过，也没有被当成 Fluent 计算结果。直接 Fluent CLI 启动不受此连接器失败影响。

## 3. 最小 DEMO 模型设计

文件位于 `validation/m1_5_demo/`，与生产 UDF 和论文复现路径隔离。

### 3.1 几何和网格意图

- 2D 铝块：`0.1 m × 0.01 m`，厚度按 1 m 计。
- 方向：左侧为 `hot_wall`，右侧为绝热端；上下边为对称/绝热边界意图。
- Gmsh 几何脚本：`m15_slab.geo`。
- 生成的结构化候选网格：100 × 1 四边形单元、202 个节点；它只作为 DEMO 几何/导入探针，不能算作已被 Fluent 接受的网格。

### 3.2 演示物性

以下数值来自仓库已有 DEMO 约定，不是 7050 铝合金正式复现物性：

| 参数 | DEMO 值 |
|---|---:|
| `rho` | `2700 kg/m³` |
| `cp_s` / `cp_l` | `900 / 1100 J/(kg·K)` |
| 固相线 / 液相线 | `748 / 911 K` |
| 潜热 | `390000 J/kg` |
| `k_s` / `k_l` | `235 / 62.8 W/(m·K)` |
| 初温 / 热边界 | `300 / 1000 K` |

等效比热容在糊状区取：

```text
cp_mushy = 0.5*(cp_s + cp_l) + L/(Tl - Ts)
h(Ts→Tl) = 0.5*(cp_s + cp_l)*(Tl - Ts) + L
```

生产 UDF 仍保留真实物性门禁；本文件明确只能用于 M1.5 DEMO。

## 4. Fluent 网格路径实测日志、错误和处理

### 4.1 求解器直接读取网格

真实 journal 探针记录了以下结果：

1. `/file/read-mesh "m15_slab.msh"`：`Error: invalid command`，`Error Object: "read-mesh"`。
2. `/file/import/ansys-vol-mesh`：`Error: invalid command`，`Error Object: "ansys-vol-mesh"`。
3. `/file/import/cgns-vol-mesh`：`Error: invalid command`，`Error Object: "cgns-vol-mesh"`。
4. 在 `3ddp` 中 `/file/import/fluent-2d-mesh`：`Error: invalid command`，`Error Object: "fluent-2d-mesh"`。
5. 用 `-meshing` 启动 2D/2DDP Fluent：启动横幅明确打印 `Mesher mode is not supported - Starting Solver mode`；随后网格导入入口仍不可用。

这些不是“网格质量差”的结论，而是当前启动上下文没有暴露对应 Fluent Meshing TUI 入口。官方 v261 文档将 Fluent Meshing 文本命令和 Workbench 中的 Meshing 编辑器分开描述；见 [Fluent User's Guide v261](https://ansyshelp.ansys.com/public/views/secured/corp/v261/en/flu_ug/flu_ug.html) 和 [Fluent Text Command List v261](https://ansyshelp.ansys.com/public/Views/Secured/corp/v261/en/pdf/Ansys_Fluent_Text_Command_List.pdf)。

### 4.2 CGNS 崩溃

曾用 Gmsh 生成二维 `m15_slab.cgns`，并通过错误的 `file/import/cgns/mesh` 探针测试。Fluent 进程实际收到：

```text
Node 999999: Process ... Received signal SIGSEGV
fl_mpi2610.exe ended prematurely, exit code 2
Fatal failure in fluent ...
```

`fluent-999999-error.log` 中的调用栈包含 `vdm_CGNSDatNextIter` 和 `vdm_CGNSVLibOpen`。这次崩溃已保留为错误记录；没有再次把 CGNS 路线当作可用路线，也没有据此伪造网格或结果。

### 4.3 Workbench 路线

为避免把求解器上下文误认为 Meshing，上游尝试了隔离 Workbench：

- 工具封装先尝试 `Fluid Flow (Fluent)` / `Fluent`，真实返回“项目中未找到模板”；修正为本机实际模板名 `Fluid Flow` 后，系统和 STEP 几何载入成功。
- Mesh 编辑器窗口真实打开为 `AnsysWBU.exe`，标题 `Meshing`。
- 通过 Workbench `SendCommand` 尝试生成网格时，Meshing 真实弹出：`Script Error`、`Line: 0 Char: 0 Error: 缺少对象`、`Code: 800a138f`。
- `Mesh.Update()`/`Component.Update()` 返回后没有生成可交给 Fluent 的已验证网格文件；只得到几何/容器文件引用。因而没有继续进入 Fluent Setup。

Workbench 的 `Mesh.SendCommand` 能力及其“需先打开 Meshing 编辑器”的约束见 [Workbench Scripting Guide v261](https://ansyshelp.ansys.com/public/Views/Secured/corp/v261/en/pdf/Workbench_Scripting_Guide.pdf)。下一轮应在可用的 Fluent Meshing/Workbench 版本化入口中先完成网格生成与导出，再重放本阶段 Fluent journal。

## 5. 离线一维参考测试（非 Fluent 结果）

脚本：`validation/m1_5_demo/m15_reference_1d.py`。

它逐项复用了 `m15_demo_udf.c` 的 `cp(T)`、积分焓 `h(T)`、线性导热系数和液相率，并以焓为有限体积状态量；热边界输入和内部焓增使用同一个面通量账本。

真实运行输出：

```text
M15 reference checks: PASS
latent_integral_J_per_kg=390000
steps=100000
final_time_s=200.0
final_Tmin_K=803.3957958244922
final_Tmax_K=997.495370453254
final_liquid_fraction=0.6092197967441882
cumulative_input_J=202188271.2350829
final_delta_enthalpy_J=202188271.23508096
max_abs_energy_residual_J=3.725290298461914e-06
```

这个输出只能说明 DEMO 公式和离线离散能量账本自洽；它不是 Fluent residual、Fluent monitor 或 Fluent case/data 结果。

## 6. 预检查、单元测试和实际状态

### 6.1 仓库检查

`python scripts/preflight.py`：**PASS**。真实输出保留了既有数据门禁：

```text
PASS: internal transcription checks. Conflict 400 vs 800 remains OPEN.
BLOCKED for exact reproduction: 7050 cp, rho, latent heat, detailed CAD, burner-switch schedule.
```

### 6.2 单元测试

仓库 `tests/run_tests.sh` 直接执行失败，原因是本机 PowerShell 环境没有 `bash`：

```text
The term 'bash' is not recognized as a name of a cmdlet, function, script file, or executable program.
```

随后用 Fluent 安装自带 MinGW GCC 重放其实际编译命令，并执行生成的 `test_material_properties.exe`；测试通过，真实输出为：

```text
PASS demo math tests; Fluent solver not tested
```

因此“数学边界测试 PASS”与“Fluent 求解器尚未测试”在本报告中保持分离。

## 7. 尚缺物性、几何和运行参数

在允许移除生产门禁前，仍需补齐并逐项标注来源：

- 7050 铝合金 `rho(T)`、固/液相 `cp(T)`、`k(T)`、固相线/液相线、潜热；
- 初始温度、液相线以上状态及 UDF 焓参考点；
- 论文几何/CAD、块体尺寸、壁厚和有效厚度；
- 热边界的温度或热流定义、气固耦合方式、辐射/对流参数；
- 炉膛/燃烧器的开关时序和热源数据；
- 网格拓扑、边界命名和 Fluent 2026 可重放的 Meshing 导出步骤；
- 论文中 `400 K` 与 `800 K` 初温/边界冲突的证据裁决。

## 8. 后续解阻顺序

1. 在本机确认可运行的 Fluent Meshing/Workbench Mesh 入口，生成并导出可重放的最小 2D slab 网格。
2. 在同一 Fluent 版本重放 `compile_udf_probe.jou`，挂接 `m15_equivalent_cp` 和 `m15_thermal_conductivity`，启用 `m15_energy_audit`。
3. 做网格检查、初始化、1000 K 热边界瞬态运行，并以 Fluent 的真实 `m15_energy.csv` 与监视器做能量闭合检查。
4. 只有在 DEMO 结果和日志闭合后，才考虑 7050 物性和正式论文复现；本报告不授权移除生产门禁。
