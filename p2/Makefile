all: vm input_gen

vm: vm.c vm.h replacement.c pagetable.c pagetable.h disk.h disk.c
		gcc vm.c replacement.c pagetable.c disk.c -g -o vm

clean:
		rm -f vm input_gen
