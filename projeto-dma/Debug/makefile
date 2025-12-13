################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

CG_TOOL_ROOT := C:/ti/ccs1281/ccs/tools/compiler/c5500_4.4.1

GEN_OPTS__FLAG := 
GEN_CMDS__FLAG := 

ORDERED_OBJS += \
"./main.obj" \
"./lcd-osd9616/exemplo_uso.obj" \
"./lcd-osd9616/lcd.obj" \
"./lib/bsl/ezdsp5502.obj" \
"./lib/bsl/ezdsp5502_gpio.obj" \
"./lib/bsl/ezdsp5502_i2c.obj" \
"./lib/bsl/ezdsp5502_i2cgpio.obj" \
"./lib/bsl/ezdsp5502_mcbsp.obj" \
"./src/drivers/aic/aic.obj" \
"./src/drivers/aic/aic3204_init.obj" \
"./src/drivers/dma/dma.obj" \
"./src/drivers/dma/vectors_dma4.obj" \
"./src/drivers/lcd/LCD.obj" \
"./src/effects/effect.obj" \
"./src/effects/effect_dma.obj" \
"./src/state_machine/state_machine.obj" \
"./src/states/state_initial.obj" \
"./src/states/state_running.obj" \
"../lnkx.cmd" \
"../lib/csl5502x.lib" \
"../lib/ezdsp5502bsl.lib" \
$(GEN_CMDS__FLAG) \
-l"C:/Users/joao0/workspace_v12/Project/projeto-dma/lib/csl5502x.lib" \
-l"C:/Users/joao0/workspace_v12/Project/projeto-dma/lib/ezdsp5502bsl.lib" \

-include ../makefile.init

RM := DEL /F
RMDIR := RMDIR /S/Q

