CS3339 MIPS Disassembler
  400000: j 400004
  400004: sw $ra, -4($sp)
  400008: sw $fp, -8($sp)
  40000c: addiu $fp, $sp, -8
  400010: addiu $sp, $fp, -56
  400014: addiu $k1, $zero, 7
  400018: sw $k1, -40($fp)
  40001c: sw $zero, -32($fp)
  400020: lw $k1, -32($fp)
  400024: addiu $k0, $zero, 1297
  400028: slt $k1, $k1, $k0
  40002c: beq $k1, $zero, 400058
  400030: lw $k1, -32($fp)
  400034: sll $k1, $k1, 2
  400038: addu $k1, $k1, $gp
  40003c: lw $k0, -32($fp)
  400040: sll $k0, $k0, 2
  400044: sw $k0, 12028($k1)
  400048: lw $k1, -32($fp)
  40004c: addiu $k1, $k1, 1
  400050: sw $k1, -32($fp)
  400054: beq $zero, $zero, 400020
  400058: sw $zero, -32($fp)
  40005c: lw $k1, -32($fp)
  400060: addiu $k0, $zero, 36
  400064: slt $k1, $k1, $k0
  400068: beq $k1, $zero, 40022c
  40006c: lw $k1, -32($fp)
  400070: addiu $k1, $k1, -1
  400074: sw $k1, -44($fp)
  400078: lw $k1, -44($fp)
  40007c: addiu $k0, $zero, 0
  400080: slt $k1, $k1, $k0
  400084: beq $k1, $zero, 400090
  400088: addiu $k1, $zero, 35
  40008c: sw $k1, -44($fp)
  400090: lw $k1, -32($fp)
  400094: addiu $k1, $k1, 1
  400098: sw $k1, -48($fp)
  40009c: lw $k1, -48($fp)
  4000a0: addiu $k0, $zero, 36
  4000a4: bne $k1, $k0, 4000ac
  4000a8: sw $zero, -48($fp)
  4000ac: sw $zero, -36($fp)
  4000b0: lw $k1, -36($fp)
  4000b4: addiu $k0, $zero, 36
  4000b8: slt $k1, $k1, $k0
  4000bc: beq $k1, $zero, 40021c
  4000c0: lw $k1, -36($fp)
  4000c4: addiu $k1, $k1, -1
  4000c8: sw $k1, -52($fp)
  4000cc: lw $k1, -52($fp)
  4000d0: addiu $k0, $zero, 0
  4000d4: slt $k1, $k1, $k0
  4000d8: beq $k1, $zero, 4000e4
  4000dc: addiu $k1, $zero, 35
  4000e0: sw $k1, -52($fp)
  4000e4: lw $k1, -36($fp)
  4000e8: addiu $k1, $k1, 1
  4000ec: sw $k1, -56($fp)
  4000f0: lw $k1, -56($fp)
  4000f4: addiu $k0, $zero, 36
  4000f8: bne $k1, $k0, 400100
  4000fc: sw $zero, -56($fp)
  400100: lw $k1, -32($fp)
  400104: addiu $k0, $zero, 36
  400108: mult $k1, $k0
  40010c: mflo $k1
  400110: lw $k0, -36($fp)
  400114: addu $k1, $k1, $k0
  400118: sll $k1, $k1, 2
  40011c: sll $k1, $k1, 2
  400120: addu $k1, $k1, $gp
  400124: lw $k0, -32($fp)
  400128: addiu $t9, $zero, 36
  40012c: mult $k0, $t9
  400130: mflo $k0
  400134: lw $t9, -52($fp)
  400138: addu $k0, $k0, $t9
  40013c: sw $k0, -8708($k1)
  400140: lw $k1, -32($fp)
  400144: addiu $k0, $zero, 36
  400148: mult $k1, $k0
  40014c: mflo $k1
  400150: lw $k0, -36($fp)
  400154: addu $k1, $k1, $k0
  400158: sll $k1, $k1, 2
  40015c: addiu $k1, $k1, 1
  400160: sll $k1, $k1, 2
  400164: addu $k1, $k1, $gp
  400168: lw $k0, -32($fp)
  40016c: addiu $t9, $zero, 36
  400170: mult $k0, $t9
  400174: mflo $k0
  400178: lw $t9, -56($fp)
  40017c: addu $k0, $k0, $t9
  400180: sw $k0, -8708($k1)
  400184: lw $k1, -32($fp)
  400188: addiu $k0, $zero, 36
  40018c: mult $k1, $k0
  400190: mflo $k1
  400194: lw $k0, -36($fp)
  400198: addu $k1, $k1, $k0
  40019c: sll $k1, $k1, 2
  4001a0: addiu $k1, $k1, 2
  4001a4: sll $k1, $k1, 2
  4001a8: addu $k1, $k1, $gp
  4001ac: lw $k0, -44($fp)
  4001b0: addiu $t9, $zero, 36
  4001b4: mult $k0, $t9
  4001b8: mflo $k0
  4001bc: lw $t9, -36($fp)
  4001c0: addu $k0, $k0, $t9
  4001c4: sw $k0, -8708($k1)
  4001c8: lw $k1, -32($fp)
  4001cc: addiu $k0, $zero, 36
  4001d0: mult $k1, $k0
  4001d4: mflo $k1
  4001d8: lw $k0, -36($fp)
  4001dc: addu $k1, $k1, $k0
  4001e0: sll $k1, $k1, 2
  4001e4: addiu $k1, $k1, 3
  4001e8: sll $k1, $k1, 2
  4001ec: addu $k1, $k1, $gp
  4001f0: lw $k0, -48($fp)
  4001f4: addiu $t9, $zero, 36
  4001f8: mult $k0, $t9
  4001fc: mflo $k0
  400200: lw $t9, -36($fp)
  400204: addu $k0, $k0, $t9
  400208: sw $k0, -8708($k1)
  40020c: lw $k1, -36($fp)
  400210: addiu $k1, $k1, 1
  400214: sw $k1, -36($fp)
  400218: beq $zero, $zero, 4000b0
  40021c: lw $k1, -32($fp)
  400220: addiu $k1, $k1, 1
  400224: sw $k1, -32($fp)
  400228: beq $zero, $zero, 40005c
  40022c: sw $zero, -32($fp)
  400230: lw $k1, -32($fp)
  400234: addiu $k0, $zero, 5184
  400238: slt $k1, $k1, $k0
  40023c: beq $k1, $zero, 400264
  400240: lw $k1, -32($fp)
  400244: sll $k1, $k1, 2
  400248: addu $k1, $k1, $gp
  40024c: addiu $k0, $zero, 1
  400250: sw $k0, -29444($k1)
  400254: lw $k1, -32($fp)
  400258: addiu $k1, $k1, 1
  40025c: sw $k1, -32($fp)
  400260: beq $zero, $zero, 400230
  400264: addiu $k1, $zero, 1296
  400268: sw $k1, -12($fp)
  40026c: sw $zero, -32($fp)
  400270: lw $k1, -32($fp)
  400274: lw $k0, -12($fp)
  400278: slt $k1, $k1, $k0
  40027c: beq $k1, $zero, 4002a8
  400280: lw $k1, -32($fp)
  400284: sll $k1, $k1, 2
  400288: addu $k1, $k1, $gp
  40028c: lui $k0, -32768
  400290: addiu $k0, $k0, -1
  400294: sw $k0, 22400($k1)
  400298: lw $k1, -32($fp)
  40029c: addiu $k1, $k1, 1
  4002a0: sw $k1, -32($fp)
  4002a4: beq $zero, $zero, 400270
  4002a8: lw $k1, -40($fp)
  4002ac: sll $k1, $k1, 2
  4002b0: addu $k1, $k1, $gp
  4002b4: sw $zero, 22400($k1)
  4002b8: lw $k1, -40($fp)
  4002bc: sll $k1, $k1, 2
  4002c0: addu $k1, $k1, $gp
  4002c4: addiu $k0, $zero, -1
  4002c8: sw $k0, 17216($k1)
  4002cc: lw $k1, -40($fp)
  4002d0: sw $k1, 27584($gp)
  4002d4: sw $zero, -4($fp)
  4002d8: addiu $k1, $zero, 1
  4002dc: sw $k1, -8($fp)
  4002e0: lw $k1, -4($fp)
  4002e4: lw $k0, -8($fp)
  4002e8: beq $k1, $k0, 40041c
  4002ec: lw $k1, -4($fp)
  4002f0: sll $k1, $k1, 2
  4002f4: addu $k1, $k1, $gp
  4002f8: lw $k0, 27584($k1)
  4002fc: sw $k0, -16($fp)
  400300: lw $k1, -4($fp)
  400304: addiu $k1, $k1, 1
  400308: sw $k1, -4($fp)
  40030c: lw $k1, -4($fp)
  400310: lw $k0, -12($fp)
  400314: bne $k1, $k0, 40031c
  400318: sw $zero, -4($fp)
  40031c: lw $k1, -16($fp)
  400320: sll $k1, $k1, 2
  400324: addu $k1, $k1, $gp
  400328: lw $k0, 22400($k1)
  40032c: sw $k0, -28($fp)
  400330: lw $k1, -16($fp)
  400334: sll $k1, $k1, 2
  400338: addu $k1, $k1, $gp
  40033c: lw $k0, 12028($k1)
  400340: sw $k0, -32($fp)
  400344: lw $k1, -16($fp)
  400348: addiu $k1, $k1, 1
  40034c: sll $k1, $k1, 2
  400350: addu $k1, $k1, $gp
  400354: lw $k0, -32($fp)
  400358: lw $t9, 12028($k1)
  40035c: slt $k0, $k0, $t9
  400360: beq $k0, $zero, 400418
  400364: lw $k1, -32($fp)
  400368: sll $k1, $k1, 2
  40036c: addu $k1, $k1, $gp
  400370: lw $k0, -8708($k1)
  400374: sw $k0, -20($fp)
  400378: lw $k1, -32($fp)
  40037c: sll $k1, $k1, 2
  400380: addu $k1, $k1, $gp
  400384: lw $k0, -28($fp)
  400388: lw $t9, -29444($k1)
  40038c: addu $k0, $k0, $t9
  400390: sw $k0, -24($fp)
  400394: lw $k1, -20($fp)
  400398: sll $k1, $k1, 2
  40039c: addu $k1, $k1, $gp
  4003a0: lw $k0, -24($fp)
  4003a4: lw $t9, 22400($k1)
  4003a8: slt $k0, $k0, $t9
  4003ac: beq $k0, $zero, 400408
  4003b0: lw $k1, -20($fp)
  4003b4: sll $k1, $k1, 2
  4003b8: addu $k1, $k1, $gp
  4003bc: lw $k0, -24($fp)
  4003c0: sw $k0, 22400($k1)
  4003c4: lw $k1, -20($fp)
  4003c8: sll $k1, $k1, 2
  4003cc: addu $k1, $k1, $gp
  4003d0: lw $k0, -16($fp)
  4003d4: sw $k0, 17216($k1)
  4003d8: lw $k1, -8($fp)
  4003dc: sll $k1, $k1, 2
  4003e0: addu $k1, $k1, $gp
  4003e4: lw $k0, -20($fp)
  4003e8: sw $k0, 27584($k1)
  4003ec: lw $k1, -8($fp)
  4003f0: addiu $k1, $k1, 1
  4003f4: sw $k1, -8($fp)
  4003f8: lw $k1, -8($fp)
  4003fc: lw $k0, -12($fp)
  400400: bne $k1, $k0, 400408
  400404: sw $zero, -8($fp)
  400408: lw $k1, -32($fp)
  40040c: addiu $k1, $k1, 1
  400410: sw $k1, -32($fp)
  400414: beq $zero, $zero, 400344
  400418: beq $zero, $zero, 4002e0
  40041c: lw $k1, 22400($gp)
  400420: trap 3600001
  400424: lw $k1, 17216($gp)
  400428: trap 3600001
  40042c: trap 0
  400430: addiu $sp, $fp, 8
  400434: lw $ra, 4($fp)
  400438: lw $fp, 0($fp)
  40043c: trap a
