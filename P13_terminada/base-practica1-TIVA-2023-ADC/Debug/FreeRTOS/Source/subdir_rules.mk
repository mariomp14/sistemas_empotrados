################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Each subdirectory must supply rules for building sources it contributes
FreeRTOS/Source/%.obj: ../FreeRTOS/Source/%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: Arm Compiler'
	"C:/ti/ccs1230/ccs/tools/compiler/ti-cgt-arm_20.2.7.LTS/bin/armcl" -mv7M4 --code_state=16 --float_support=FPv4SPD16 -me -Ooff --include_path="C:/ti/ccs1230/ccs/tools/compiler/ti-cgt-arm_20.2.7.LTS/include" --include_path="C:/Users/mario/Desktop/PORTATIL/carrera/CUARTO CURSO/Segundo cuatrimestre/empotrados/P13_terminada/base-practica1-TIVA-2023-ADC" --include_path="C:/Users/mario/Desktop/PORTATIL/carrera/CUARTO CURSO/Segundo cuatrimestre/empotrados/P13_terminada/base-practica1-TIVA-2023-ADC/FreeRTOS/Source/include" --include_path="C:/Users/mario/Desktop/PORTATIL/carrera/CUARTO CURSO/Segundo cuatrimestre/empotrados/P13_terminada/base-practica1-TIVA-2023-ADC/FreeRTOS/Source/portable/CCS/ARM_CM4F" --include_path="C:/Users/mario/Desktop/PORTATIL/carrera/CUARTO CURSO/Segundo cuatrimestre/empotrados/P13_terminada/base-practica1-TIVA-2023-ADC/remotelink" --define=ccs="ccs" --define=TARGET_IS_BLIZZARD_RB1 --define=UART_BUFFERED --define=WANT_CMDLINE_HISTORY --define=WANT_FREERTOS_SUPPORT --define=PART_TM4C123GH6PM --define=DEBUG -g --c89 --gcc --diag_warning=225 --diag_wrap=off --display_error_number --abi=eabi --preproc_with_compile --preproc_dependency="FreeRTOS/Source/$(basename $(<F)).d_raw" --obj_directory="FreeRTOS/Source" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '


