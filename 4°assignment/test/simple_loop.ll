; simple_loop.ll
define void @foo(i32* %A) {
entry:
  br label %loop

loop:
  %i = phi i32 [0, %entry], [%i.next, %loop]
  %val = load i32, i32* %A               
  %tmp = add i32 %i, %val                
  %i.next = add i32 %i, 1
  %cond = icmp slt i32 %i.next, 10
  br i1 %cond, label %loop, label %exit

exit:
  ret void
}
