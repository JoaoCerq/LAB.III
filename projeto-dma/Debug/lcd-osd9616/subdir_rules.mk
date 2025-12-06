################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Each subdirectory must supply rules for building sources it contributes
lcd-osd9616/%.obj: ../lcd-osd9616/%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: C5500 Compiler'
	"C:/ti/ccs1281/ccs/tools/compiler/c5500_4.4.1/bin/cl55" -v5502 --memory_model=large -g --include_path="C:/Users/joao0/workspace_v12/Project" --include_path="C:/ti/ccs1281/ccs/tools/compiler/c5500_4.4.1/include" --include_path="C:/Users/joao0/workspace_v12/Project/include" --define=c5502 --define=CHIP_5502 --display_error_number --diag_warning=225 --ptrdiff_size=32 --preproc_with_compile --preproc_dependency="lcd-osd9616/$(basename $(<F)).d_raw" --obj_directory="lcd-osd9616" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '


