{{#address}}
BOARD_KERNEL_CMDLINE += \
	memmap={{{size}}}\$${{{address}}} \
	ramoops.mem_address={{{address}}} \
	ramoops.mem_size={{{size}}}
{{/address}}
{{#record_size}}
BOARD_KERNEL_CMDLINE += \
	ramoops.record_size={{{record_size}}}
{{/record_size}}

{{#console_size}}
BOARD_KERNEL_CMDLINE += \
	ramoops.console_size={{{console_size}}}
{{/console_size}}

{{#ftrace_size}}
BOARD_KERNEL_CMDLINE += \
	ramoops.ftrace_size={{{ftrace_size}}}
{{/ftrace_size}}

{{#dump_oops}}
BOARD_KERNEL_CMDLINE += \
	ramoops.dump_oops={{{dump_oops}}}
{{/dump_oops}}

{{#ecc}}
BOARD_KERNEL_CMDLINE += \
	ramoops.ecc={{{ecc}}}
{{/ecc}}
