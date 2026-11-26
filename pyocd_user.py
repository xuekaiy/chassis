"""
pyOCD 用户脚本 —— 在连接 STM32H723 之前先把 DBGMCU_CR 配好。

为什么需要这个脚本
------------------
pyOCD 连上 DAP 之后，会去探测 Cortex-M7 的 PPB ROM 表，表里第 0 项就是 SCS
(0xE000E000 起始的 System Control Space)。在 STM32H7 上，如果 DBGMCU 的调试
时钟 / 低功耗保持位没有打开，读 SCS 就会返回 memory transfer fault，在 VS Code
里表现为：

    Unable to read memory. (@ 0xe000ed14)     <- SCB->CCR
    Unable to read memory. (@ 0xe000ed10)     <- SCB->SCR
    Debug session ended: CMSIS_DAP@pyOCD (launch)

然后调试会话直接结束。这是 pyOCD 与 STM32H7 的已知问题，不是代码或无线调试器的
毛病，详见 pyocd/pyOCD#1148。

用法
----
1) 本文件放在工作区根目录，launch.json 里用
       "--script", "${workspaceFolder}/pyocd_user.py"
   传给 pyocd；
2) 或者把本文件复制到"运行 pyocd 时的当前目录"，pyOCD 会自动加载同名的
   pyocd_user.py，不需要额外参数。

兼容性
------
- 只对 STM32H7x2/H7x3/H74x/H75x 有意义（DBGMCU 在 APB-AP#2 的 0xE00E1004）。
  换别的芯片时把 DBGMCU_ADDR 改掉即可；写失败只会打印一条警告，不会中断调试。
- pyOCD 的内部钩子（wrap_task / insert_before）在不同版本之间可能改名，这里做了
  try/except，钩不上就只打印警告，改用 --pack 让 Pack 自带的 "Unlock Debug"
  调试序列去配置 DBGMCU。
"""

# DBGMCU_BASE = 0xE00E1000（见 STM32H723 参考手册），CR 在 +0x04
DBGMCU_ADDR = 0xE00E1004

# bit0-2   : DBGSLEEP_D1 / DBGSTOP_D1 / DBGSTBY_D1（低功耗下保持调试）
# bit20-22 : TRACECLKEN 等调试时钟使能
DBGMCU_VALUE_PRIMARY = 0x00700007
DBGMCU_VALUE_FALLBACK = 0x00670007

_hook_installed = False


def _configure_dbgmcu(target):
    """把 DBGMCU_CR 写进去，并回读确认。失败只告警，不抛异常。"""

    # 首选：走 APB-AP#2 的普通内存访问（AP 已建好之后才可用）
    for value in (DBGMCU_VALUE_PRIMARY, DBGMCU_VALUE_FALLBACK):
        try:
            target.aps[2].write_memory(DBGMCU_ADDR, value)
            readback = target.aps[2].read_memory(DBGMCU_ADDR)
            print("[pyocd_user] DBGMCU_CR(0x%08X) = 0x%08X (readback 0x%08X)"
                  % (DBGMCU_ADDR, value, readback))
            return
        except Exception as exc:
            print("[pyocd_user] DBGMCU_CR write 0x%08X failed: %s" % (value, exc))

    # 兜底：直接写 AP 寄存器（维护者在 #1148 里给出的写法）
    try:
        target.dp.write_ap(0x04, DBGMCU_ADDR)         # TAR
        target.dp.write_ap(0x0C, DBGMCU_VALUE_PRIMARY)  # DRW
        print("[pyocd_user] DBGMCU_CR written through dp.write_ap()")
        return
    except Exception as exc:
        print("[pyocd_user] dp.write_ap fallback failed: %s" % exc)

    print("[pyocd_user] WARNING: DBGMCU_CR 未能配置，"
          "请改用 --pack 让 Pack 的 Unlock Debug 序列生效")


def will_init_target(target, init_sequence):
    """pyOCD 在初始化目标前调用本钩子。"""
    global _hook_installed

    if _hook_installed:
        return

    def _install():
        global _hook_installed
        try:
            # 在 discovery 任务的 find_components 之前插入：此时 AP 已创建，
            # 但还没开始探测 ROM 表 / SCS。
            init_sequence.wrap_task(
                'discovery',
                lambda seq: seq.insert_before(
                    'find_components',
                    ('configure_dbgmcu', lambda: _configure_dbgmcu(target)),
                ),
            )
            _hook_installed = True
            print("[pyocd_user] 已挂钩：探测组件前会先配置 DBGMCU_CR")
        except Exception as exc:
            print("[pyocd_user] 挂钩失败（%s），继续不带钩子的连接" % exc)

    _install()
