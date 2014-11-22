.thumb
.syntax unified
.section INTERRUPT_VECTOR, "x"
.global _Reset
_Reset:
  B Reset_Handler /* Reset */
  B . /* Undefined */
  B . /* SWI */
  B . /* Prefetch Abort */
  B . /* Data Abort */
  B . /* reserved */
  B . /* IRQ */
  B . /* FIQ */
 
Reset_Handler:
  LDR sp, =stack_top
  MOV r0, #41
  MOV r1, #41
  MOV r2, #41
  MOV r3, #41
  MOV r4, #41
  MOV r5, #41
  MOV r6, #41
  MOV r7, #41
  MOV r8, #41
  MOV r9, #41
  MOV r10, #41
  MOV r11, #41
//  BL c_entry
  B .
