@A = global [100 x i32] zeroinitializer
@B = global [100 x i32] zeroinitializer

define void @two_loops() {
entry:
  br label %loop1_header

loop1_header:
  %i1 = phi i32 [ 0, %entry ], [ %i1_next, %loop1_latch ]
  %cond1 = icmp slt i32 %i1, 100
  br i1 %cond1, label %loop1_body, label %loop2_header

loop1_body:
  %a_ptr = getelementptr inbounds [100 x i32], [100 x i32]* @A, i32 0, i32 %i1
  %a_val = load i32, i32* %a_ptr
  %a_add = add i32 %a_val, 1
  store i32 %a_add, i32* %a_ptr
  %i1_next = add i32 %i1, 1
  br label %loop1_latch

loop1_latch:
  br label %loop1_header

loop2_header:
  %i2 = phi i32 [ 0, %loop1_header ], [ %i2_next, %loop2_latch ]
  %cond2 = icmp slt i32 %i2, 100
  br i1 %cond2, label %loop2_body, label %end

loop2_body:
  %b_ptr = getelementptr inbounds [100 x i32], [100 x i32]* @B, i32 0, i32 %i2
  %b_val = load i32, i32* %b_ptr
  %b_add = add i32 %b_val, 1
  store i32 %b_add, i32* %b_ptr
  %i2_next = add i32 %i2, 1
  br label %loop2_latch

loop2_latch:
  br label %loop2_header

end:
  ret void
}
