################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Each subdirectory must supply rules for building sources it contributes
drivers/%.obj: ../drivers/%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: Arm Compiler'
	"C:/ti/ccs1110/ccs/tools/compiler/ti-cgt-arm_20.2.5.LTS/bin/armcl" -mv7M4 --code_state=16 --float_support=FPv4SPD16 -me -Ooff --include_path="C:/ti/ccs1110/ccs/tools/compiler/ti-cgt-arm_20.2.5.LTS/include" --include_path="C:/Users/Daniel/Documents/Universidad/Cuarto/Microbotica/Practicas/P2/NUEVO_PRACTICA/P2/P2" --include_path="C:/Users/Daniel/Documents/Universidad/Cuarto/Microbotica/Practicas/P2/NUEVO_PRACTICA/P2/P2/FreeRTOS/Source/include" --include_path="C:/Users/Daniel/Documents/Universidad/Cuarto/Microbotica/Practicas/P2/NUEVO_PRACTICA/P2/P2/FreeRTOS/Source/portable/CCS/ARM_CM4F" --include_path="C:/Users/Daniel/Documents/Universidad/Cuarto/Microbotica/Practicas/P2/NUEVO_PRACTICA/P2/P2/remotelink" --define=ccs="ccs" --define=TARGET_IS_BLIZZARD_RB1 --define=UART_BUFFERED --define=WANT_CMDLINE_HISTORY --define=WANT_FREERTOS_SUPPORT --define=PART_TM4C123GH6PM --define=DEBUG -g --c89 --gcc --diag_warning=225 --diag_wrap=off --display_error_number --abi=eabi --preproc_with_compile --preproc_dependency="drivers/$(basename $(<F)).d_raw" --obj_directory="drivers" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '


