; ModuleID = '../test/simple_loop.ll'
source_filename = "../test/simple_loop.ll"

define void @foo(ptr %A) {
entry:
  br label %loop

loop:                                             ; preds = %loop, %entry
  %i = phi i32 [ 0, %entry ], [ %i.next, %loop ]
  %val = load i32, ptr %A, align 4
  %tmp = add i32 %i, %val
  %i.next = add i32 %i, 1
  %cond = icmp slt i32 %i.next, 10
  br i1 %cond, label %loop, label %exit

exit:                                             ; preds = %loop
  ret void
}
