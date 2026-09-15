obj-m += venus_enable.o
ccflags-y += -I$(obj)

targets += venus-enable.dtbo venus_overlay.h

quiet_cmd_venus_dtbo = DTC     $@
      cmd_venus_dtbo = $(objtree)/scripts/dtc/dtc -I dts -O dtb -o $@ $<

quiet_cmd_venus_header = GEN     $@
      cmd_venus_header = python3 -c 'import pathlib, sys; print("static const unsigned char venus_overlay[] = {" + ",".join(str(b) for b in pathlib.Path(sys.argv[1]).read_bytes()) + "};")' $< > $@

$(obj)/venus-enable.dtbo: $(src)/venus-enable.dts FORCE
	$(call if_changed,venus_dtbo)

$(obj)/venus_overlay.h: $(obj)/venus-enable.dtbo FORCE
	$(call if_changed,venus_header)

$(obj)/venus_enable.o: $(obj)/venus_overlay.h
