# groups.cmake

# group Application/MDK-ARM
add_library(Group_Application_MDK-ARM OBJECT
  "${SOLUTION_ROOT}/startup_stm32h723xx.s"
)
target_include_directories(Group_Application_MDK-ARM PUBLIC
  $<TARGET_PROPERTY:${CONTEXT},INTERFACE_INCLUDE_DIRECTORIES>
)
target_compile_definitions(Group_Application_MDK-ARM PUBLIC
  $<TARGET_PROPERTY:${CONTEXT},INTERFACE_COMPILE_DEFINITIONS>
)
add_library(Group_Application_MDK-ARM_ABSTRACTIONS INTERFACE)
target_link_libraries(Group_Application_MDK-ARM_ABSTRACTIONS INTERFACE
  ${CONTEXT}_ABSTRACTIONS
)
target_compile_options(Group_Application_MDK-ARM PUBLIC
  $<TARGET_PROPERTY:${CONTEXT},INTERFACE_COMPILE_OPTIONS>
)
target_link_libraries(Group_Application_MDK-ARM PUBLIC
  Group_Application_MDK-ARM_ABSTRACTIONS
)
set(COMPILE_DEFINITIONS
  STM32H723xx
  _RTE_
)
cbuild_set_defines(AS_ARM COMPILE_DEFINITIONS)
set_source_files_properties("${SOLUTION_ROOT}/startup_stm32h723xx.s" PROPERTIES
  COMPILE_FLAGS "${COMPILE_DEFINITIONS}"
)

# group Application/User/Core
add_library(Group_Application_User_Core OBJECT
  "${SOLUTION_ROOT}/../Core/Src/main.c"
  "${SOLUTION_ROOT}/../Core/Src/gpio.c"
  "${SOLUTION_ROOT}/../Core/Src/freertos.c"
  "${SOLUTION_ROOT}/../Core/Src/dma.c"
  "${SOLUTION_ROOT}/../Core/Src/fdcan.c"
  "${SOLUTION_ROOT}/../Core/Src/tim.c"
  "${SOLUTION_ROOT}/../Core/Src/usart.c"
  "${SOLUTION_ROOT}/../Core/Src/stm32h7xx_it.c"
  "${SOLUTION_ROOT}/../Core/Src/stm32h7xx_hal_msp.c"
  "${SOLUTION_ROOT}/../Core/Src/stm32h7xx_hal_timebase_tim.c"
)
target_include_directories(Group_Application_User_Core PUBLIC
  $<TARGET_PROPERTY:${CONTEXT},INTERFACE_INCLUDE_DIRECTORIES>
)
target_compile_definitions(Group_Application_User_Core PUBLIC
  $<TARGET_PROPERTY:${CONTEXT},INTERFACE_COMPILE_DEFINITIONS>
)
add_library(Group_Application_User_Core_ABSTRACTIONS INTERFACE)
target_link_libraries(Group_Application_User_Core_ABSTRACTIONS INTERFACE
  ${CONTEXT}_ABSTRACTIONS
)
target_compile_options(Group_Application_User_Core PUBLIC
  $<TARGET_PROPERTY:${CONTEXT},INTERFACE_COMPILE_OPTIONS>
)
target_link_libraries(Group_Application_User_Core PUBLIC
  Group_Application_User_Core_ABSTRACTIONS
)

# group BSP
add_library(Group_BSP OBJECT
  "${SOLUTION_ROOT}/../BSP/Src/sysconfig.c"
  "${SOLUTION_ROOT}/../BSP/Src/BSP_Init.c"
  "${SOLUTION_ROOT}/../BSP/Src/PID.c"
  "${SOLUTION_ROOT}/../BSP/Src/Motor.c"
  "${SOLUTION_ROOT}/../BSP/Src/DJI_Motor.c"
  "${SOLUTION_ROOT}/../BSP/Src/DM_driver.c"
  "${SOLUTION_ROOT}/../BSP/Src/User_CAN.c"
  "${SOLUTION_ROOT}/../BSP/Src/Task_Can.c"
  "${SOLUTION_ROOT}/../BSP/Src/RemoteControl_Comm.c"
  "${SOLUTION_ROOT}/../BSP/Src/Debug_Uart.c"
)
target_include_directories(Group_BSP PUBLIC
  $<TARGET_PROPERTY:${CONTEXT},INTERFACE_INCLUDE_DIRECTORIES>
)
target_compile_definitions(Group_BSP PUBLIC
  $<TARGET_PROPERTY:${CONTEXT},INTERFACE_COMPILE_DEFINITIONS>
)
add_library(Group_BSP_ABSTRACTIONS INTERFACE)
target_link_libraries(Group_BSP_ABSTRACTIONS INTERFACE
  ${CONTEXT}_ABSTRACTIONS
)
target_compile_options(Group_BSP PUBLIC
  $<TARGET_PROPERTY:${CONTEXT},INTERFACE_COMPILE_OPTIONS>
)
target_link_libraries(Group_BSP PUBLIC
  Group_BSP_ABSTRACTIONS
)

