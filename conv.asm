.section .text
// fn: [cvtint] (vars: 128, total: 304)
.align 2
cvtint:
  sub sp, sp, 304
  stp x29, x30, [sp, 288]
  add x29, sp, 288
  stp x19, x20, [x29, -16]
  stp x21, x22, [x29, -32]
  str x0, [x29, -48]  // param v
  str x1, [x29, -56]  // param tam
  ldr w0, [x29, -56]
  mov w19, w0
  str w0, [x29, -80]
  ldr w0, [x29, -80]
  mov w19, w0
  mov w0, 0
  mov w1, w19
  cmp w1, w0
  cset w0, eq
  mov w19, w0
  mov w0, w19
  cmp w0, 0
  beq .B0
  mov w0, 0
  b 1f
  b .B1
.B0:
.B1:
  mov w0, 0
  mov w19, w0
  str w0, [x29, -96]
  mov w0, 0
  mov w19, w0
  strb w0, [x29, -112]
  mov w0, 0
  str w0, [sp, -16]!
  mov w0, 0
  ldr w1, [sp], 16
  mov w2, 1
  mul w1, w1, w2
  add w0, w0, w1
  ldr x2, [x29, -48]
  add x2, x2, x0
  ldrb w0, [x2]
  mov w19, w0
  strb w0, [x29, -128]
  ldrb w0, [x29, -128]
  mov w19, w0
  mov w0, 45
  mov w1, w19
  cmp w1, w0
  cset w0, eq
  mov w19, w0
  mov w0, w19
  cmp w0, 0
  beq .B2
  mov w0, 1
  mov w19, w0
  mov w0, w19
  strb w0, [x29, -112]
  mov w0, 1
  mov w19, w0
  mov w0, w19
  str w0, [x29, -96]
  b .B3
.B2:
  ldrb w0, [x29, -128]
  mov w19, w0
  mov w0, 43
  mov w1, w19
  cmp w1, w0
  cset w0, eq
  mov w19, w0
  mov w0, w19
  cmp w0, 0
  beq .B4
  mov w0, 1
  mov w19, w0
  mov w0, w19
  str w0, [x29, -96]
  b .B5
.B4:
.B5:
.B3:
  ldr w0, [x29, -80]
  str w0, [sp, -16]!
  ldr w0, [x29, -96]
  ldr w1, [sp], 16
  sub w0, w1, w0
  mov w19, w0
  str w0, [x29, -144]
  ldr w0, [x29, -144]
  mov w19, w0
  mov w0, 0
  mov w1, w19
  cmp w1, w0
  cset w0, eq
  mov w19, w0
  mov w0, w19
  cmp w0, 0
  beq .B6
  mov w0, 0
  b 1f
  b .B7
.B6:
.B7:
  ldr w0, [x29, -144]
  mov w19, w0
  mov w0, 4
  mov w1, w19
  cmp w1, w0
  cset w0, le
  mov w19, w0
  mov w0, w19
  cmp w0, 0
  beq .B8
  ldr w0, [x29, -144]
  mov w19, w0
  mov w0, 1
  mov w1, w19
  cmp w1, w0
  cset w0, eq
  mov w19, w0
  mov w0, w19
  cmp w0, 0
  beq .B9
  ldr w0, [x29, -96]
  str w0, [sp, -16]!
  mov w0, 0
  ldr w1, [sp], 16
  mov w2, 1
  mul w1, w1, w2
  add w0, w0, w1
  ldr x2, [x29, -48]
  add x2, x2, x0
  ldrb w0, [x2]
  str w0, [sp, -16]!
  mov w0, 48
  ldr w1, [sp], 16
  sub w0, w1, w0
  mov w19, w0
  mov w0, w19
  str w0, [x29, 288]
  b .B10
.B9:
  ldr w0, [x29, -144]
  mov w19, w0
  mov w0, 2
  mov w1, w19
  cmp w1, w0
  cset w0, eq
  mov w19, w0
  mov w0, w19
  cmp w0, 0
  beq .B11
  ldr w0, [x29, -96]
  str w0, [sp, -16]!
  mov w0, 0
  ldr w1, [sp], 16
  mov w2, 1
  mul w1, w1, w2
  add w0, w0, w1
  ldr x2, [x29, -48]
  add x2, x2, x0
  ldrb w0, [x2]
  str w0, [sp, -16]!
  mov w0, 48
  ldr w1, [sp], 16
  sub w0, w1, w0
  mov w19, w0  // salva em reg
  mov w0, 10
  mov w1, w19  // restaura do reg
  mul w0, w1, w0
  str w0, [sp, -16]!
  ldr w0, [x29, -96]
  str w0, [sp, -16]!
  mov w0, 1
  ldr w1, [sp], 16
  add w0, w1, w0
  str w0, [sp, -16]!
  mov w0, 0
  ldr w1, [sp], 16
  mov w2, 1
  mul w1, w1, w2
  add w0, w0, w1
  ldr x2, [x29, -48]
  add x2, x2, x0
  ldrb w0, [x2]
  str w0, [sp, -16]!
  mov w0, 48
  ldr w1, [sp], 16
  sub w0, w1, w0
  ldr w1, [sp], 16
  add w0, w1, w0
  mov w19, w0
  mov w0, w19
  str w0, [x29, 288]
  b .B12
.B11:
  ldr w0, [x29, -144]
  mov w19, w0
  mov w0, 3
  mov w1, w19
  cmp w1, w0
  cset w0, eq
  mov w19, w0
  mov w0, w19
  cmp w0, 0
  beq .B13
  ldr w0, [x29, -96]
  str w0, [sp, -16]!
  mov w0, 0
  ldr w1, [sp], 16
  mov w2, 1
  mul w1, w1, w2
  add w0, w0, w1
  ldr x2, [x29, -48]
  add x2, x2, x0
  ldrb w0, [x2]
  str w0, [sp, -16]!
  mov w0, 48
  ldr w1, [sp], 16
  sub w0, w1, w0
  mov w19, w0  // salva em reg
  mov w0, 100
  mov w1, w19  // restaura do reg
  mul w0, w1, w0
  str w0, [sp, -16]!
  ldr w0, [x29, -96]
  str w0, [sp, -16]!
  mov w0, 1
  ldr w1, [sp], 16
  add w0, w1, w0
  str w0, [sp, -16]!
  mov w0, 0
  ldr w1, [sp], 16
  mov w2, 1
  mul w1, w1, w2
  add w0, w0, w1
  ldr x2, [x29, -48]
  add x2, x2, x0
  ldrb w0, [x2]
  str w0, [sp, -16]!
  mov w0, 48
  ldr w1, [sp], 16
  sub w0, w1, w0
  mov w19, w0  // salva em reg
  mov w0, 10
  mov w1, w19  // restaura do reg
  mul w0, w1, w0
  ldr w1, [sp], 16
  add w0, w1, w0
  str w0, [sp, -16]!
  ldr w0, [x29, -96]
  str w0, [sp, -16]!
  mov w0, 2
  ldr w1, [sp], 16
  add w0, w1, w0
  str w0, [sp, -16]!
  mov w0, 0
  ldr w1, [sp], 16
  mov w2, 1
  mul w1, w1, w2
  add w0, w0, w1
  ldr x2, [x29, -48]
  add x2, x2, x0
  ldrb w0, [x2]
  str w0, [sp, -16]!
  mov w0, 48
  ldr w1, [sp], 16
  sub w0, w1, w0
  ldr w1, [sp], 16
  add w0, w1, w0
  mov w19, w0
  mov w0, w19
  str w0, [x29, 288]
  b .B14
.B13:
  ldr w0, [x29, -96]
  str w0, [sp, -16]!
  mov w0, 0
  ldr w1, [sp], 16
  mov w2, 1
  mul w1, w1, w2
  add w0, w0, w1
  ldr x2, [x29, -48]
  add x2, x2, x0
  ldrb w0, [x2]
  str w0, [sp, -16]!
  mov w0, 48
  ldr w1, [sp], 16
  sub w0, w1, w0
  mov w19, w0  // salva em reg
  mov w0, 1000
  mov w1, w19  // restaura do reg
  mul w0, w1, w0
  str w0, [sp, -16]!
  ldr w0, [x29, -96]
  str w0, [sp, -16]!
  mov w0, 1
  ldr w1, [sp], 16
  add w0, w1, w0
  str w0, [sp, -16]!
  mov w0, 0
  ldr w1, [sp], 16
  mov w2, 1
  mul w1, w1, w2
  add w0, w0, w1
  ldr x2, [x29, -48]
  add x2, x2, x0
  ldrb w0, [x2]
  str w0, [sp, -16]!
  mov w0, 48
  ldr w1, [sp], 16
  sub w0, w1, w0
  mov w19, w0  // salva em reg
  mov w0, 100
  mov w1, w19  // restaura do reg
  mul w0, w1, w0
  ldr w1, [sp], 16
  add w0, w1, w0
  str w0, [sp, -16]!
  ldr w0, [x29, -96]
  str w0, [sp, -16]!
  mov w0, 2
  ldr w1, [sp], 16
  add w0, w1, w0
  str w0, [sp, -16]!
  mov w0, 0
  ldr w1, [sp], 16
  mov w2, 1
  mul w1, w1, w2
  add w0, w0, w1
  ldr x2, [x29, -48]
  add x2, x2, x0
  ldrb w0, [x2]
  str w0, [sp, -16]!
  mov w0, 48
  ldr w1, [sp], 16
  sub w0, w1, w0
  mov w19, w0  // salva em reg
  mov w0, 10
  mov w1, w19  // restaura do reg
  mul w0, w1, w0
  ldr w1, [sp], 16
  add w0, w1, w0
  str w0, [sp, -16]!
  ldr w0, [x29, -96]
  str w0, [sp, -16]!
  mov w0, 3
  ldr w1, [sp], 16
  add w0, w1, w0
  str w0, [sp, -16]!
  mov w0, 0
  ldr w1, [sp], 16
  mov w2, 1
  mul w1, w1, w2
  add w0, w0, w1
  ldr x2, [x29, -48]
  add x2, x2, x0
  ldrb w0, [x2]
  str w0, [sp, -16]!
  mov w0, 48
  ldr w1, [sp], 16
  sub w0, w1, w0
  ldr w1, [sp], 16
  add w0, w1, w0
  mov w19, w0
  mov w0, w19
  str w0, [x29, 288]
.B14:
.B12:
.B10:
  mov w0, 0
  mov w19, w0
  str w0, [x29, 272]
  ldrb w0, [x29, -112]
  mov w19, w0
  mov w0, w19
  cmp w0, 0
  beq .B15
  ldr w0, [x29, 288]
  neg w0, w0
  mov w19, w0
  mov w0, w19
  str w0, [x29, 272]
  b .B16
.B15:
  ldr w0, [x29, 288]
  mov w19, w0
  mov w0, w19
  str w0, [x29, 272]
.B16:
  ldr w0, [x29, 272]
  b 1f
  b .B17
.B8:
.B17:
  mov w0, 0
  mov w19, w0
  str w0, [x29, -160]
.B19:
  ldr w0, [x29, -96]
  mov w19, w0
  ldr w0, [x29, -80]
  mov w1, w19
  cmp w1, w0
  cset w0, lt
  mov w19, w0
  mov w0, w19
  cmp w0, 0
  beq .B20
  ldr w0, [x29, -96]
  str w0, [sp, -16]!
  mov w0, 0
  ldr w1, [sp], 16
  mov w2, 1
  mul w1, w1, w2
  add w0, w0, w1
  ldr x2, [x29, -48]
  add x2, x2, x0
  ldrb w0, [x2]
  mov w19, w0
  mov w0, 48
  mov w1, w19
  cmp w1, w0
  cset w0, ge
  str w0, [sp, -16]!
  ldr w0, [x29, -96]
  str w0, [sp, -16]!
  mov w0, 0
  ldr w1, [sp], 16
  mov w2, 1
  mul w1, w1, w2
  add w0, w0, w1
  ldr x2, [x29, -48]
  add x2, x2, x0
  ldrb w0, [x2]
  mov w19, w0
  mov w0, 57
  mov w1, w19
  cmp w1, w0
  cset w0, le
  ldr w1, [sp], 16
  cmp w1, 0
  cset w1, ne
  cmp w0, 0
  cset w0, ne
  and w0, w1, w0
  mov w19, w0
  mov w0, w19
  cmp w0, 0
  beq .B21
  ldr w0, [x29, -160]
  mov w19, w0  // salva em reg
  mov w0, 10
  mov w1, w19  // restaura do reg
  mul w0, w1, w0
  str w0, [sp, -16]!
  ldr w0, [x29, -96]
  str w0, [sp, -16]!
  mov w0, 0
  ldr w1, [sp], 16
  mov w2, 1
  mul w1, w1, w2
  add w0, w0, w1
  ldr x2, [x29, -48]
  add x2, x2, x0
  ldrb w0, [x2]
  str w0, [sp, -16]!
  mov w0, 48
  ldr w1, [sp], 16
  sub w0, w1, w0
  ldr w1, [sp], 16
  add w0, w1, w0
  mov w19, w0
  mov w0, w19
  str w0, [x29, -160]
  b .B22
.B21:
.B22:
  ldr w0, [x29, -96]
  add w0, w0, 1
  str w0, [x29, -96]
  b .B19
.B20:
  ldrb w0, [x29, -112]
  mov w19, w0
  mov w0, w19
  cmp w0, 0
  beq .B23
  ldr w0, [x29, -160]
  neg w0, w0
  mov w19, w0
  mov w0, w19
  str w0, [x29, 272]
  b .B24
.B23:
  ldr w0, [x29, -160]
  mov w19, w0
  mov w0, w19
  str w0, [x29, 272]
.B24:
  ldr w0, [x29, 272]
  b 1f
  b 1f
// epilogo
1:
  ldp x19, x20, [x29, -16]
  ldp x21, x22, [x29, -32]
  ldp x29, x30, [sp, 288]
  add sp, sp, 304
  ret
// fim: [cvtint]
// fn: [cvtflu] (vars: 64, total: 240)
.align 2
cvtflu:
  sub sp, sp, 240
  stp x29, x30, [sp, 224]
  add x29, sp, 224
  stp x19, x20, [x29, -16]
  stp x21, x22, [x29, -32]
  str x0, [x29, -48]  // param v
  str x1, [x29, -56]  // param tam
  ldr w0, [x29, -56]
  mov w19, w0
  mov w0, 0
  mov w1, w19
  cmp w1, w0
  cset w0, eq
  mov w19, w0
  mov w0, w19
  cmp w0, 0
  beq .B25
  ldr x0, = const_0
  ldr s0, [x0]
  b 1f
  b .B26
.B25:
.B26:
  mov w0, 0
  mov w19, w0
  str w0, [x29, -80]
  mov w0, 0
  mov w19, w0
  strb w0, [x29, -96]
  mov w0, 0
  str w0, [sp, -16]!
  mov w0, 0
  ldr w1, [sp], 16
  mov w2, 1
  mul w1, w1, w2
  add w0, w0, w1
  ldr x2, [x29, -48]
  add x2, x2, x0
  ldrb w0, [x2]
  mov w19, w0
  mov w0, 45
  mov w1, w19
  cmp w1, w0
  cset w0, eq
  mov w19, w0
  mov w0, w19
  cmp w0, 0
  beq .B27
  mov w0, 1
  mov w19, w0
  mov w0, w19
  strb w0, [x29, -96]
  mov w0, 1
  mov w19, w0
  mov w0, w19
  str w0, [x29, -80]
  b .B28
.B27:
  mov w0, 0
  str w0, [sp, -16]!
  mov w0, 0
  ldr w1, [sp], 16
  mov w2, 1
  mul w1, w1, w2
  add w0, w0, w1
  ldr x2, [x29, -48]
  add x2, x2, x0
  ldrb w0, [x2]
  mov w19, w0
  mov w0, 43
  mov w1, w19
  cmp w1, w0
  cset w0, eq
  mov w19, w0
  mov w0, w19
  cmp w0, 0
  beq .B29
  mov w0, 1
  mov w19, w0
  mov w0, w19
  str w0, [x29, -80]
  b .B30
.B29:
.B30:
.B28:
  ldr x0, = const_0
  ldr s0, [x0]
  fmov s19, s0
  str s0, [x29, -112]
.B32:
  ldr w0, [x29, -80]
  mov w19, w0
  ldr w0, [x29, -56]
  mov w1, w19
  cmp w1, w0
  cset w0, lt
  str w0, [sp, -16]!
  ldr w0, [x29, -80]
  str w0, [sp, -16]!
  mov w0, 0
  ldr w1, [sp], 16
  mov w2, 1
  mul w1, w1, w2
  add w0, w0, w1
  ldr x2, [x29, -48]
  add x2, x2, x0
  ldrb w0, [x2]
  mov w19, w0
  mov w0, 46
  mov w1, w19
  cmp w1, w0
  cset w0, ne
  str w0, [sp, -16]!
  ldr w0, [x29, -80]
  str w0, [sp, -16]!
  mov w0, 0
  ldr w1, [sp], 16
  mov w2, 1
  mul w1, w1, w2
  add w0, w0, w1
  ldr x2, [x29, -48]
  add x2, x2, x0
  ldrb w0, [x2]
  mov w19, w0
  mov w0, 101
  mov w1, w19
  cmp w1, w0
  cset w0, ne
  str w0, [sp, -16]!
  ldr w0, [x29, -80]
  str w0, [sp, -16]!
  mov w0, 0
  ldr w1, [sp], 16
  mov w2, 1
  mul w1, w1, w2
  add w0, w0, w1
  ldr x2, [x29, -48]
  add x2, x2, x0
  ldrb w0, [x2]
  mov w19, w0
  mov w0, 69
  mov w1, w19
  cmp w1, w0
  cset w0, ne
  ldr w1, [sp], 16
  cmp w1, 0
  cset w1, ne
  cmp w0, 0
  cset w0, ne
  and w0, w1, w0
  ldr w1, [sp], 16
  cmp w1, 0
  cset w1, ne
  cmp w0, 0
  cset w0, ne
  and w0, w1, w0
  ldr w1, [sp], 16
  cmp w1, 0
  cset w1, ne
  cmp w0, 0
  cset w0, ne
  and w0, w1, w0
  mov w19, w0
  mov w0, w19
  cmp w0, 0
  beq .B33
  ldr w0, [x29, -80]
  str w0, [sp, -16]!
  mov w0, 0
  ldr w1, [sp], 16
  mov w2, 1
  mul w1, w1, w2
  add w0, w0, w1
  ldr x2, [x29, -48]
  add x2, x2, x0
  ldrb w0, [x2]
  mov w19, w0
  mov w0, 48
  mov w1, w19
  cmp w1, w0
  cset w0, ge
  str w0, [sp, -16]!
  ldr w0, [x29, -80]
  str w0, [sp, -16]!
  mov w0, 0
  ldr w1, [sp], 16
  mov w2, 1
  mul w1, w1, w2
  add w0, w0, w1
  ldr x2, [x29, -48]
  add x2, x2, x0
  ldrb w0, [x2]
  mov w19, w0
  mov w0, 57
  mov w1, w19
  cmp w1, w0
  cset w0, le
  ldr w1, [sp], 16
  cmp w1, 0
  cset w1, ne
  cmp w0, 0
  cset w0, ne
  and w0, w1, w0
  mov w19, w0
  mov w0, w19
  cmp w0, 0
  beq .B34
  ldr s0, [x29, -112]
  fmov s19, s0  // salva em reg
  ldr x0, = const_1
  ldr s0, [x0]
  fmov s1, s19  // restaura do reg
  fmul s0, s1, s0
  str s0, [sp, -16]!
  ldr w0, [x29, -80]
  str w0, [sp, -16]!
  mov w0, 0
  ldr w1, [sp], 16
  mov w2, 1
  mul w1, w1, w2
  add w0, w0, w1
  ldr x2, [x29, -48]
  add x2, x2, x0
  ldrb w0, [x2]
  str w0, [sp, -16]!
  mov w0, 48
  ldr w1, [sp], 16
  sub w0, w1, w0
  sxtb w0, w0
  scvtf s0, w0
  ldr s1, [sp], 16
  fadd s0, s1, s0
  fmov s19, s0
  fmov s0, s19
  str s0, [x29, -112]
  b .B35
.B34:
.B35:
  ldr w0, [x29, -80]
  add w0, w0, 1
  str w0, [x29, -80]
  b .B32