# All of the sources participating in the build are defined here
-include sources.mk
-include subdir_vars.mk
-include lcd-osd9616/subdir_vars.mk
-include lib/bsl/subdir_vars.mk
-include lib/subdir_vars.mk
-include src/drivers/aic/subdir_vars.mk
-include src/drivers/dma/subdir_vars.mk
-include src/drivers/lcd/subdir_vars.mk
-include src/effects/subdir_vars.mk
-include src/state_machine/subdir_vars.mk
-include src/states/subdir_vars.mk
-include subdir_rules.mk
-include lcd-osd9616/subdir_rules.mk
-include lib/bsl/subdir_rules.mk
-include lib/subdir_rules.mk
-include src/drivers/aic/subdir_rules.mk
-include src/drivers/dma/subdir_rules.mk
-include src/drivers/lcd/subdir_rules.mk
-include src/effects/subdir_rules.mk
-include src/state_machine/subdir_rules.mk
-include src/states/subdir_rules.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C55_DEPS)),)
-include $(C55_DEPS)
endif
ifneq ($(strip $(C_UPPER_DEPS)),)
-include $(C_UPPER_DEPS)
endif
ifneq ($(strip $(S67_DEPS)),)
-include $(S67_DEPS)
endif
ifneq ($(strip $(S62_DEPS)),)
-include $(S62_DEPS)
endif
ifneq ($(strip $(S_DEPS)),)
-include $(S_DEPS)
endif
ifneq ($(strip $(OPT_DEPS)),)
-include $(OPT_DEPS)
endif
ifneq ($(strip $(C??_DEPS)),)
-include $(C??_DEPS)
endif
ifneq ($(strip $(ASM_UPPER_DEPS)),)
-include $(ASM_UPPER_DEPS)
endif
ifneq ($(strip $(S??_DEPS)),)
-include $(S??_DEPS)
endif
ifneq ($(strip $(C64_DEPS)),)
-include $(C64_DEPS)
endif
ifneq ($(strip $(CXX_DEPS)),)
-include $(CXX_DEPS)
endif
ifneq ($(strip $(S64_DEPS)),)
-include $(S64_DEPS)
endif
ifneq ($(strip $(INO_DEPS)),)
-include $(INO_DEPS)
endif
ifneq ($(strip $(CLA_DEPS)),)
-include $(CLA_DEPS)
endif
ifneq ($(strip $(S55_DEPS)),)
-include $(S55_DEPS)
endif
ifneq ($(strip $(SV7A_DEPS)),)
-include $(SV7A_DEPS)
endif
ifneq ($(strip $(C62_DEPS)),)
-include $(C62_DEPS)
endif
ifneq ($(strip $(C67_DEPS)),)
-include $(C67_DEPS)
endif
ifneq ($(strip $(PDE_DEPS)),)
-include $(PDE_DEPS)
endif
ifneq ($(strip $(K_DEPS)),)
-include $(K_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
ifneq ($(strip $(CC_DEPS)),)
-include $(CC_DEPS)
endif
ifneq ($(strip $(C++_DEPS)),)
-include $(C++_DEPS)
endif
ifneq ($(strip $(C43_DEPS)),)
-include $(C43_DEPS)
endif
ifneq ($(strip $(S43_DEPS)),)
-include $(S43_DEPS)
endif
ifneq ($(strip $(ASM_DEPS)),)
-include $(ASM_DEPS)
endif
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
ifneq ($(strip $(CPP_DEPS)),)
-include $(CPP_DEPS)
endif
ifneq ($(strip $(SA_DEPS)),)
-include $(SA_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 
EXE_OUTPUTS += \
Project.out 

EXE_OUTPUTS__QUOTED += \
"Project.out" 


# All Target
all: $(OBJS) $(CMD_SRCS) $(LIB_SRCS) $(GEN_CMDS)
	@$(MAKE) --no-print-directory -Onone "Project.out"

# Tool invocations
Project.out: $(OBJS) $(CMD_SRCS) $(LIB_SRCS) $(GEN_CMDS)
	@echo 'Building target: "$@"'
	@echo 'Invoking: C5500 Linker'
	"C:/ti/ccs1281/ccs/tools/compiler/c5500_4.4.1/bin/cl55" -v5502 --memory_model=large -g --define=c5502 --define=CHIP_5502 --display_error_number --diag_warning=225 --ptrdiff_size=32 -z -m"Project.map" --stack_size=0x200 --heap_size=0x400 -i"C:/ti/ccs1281/ccs/tools/compiler/c5500_4.4.1/lib" -i"C:/Users/joao0/workspace_v12/Project/projeto-dma/lib" -i"C:/Users/joao0/Downloads/ezdsp5502_v1/tests/lcd-osd9616" -i"C:/ti/ccs1281/ccs/tools/compiler/c5500_4.4.1/include" --reread_libs --display_error_number --warn_sections --xml_link_info="Project_linkInfo.xml" --rom_model --sys_stacksize=0x200 -o "Project.out" $(ORDERED_OBJS)
	@echo 'Finished building target: "$@"'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(EXE_OUTPUTS__QUOTED)
	-$(RM) "main.obj" "lcd-osd9616\exemplo_uso.obj" "lcd-osd9616\lcd.obj" "lib\bsl\ezdsp5502.obj" "lib\bsl\ezdsp5502_gpio.obj" "lib\bsl\ezdsp5502_i2c.obj" "lib\bsl\ezdsp5502_i2cgpio.obj" "lib\bsl\ezdsp5502_mcbsp.obj" "src\drivers\aic\aic.obj" "src\drivers\aic\aic3204_init.obj" "src\drivers\dma\dma.obj" "src\drivers\dma\vectors_dma4.obj" "src\drivers\lcd\LCD.obj" "src\effects\effect.obj" "src\effects\effect_dma.obj" "src\state_machine\state_machine.obj" "src\states\state_initial.obj" "src\states\state_running.obj" 
	-$(RM) "src\drivers\dma\vectors_dma4.d" 
	-$(RM) "main.d" "lcd-osd9616\exemplo_uso.d" "lcd-osd9616\lcd.d" "lib\bsl\ezdsp5502.d" "lib\bsl\ezdsp5502_gpio.d" "lib\bsl\ezdsp5502_i2c.d" "lib\bsl\ezdsp5502_i2cgpio.d" "lib\bsl\ezdsp5502_mcbsp.d" "src\drivers\aic\aic.d" "src\drivers\aic\aic3204_init.d" "src\drivers\dma\dma.d" "src\drivers\lcd\LCD.d" "src\effects\effect.d" "src\effects\effect_dma.d" "src\state_machine\state_machine.d" "src\states\state_initial.d" "src\states\state_running.d" 
	-@echo 'Finished clean'
	-@echo ' '

.PHONY: all clean dependents
.SECONDARY:

-include ../makefile.targets