# group Drivers/STM32H7xx_HAL_Driver
add_library(Group_Drivers_STM32H7xx_HAL_Driver OBJECT
  "${SOLUTION_ROOT}/../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_tim.c"
  "${SOLUTION_ROOT}/../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_tim_ex.c"
  "${SOLUTION_ROOT}/../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_cortex.c"
  "${SOLUTION_ROOT}/../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_rcc.c"
  "${SOLUTION_ROOT}/../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_rcc_ex.c"
  "${SOLUTION_ROOT}/../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_flash.c"
  "${SOLUTION_ROOT}/../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_flash_ex.c"
  "${SOLUTION_ROOT}/../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_gpio.c"
  "${SOLUTION_ROOT}/../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_hsem.c"
  "${SOLUTION_ROOT}/../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_dma.c"
  "${SOLUTION_ROOT}/../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_dma_ex.c"
  "${SOLUTION_ROOT}/../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_mdma.c"
  "${SOLUTION_ROOT}/../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_pwr.c"
  "${SOLUTION_ROOT}/../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_pwr_ex.c"
  "${SOLUTION_ROOT}/../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal.c"
  "${SOLUTION_ROOT}/../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_i2c.c"
  "${SOLUTION_ROOT}/../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_i2c_ex.c"
  "${SOLUTION_ROOT}/../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_exti.c"
  "${SOLUTION_ROOT}/../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_fdcan.c"
  "${SOLUTION_ROOT}/../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_uart.c"
  "${SOLUTION_ROOT}/../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_uart_ex.c"
)
target_include_directories(Group_Drivers_STM32H7xx_HAL_Driver PUBLIC
  $<TARGET_PROPERTY:${CONTEXT},INTERFACE_INCLUDE_DIRECTORIES>
)
target_compile_definitions(Group_Drivers_STM32H7xx_HAL_Driver PUBLIC
  $<TARGET_PROPERTY:${CONTEXT},INTERFACE_COMPILE_DEFINITIONS>
)
add_library(Group_Drivers_STM32H7xx_HAL_Driver_ABSTRACTIONS INTERFACE)
target_link_libraries(Group_Drivers_STM32H7xx_HAL_Driver_ABSTRACTIONS INTERFACE
  ${CONTEXT}_ABSTRACTIONS
)
target_compile_options(Group_Drivers_STM32H7xx_HAL_Driver PUBLIC
  $<TARGET_PROPERTY:${CONTEXT},INTERFACE_COMPILE_OPTIONS>
)
target_link_libraries(Group_Drivers_STM32H7xx_HAL_Driver PUBLIC
  Group_Drivers_STM32H7xx_HAL_Driver_ABSTRACTIONS
)

# group Drivers/CMSIS
add_library(Group_Drivers_CMSIS OBJECT
  "${SOLUTION_ROOT}/../Core/Src/system_stm32h7xx.c"
)
target_include_directories(Group_Drivers_CMSIS PUBLIC
  $<TARGET_PROPERTY:${CONTEXT},INTERFACE_INCLUDE_DIRECTORIES>
)
target_compile_definitions(Group_Drivers_CMSIS PUBLIC
  $<TARGET_PROPERTY:${CONTEXT},INTERFACE_COMPILE_DEFINITIONS>
)
add_library(Group_Drivers_CMSIS_ABSTRACTIONS INTERFACE)
target_link_libraries(Group_Drivers_CMSIS_ABSTRACTIONS INTERFACE
  ${CONTEXT}_ABSTRACTIONS
)
target_compile_options(Group_Drivers_CMSIS PUBLIC
  $<TARGET_PROPERTY:${CONTEXT},INTERFACE_COMPILE_OPTIONS>
)
target_link_libraries(Group_Drivers_CMSIS PUBLIC
  Group_Drivers_CMSIS_ABSTRACTIONS
)

# group Middlewares/FreeRTOS
add_library(Group_Middlewares_FreeRTOS OBJECT
  "${SOLUTION_ROOT}/../Middlewares/Third_Party/FreeRTOS/Source/croutine.c"
  "${SOLUTION_ROOT}/../Middlewares/Third_Party/FreeRTOS/Source/event_groups.c"
  "${SOLUTION_ROOT}/../Middlewares/Third_Party/FreeRTOS/Source/list.c"
  "${SOLUTION_ROOT}/../Middlewares/Third_Party/FreeRTOS/Source/queue.c"
  "${SOLUTION_ROOT}/../Middlewares/Third_Party/FreeRTOS/Source/stream_buffer.c"
  "${SOLUTION_ROOT}/../Middlewares/Third_Party/FreeRTOS/Source/tasks.c"
  "${SOLUTION_ROOT}/../Middlewares/Third_Party/FreeRTOS/Source/timers.c"
  "${SOLUTION_ROOT}/../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2/cmsis_os2.c"
  "${SOLUTION_ROOT}/../Middlewares/Third_Party/FreeRTOS/Source/portable/MemMang/heap_4.c"
  "${SOLUTION_ROOT}/../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F/port.c"
)
target_include_directories(Group_Middlewares_FreeRTOS PUBLIC
  $<TARGET_PROPERTY:${CONTEXT},INTERFACE_INCLUDE_DIRECTORIES>
)
target_compile_definitions(Group_Middlewares_FreeRTOS PUBLIC
  $<TARGET_PROPERTY:${CONTEXT},INTERFACE_COMPILE_DEFINITIONS>
)
add_library(Group_Middlewares_FreeRTOS_ABSTRACTIONS INTERFACE)
target_link_libraries(Group_Middlewares_FreeRTOS_ABSTRACTIONS INTERFACE
  ${CONTEXT}_ABSTRACTIONS
)
target_compile_options(Group_Middlewares_FreeRTOS PUBLIC
  $<TARGET_PROPERTY:${CONTEXT},INTERFACE_COMPILE_OPTIONS>
)
target_link_libraries(Group_Middlewares_FreeRTOS PUBLIC
  Group_Middlewares_FreeRTOS_ABSTRACTIONS
)
