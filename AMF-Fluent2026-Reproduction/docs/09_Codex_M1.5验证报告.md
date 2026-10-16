# M1.5 Fluent 2026 环境验证与一维熔化基准测试报告

- 项目：AMF-Fluent2026-Reproduction
- 仓库：`zbnoop/men`
- 工作分支：`amf-fluent2026`
- 验证日期：2026-10-16
- 基线提交：`ebf62d5`
- 范围：Fluent 2026 R1 环境、编译型 UDF、隔离 DEMO 一维铝块导热/熔化基准
- 结论：**M1.5 Fluent DEMO 运行 PASS；PyFluent 连接器仍 BLOCKED；不代表论文模型复现完成**

本阶段严格限制在隔离的 `validation/m1_5_demo` DEMO 环境。演示物性不能用于正式论文复现；没有建立完整炉膛、没有运行 20 组参数优化，也没有修改 `main` 分支。

## 1. 最终结论

本轮防火墙/连接问题重试后，已经获得真实 Fluent 2026 R1 求解器运行：

1. Fluent 2026 R1（Build Id `10262`）在本机启动，许可证 `1055@localhost` 可用，内置 Clang/LLD 编译并加载 UDF 成功。
2. Workbench `Fluid Flow` 路线生成的 2D 四边形网格被 Fluent 直接读取；缩放后网格检查通过。
3. UDF 等效比热容、导热系数、热流 profile 和能量审计均在 Fluent 中挂接；350 个时间步全部报告 `solution is converged`。
4. 70 s 末端的焓增与规定热输入闭合，最大绝对能量残差为 `2.5132237 J`，最大相对残差约 `2.3601e-6`。
5. 熔化区被实际跨越：首次进入糊状区在 `t=4.6 s`；70 s 时温度定义的液相率为 `0.6171876`，潜热吸收为 `649898.54 J`。

这些是本机 Fluent DEMO 的真实结果，不是论文结果，也不证明 7050 铝合金物性、炉膛边界或论文图表已经复现。

## 2. Fluent 2026、许可证和 UDF 编译环境

| 项目 | 实测值 | 状态 |
|---|---|---|
| Fluent 可执行文件 | `C:\Program Files\ANSYS Inc\v261\fluent\ntbin\win64\fluent.exe` | PASS |
| Fluent 版本 | `ANSYS Fluent 2026 R1`，Build Id `10262`，版本 `26.1.0` | PASS |
| 许可证 | `1055@localhost` | PASS |
| 并行许可 | 启动横幅显示 `4-way parallel execution` | PASS |
| 编译器 | Fluent 内置 `Clang (builtin)`，`clang-cl` + `lld-link` | PASS |
| UDF 库 | `m15_demo_lib` | PASS |

最终编译重放文件为 [`compile_udf_final.jou`](../validation/m1_5_demo/compile_udf_final.jou)，transcript 为 `validation/m1_5_demo/m15_udf_compile_final2.trn`。Fluent 加载时实际列出：

```text
m15_equivalent_cp
m15_thermal_conductivity
m15_demo_wall_heat_flux
m15_energy_audit
```

该 UDF 采用 `DEFINE_SPECIFIC_HEAT` 返回 `cp(T)` 并通过 `*h` 返回一致的积分焓，执行末端审计使用 `DEFINE_EXECUTE_AT_END`；宏和编译行为按 Fluent v261 官方文档核对：[Fluent UDF Manual v261](https://ansyshelp.ansys.com/public/Views/Secured/corp/v261/en/pdf/Ansys_Fluent_UDF_Manual.pdf)。

PyFluent 仍未打通：按本机 v261、单进程、`-mpi=intel` 和隐藏 GUI 配置重试后，进程启动但 Python 端点保持 `disconnected` 并超时。该失败没有被绕过，也没有被当作 Fluent 计算结果。

## 3. DEMO 模型和物性

### 3.1 几何、网格和边界

- 2D 铝块，缩放后尺寸 `0.1 m × 0.01 m`，厚度按 `1 m` 计。
- 由 Workbench `Fluid Flow` 系统从 STEP 几何生成最小网格；Fluent 读取后得到 `40` 个四边形单元、`63` 个节点、`102` 个面、`1` 个边界 wall zone。
- Fluent 读入后执行 `/mesh/scale 1000 1000`；缩放后总体积（2D 面积/单位深度）为 `1.0e-3 m3`，网格检查 PASS。
- 单一 wall zone 由 UDF 按面心 x 坐标区分：左侧 `x=0` 面施加 `5.0e6 W/m2`，其余边界为零热流。这是为保持一维导热意图的 DEMO 处理，不改变生产几何或生产 UDF。
- 左侧热面长度按单位深度为 `0.01 m`，规定输入为 `5.0e6 × 0.01 = 5.0e4 W`。

最终 Fluent journal 为 [`fluent_m15_demo_run.jou`](../validation/m1_5_demo/fluent_m15_demo_run.jou)，原始真实 transcript 为 `validation/m1_5_demo/m15_fluent_demo_run2.trn`。

### 3.2 DEMO-only 物性

| 参数 | 数值 | 说明 |
|---|---:|---|
| 密度 | `2700 kg/m3` | DEMO，不是已核实的 7050 物性 |
| 固相/液相 cp | `900 / 1100 J/(kg K)` | DEMO |
| 固相线/液相线 | `748 / 911 K` | 项目当前相变区约定 |
| 潜热 | `390000 J/kg` | DEMO，不是论文给出的 7050 数据 |
| 固相/液相导热系数 | `235 / 62.8 W/(m K)` | DEMO 线性过渡 |
| 初温 | `300 K` | DEMO |
| 等效热流 | `5.0e6 W/m2` | 仅本次隔离测试 |

相变区采用常数等效比热：

```text
cp_mushy = 0.5*(cp_s + cp_l) + L/(Tl - Ts)
```

审计中的液相率为 `clip((T-748)/(911-748), 0, 1)` 的温度定义值，不是 Fluent 内置熔化模型的液相率。

## 4. Fluent 实际运行设置

```text
/file/read-case "C:/Users/admin/m15_retry_demo/FFF.msh"
/mesh/scale 1000 1000
/define/models/energy yes
UDF: m15_equivalent_cp, m15_thermal_conductivity
wall profile: m15_demo_wall_heat_flux
execute-at-end: m15_energy_audit
unsteady: first order
time step: 0.2 s
time steps: 350
max iterations/time step: 10
```

读入路径使用 ASCII 临时目录，是因为 Fluent 直接读取中文工作区路径时曾真实返回 `File not found`；ASCII staging 成功后，Fluent 正常读入同一 Workbench 网格。该路径编码问题不是网格质量结论。

## 5. 真实 Fluent 结果和能量闭合

本次运行总时间 `70 s`，每个时间步均出现 `solution is converged`。`m15_energy.csv` 由 UDF 在 Fluent 运行期间写出；关键数据如下：

| 指标 | 实测值 |
|---|---:|
| 时间步数 / 时间步长 | `350 / 0.2 s` |
| 末端时间 | `70 s` |
| 2D 单位深度体积 | `0.001 m3` |
| 末端焓增 `delta_enthalpy` | `3500001.52137 J` |
| 末端规定热输入 | `3500000 J` |
| 末端能量残差 | `1.5213675 J` |
| 全程最大绝对残差 | `2.5132237 J` |
| 全程最大相对残差 | `2.3601e-6` |
| 首次进入糊状区 | `t=4.6 s` |
| 首次糊状区潜热 | `510.689 J` |
| 末端潜热吸收 | `649898.543254 J` |
| 潜热总容量 `rho*V*L` | `1053000 J` |
| 末端温度定义液相率 | `0.6171876004` |
| 末端 `Tmin / Tmax` | `753.1587 / 3200.6764 K` |

潜热账本的末端比值为 `649898.543254 / 1053000 = 0.6171876004`，与温度定义液相率一致。末端最高温度很高，来自小网格、恒定高热流和单位深度 DEMO 的数值设置；它是诊断信号，不应解释为物理炉膛温度或论文预测。

### 5.1 能量审计口径

本次 `hot_q_in` 是 UDF 对**已施加 DEMO 热流 profile 的热面面积积分**，并通过时间积分得到规定热输入。它不是独立读取 Fluent `WALL_HEAT_FLUX` 报告量的第二套测量。

此前探针中 `WALL_HEAT_FLUX` 读数约 `250 W`，与规定 profile 的 `50000 W` 不一致；因此没有掩盖冲突，而是将审计口径改为已知 profile 的面积积分，并在本报告中明确。这样验证的是“规定输入 → 固体积分焓”的数值闭合，不等同于边界通量后处理独立交叉验证。

## 6. 失败、修复与真实日志

| 问题 | 真实现象 | 处理/结论 |
|---|---|---|
| PyFluent | v261 进程启动但 Python session 长时间 `disconnected` | 保留为 BLOCKED；改用直接 Fluent journal，未伪造连接结果 |
| 中文路径 | `/file/read-case` 返回 `File not found` | 使用 `C:\Users\admin\m15_retry_demo` ASCII staging；同一 Workbench 网格成功读入 |
| Gmsh Msh | Fluent `/file/read-case` 返回 `Error: Null Domain Pointer` | 不再使用 Gmsh 结果；改用 Workbench 生成的 `.msh` |
| t=0 审计回调 | 初版 UDF 在未初始化场上访问 cell/face 数据，Fluent 真实 `SIGSEGV` | `CURRENT_TIME <= 0` 直接返回；后续 smoke 和长时运行稳定 |
| 并行审计 CSV | node 端有数值但 host CSV 初版为零 | 加入 `node_to_host_real_*`，最终 CSV 与 Fluent Message 一致 |
| 边界通量交叉读数 | `WALL_HEAT_FLUX` 约 250 W，规定 profile 输入为 50 kW | 不隐藏冲突；审计明确采用 profile 面积积分，并列为剩余验证限制 |
| transcript 覆盖 | 重放 journal 时出现覆盖提示导致中止 | 最终 journal 使用唯一 transcript 名称 `m15_udf_compile_final2.trn`、`m15_fluent_demo_run2.trn` |

相关探针 journal 保存在 `validation/m1_5_demo/`；最终复现入口为上述两个 `.jou` 文件。Gmsh 失败 transcript 为 `m15_gmsh_mesh_probe.trn`；t=0 smoke 修复后的 transcript 为 `m15_fluent_smoke_final.trn`。

## 7. 预检查和单元测试

- `python scripts/preflight.py`：**PASS**。仍明确报告 exact reproduction 被 7050 物性、详细 CAD、燃烧器时序等资料阻断。
- `bash tests/run_tests.sh`：**未能直接执行**，本机 PowerShell 环境没有 `bash`，真实错误为 `bash is not recognized`。
- 使用 Fluent 安装自带 MinGW/GCC 重放 `tests/test_material_properties.c` 的实际编译命令：**PASS**，输出 `PASS demo math tests; Fluent solver not tested`。
- Fluent 编译/加载 `m15_demo_udf.c`：**PASS**。
- Fluent 350 步 DEMO：**PASS**，收敛、潜热吸收和规定输入—焓增账本通过。

## 8. 剩余问题和禁止外推

1. PyFluent IPC/隐藏会话仍未解决；当前流程依赖直接 Fluent journal。
2. Workbench 网格生成可用，但目前通过 ASCII staging 规避中文路径；网格只有一个 wall zone，正式模型需要明确边界命名和可重放的 Fluent Meshing 导出步骤。
3. `WALL_HEAT_FLUX` 与规定 profile 的独立后处理读数仍需在正式边界拓扑上解释和复核。
4. DEMO 使用 `rho=2700`、`cp=900/1100`、`L=390000` 等演示值；在用户核实 7050 固/液相比热、密度、导热系数、固液相线和潜热前，不能取消生产 UDF 门禁。
5. 仍缺论文正式复现所需的完整几何、燃气/辐射边界、烧嘴换向时序及 `400/800` 参数冲突裁决。
6. 本阶段没有开始完整炉膛建模，也没有运行 20 组参数优化。

因此，本报告的 PASS 只覆盖“本机 Fluent 环境 + 隔离 DEMO 一维等效比热容基准”，不覆盖论文物理真实性或正式结果复现。

## 9. 可重放文件

- UDF 源码：[`m15_demo_udf.c`](../validation/m1_5_demo/m15_demo_udf.c)
- UDF 编译/加载 journal：[`compile_udf_final.jou`](../validation/m1_5_demo/compile_udf_final.jou)
- Fluent DEMO journal：[`fluent_m15_demo_run.jou`](../validation/m1_5_demo/fluent_m15_demo_run.jou)
- 离线参考测试：[`m15_reference_1d.py`](../validation/m1_5_demo/m15_reference_1d.py)
- 初始审计报告：[`08_Codex_初次审计报告.md`](08_Codex_初次审计报告.md)