.B33:
  ldr w0, [x29, -80]
  mov w19, w0
  ldr w0, [x29, -56]
  mov w1, w19
  cmp w1, w0
  cset w0, lt
  str w0, [sp, -16]!
  ldr w0, [x29, -80]
  str w0, [sp, -16]!
  mov w0, 0
  ldr w1, [sp], 16
  mov w2, 1
  mul w1, w1, w2
  add w0, w0, w1
  ldr x2, [x29, -48]
  add x2, x2, x0
  ldrb w0, [x2]
  mov w19, w0
  mov w0, 46
  mov w1, w19
  cmp w1, w0
  cset w0, eq
  ldr w1, [sp], 16
  cmp w1, 0
  cset w1, ne
  cmp w0, 0
  cset w0, ne
  and w0, w1, w0
  mov w19, w0
  mov w0, w19
  cmp w0, 0
  beq .B36
  ldr w0, [x29, -80]
  add w0, w0, 1
  str w0, [x29, -80]
  ldr x0, = const_2
  ldr s0, [x0]
  fmov s19, s0
  str s0, [x29, 224]
.B38:
  ldr w0, [x29, -80]
  mov w19, w0
  ldr w0, [x29, -56]
  mov w1, w19
  cmp w1, w0
  cset w0, lt
  str w0, [sp, -16]!
  ldr w0, [x29, -80]
  str w0, [sp, -16]!
  mov w0, 0
  ldr w1, [sp], 16
  mov w2, 1
  mul w1, w1, w2
  add w0, w0, w1
  ldr x2, [x29, -48]
  add x2, x2, x0
  ldrb w0, [x2]
  mov w19, w0
  mov w0, 48
  mov w1, w19
  cmp w1, w0
  cset w0, ge
  str w0, [sp, -16]!
  ldr w0, [x29, -80]
  str w0, [sp, -16]!
  mov w0, 0
  ldr w1, [sp], 16
  mov w2, 1
  mul w1, w1, w2
  add w0, w0, w1
  ldr x2, [x29, -48]
  add x2, x2, x0
  ldrb w0, [x2]
  mov w19, w0
  mov w0, 57
  mov w1, w19
  cmp w1, w0
  cset w0, le
  ldr w1, [sp], 16
  cmp w1, 0
  cset w1, ne
  cmp w0, 0
  cset w0, ne
  and w0, w1, w0
  ldr w1, [sp], 16
  cmp w1, 0
  cset w1, ne
  cmp w0, 0
  cset w0, ne
  and w0, w1, w0
  mov w19, w0
  mov w0, w19
  cmp w0, 0
  beq .B39
  ldr s0, [x29, -112]
  str s0, [sp, -16]!
  ldr w0, [x29, -80]
  str w0, [sp, -16]!
  mov w0, 0
  ldr w1, [sp], 16
  mov w2, 1
  mul w1, w1, w2
  add w0, w0, w1
  ldr x2, [x29, -48]
  add x2, x2, x0
  ldrb w0, [x2]
  str w0, [sp, -16]!
  mov w0, 48
  ldr w1, [sp], 16
  sub w0, w1, w0
  sxtb w0, w0
  scvtf s0, w0
  fmov s19, s0  // salva em reg
  ldr s0, [x29, 224]
  fmov s1, s19  // restaura do reg
  fmul s0, s1, s0
  ldr s1, [sp], 16
  fadd s0, s1, s0
  fmov s19, s0
  fmov s0, s19
  str s0, [x29, -112]
  ldr s0, [x29, 224]
  fmov s19, s0  // salva em reg
  ldr x0, = const_2
  ldr s0, [x0]
  fmov s1, s19  // restaura do reg
  fmul s0, s1, s0
  fmov s19, s0
  fmov s0, s19
  str s0, [x29, 224]
  ldr w0, [x29, -80]
  add w0, w0, 1
  str w0, [x29, -80]
  b .B38
.B39:
  b .B40
.B36:
.B40:
  ldrb w0, [x29, -96]
  mov w19, w0
  mov w0, w19
  cmp w0, 0
  beq .B41
  ldr s0, [x29, -112]
  fneg s0, s0
  fmov s19, s0
  fmov s0, s19
  str s0, [x29, -112]
  b .B42
.B41:
.B42:
  ldr s0, [x29, -112]
  b 1f
  b 1f
// epilogo
1:
  ldp x19, x20, [x29, -16]
  ldp x21, x22, [x29, -32]
  ldp x29, x30, [sp, 224]
  add sp, sp, 240
  ret
// fim: [cvtflu]
.global inicio
