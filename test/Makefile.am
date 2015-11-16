IJVM_FILES = 					\
	test-asm.j				\
	test-getchar.j 				\
	test-iinc.j				\
	test-imul.j				\
	test-main.j				\
	test-min.j				\
	test-putchar.j				\
	test-sign.j				\
	test-sim.j				\
	test-iconst-0.j

test-ijvm-asm:
	-(for f in $(IJVM_FILES); do ../ijvm-asm $$f; done) >test-output 2>&1 
	diff test-output ijvm-asm-output && rm test-output

EXTRA_DIST =					\
	test-asm.j				\
	test-asm.run				\
	test-getchar.j				\
	test-iinc.j				\
	test-imul.j				\
	test-main.j				\
	test-min.j				\
	test-putchar.j				\
	test-sign.j				\
	test-sim.j				\
	check-error.mic				\
	layout-error.mic			\
	parse-error.mic				\
	gcd.mal					\
	ijvm-iconst0.mal			\
	ijvm.mal				\
	test-iconst-0.j				\
	ijvm-iconst0.spec
