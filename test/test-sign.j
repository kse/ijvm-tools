// Test af grænsetilfælde for datatyper

.method main
.define k = 50

	ldc_w 32
	ldc_w -32
	ldc_w --32
	ldc_w 1 - 54
	ldc_w 1 + -56
	ldc_w 70 + - 56

	ldc_w 25 - 23

//	ldc_w -0x80000000 - 2
//	ldc_w 0x7ffffff0 - -20
//	ldc_w -0x7ffffff0 + -20
//	ldc_w 0x7ffffff0 + 20

	ldc_w 0x7ffffff0 + k

//	ldc_w --0x80000000
//	ldc_w -0x80000001
//	ldc_w 0x800000011234234
	ireturn
